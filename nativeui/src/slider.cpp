#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "slider.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>
#include "font.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

namespace nativeui {

Slider::Slider(SliderShape shape) : shape_(shape) {
    if (shape == SliderShape::Arc) {
        width_ = 100.0f; // Default radius
    }
}

void Slider::set_range(float min, float max) {
    min_ = min;
    max_ = max;
    // Clamp current value
    value_ = std::clamp(value_, min_, max_);
}

void Slider::set_value(float value) {
    value_ = std::clamp(value, min_, max_);
    if (on_change_) on_change_(value_);
}

void Slider::set_shape(SliderShape shape) {
    shape_ = shape;
}

void Slider::set_position(float x, float y) {
    x_ = x;
    y_ = y;
}

void Slider::set_dimensions(float width, float height) {
    width_ = width;
    height_ = height;
    current_thickness_ = height;
}

void Slider::set_arc_angles(float start, float sweep) {
    start_angle_ = start;
    sweep_angle_ = sweep;
}

void Slider::set_colors(const nativeui::Color& bg, const nativeui::Color& fill, const nativeui::Color& text) {
    bg_color_ = bg;
    fill_color_ = fill;
    text_color_ = text;
}

void Slider::update(float dt) {
    // Stability: Clamp dt to prevent physics explosion
    if (dt > 0.04f) dt = 0.04f;
    if (dt < 0.0f) dt = 0.0f;

    // Spring physics constants
    const float tension = 150.0f;
    const float friction = 25.0f; // Critical damping logic relies on friction * dt < 2.0

    // Animate Value Display
    float val_diff = value_ - current_value_display_;
    float val_acc = val_diff * tension - value_velocity_ * friction;
    value_velocity_ += val_acc * dt;
    current_value_display_ += value_velocity_ * dt;
    
    // Hard clamp to limits to prevent boundary violation
    if (current_value_display_ < min_) {
        current_value_display_ = min_;
        value_velocity_ = 0.0f;
    } else if (current_value_display_ > max_) {
        current_value_display_ = max_;
        value_velocity_ = 0.0f;
    }
    
    // Snap if close
    if (std::abs(val_diff) < 0.1f && std::abs(value_velocity_) < 0.1f) {
        current_value_display_ = value_;
        value_velocity_ = 0.0f;
    }

    // Animate Thickness
    float target_thickness = is_hovered_ || is_dragging_ ? height_ * 1.5f : height_;
    float thick_diff = target_thickness - current_thickness_;
    float thick_acc = thick_diff * tension - thickness_velocity_ * friction;
    thickness_velocity_ += thick_acc * dt;
    current_thickness_ += thickness_velocity_ * dt;
    
    // Animate Overshoot (Elasticity)
    // If not dragging, target overshoot is 0 (spring back)
    if (!is_dragging_) {
        drag_overshoot_ = 0.0f;
    }
    
    // Animate current_overshoot_ to drag_overshoot_
    float over_diff = drag_overshoot_ - current_overshoot_;
    float over_acc = over_diff * tension - overshoot_velocity_ * friction;
    overshoot_velocity_ += over_acc * dt;
    current_overshoot_ += overshoot_velocity_ * dt;
    
    // Fine Control Logic
    if (is_pressing_candidate_) {
        time_since_press_ += dt;
        if (time_since_press_ > 0.3f) { // 0.3s hold trigger
            is_pressing_candidate_ = false;
            is_fine_control_active_ = true;
            // Re-base drag to prevent jump when scale changes
            drag_start_value_ = value_;
            drag_start_mouse_x_ = press_origin_x_; 
        }
    }
    
    // Animate Zoom
    float target_zoom = is_fine_control_active_ ? 4.0f : 1.0f; // 4x Zoom
    float zoom_diff = target_zoom - current_zoom_;
    float zoom_acc = zoom_diff * tension - zoom_velocity_ * friction;
    zoom_velocity_ += zoom_acc * dt;
    current_zoom_ += zoom_velocity_ * dt;
}

    void Slider::handle_event(const nativeui::Event& event) {
    if (event.type == nativeui::EventType::MouseMotion) {
        if (is_dragging_) {
            // Check for hold cancel
            if (is_pressing_candidate_) {
                if (std::abs(event.mouse_x - press_origin_x_) > 5) {
                    is_pressing_candidate_ = false;
                    time_since_press_ = 0.0f;
                }
            }
            update_value_from_mouse(event.mouse_x, event.mouse_y);
        } else {
            is_hovered_ = hit_test(event.mouse_x, event.mouse_y);
        }
    } else if (event.type == nativeui::EventType::MouseButtonDown && event.mouse_button == 1) { // Left click
        if (hit_test(event.mouse_x, event.mouse_y)) {
            is_dragging_ = true;
            if (shape_ == SliderShape::Selector) {
                drag_start_value_ = value_;
                drag_start_mouse_x_ = event.mouse_x;
                
                // Init Fine Control candidate
                if (fine_control_enabled_) {
                    is_pressing_candidate_ = true;
                    time_since_press_ = 0.0f;
                    press_origin_x_ = event.mouse_x;
                    is_fine_control_active_ = false;
                }
            }
            update_value_from_mouse(event.mouse_x, event.mouse_y);
        }
    } else if (event.type == nativeui::EventType::MouseButtonUp && event.mouse_button == 1) {
        if (is_dragging_) {
            is_dragging_ = false;
            is_pressing_candidate_ = false;
            is_fine_control_active_ = false;
            
            if (shape_ == SliderShape::Selector) {
                // Snap to nearest tick 
                float snap_density = 10.0f;
                // Use finer snap if we were zoomed in
                if (current_zoom_ > 2.0f) snap_density = 50.0f;
                
                float v = value_to_visual(value_);
                float snapped_v = std::round(v * snap_density) / snap_density;
                float snapped_val = visual_to_value(snapped_v);
                
                // Always snap
                set_value(snapped_val);
                if (on_change_) on_change_(value_);
            }
            // Overshoot will animate back to 0 in update() for linear sliders
        }
    } else if (event.type == nativeui::EventType::MouseWheel) {
        if (is_hovered_) {
            float range = max_ - min_;
            float step = range * 0.05f; // 5% scroll step
            if (event.wheel_y != 0) {
                float new_val = value_ + event.wheel_y * step;
                set_value(new_val);
                if (on_change_) on_change_(value_);
            }
        }
    }
}

bool Slider::hit_test(int mx, int my) const {
    float mx_f = (float)mx;
    float my_f = (float)my;

    if (shape_ == SliderShape::Arc) {
        // Distance check from center (x, y)
        float dx = mx_f - x_;
        float dy = my_f - y_;
        float dist = std::sqrt(dx*dx + dy*dy);
        
        // Check fuzzy radius
        float tolerance = std::max(height_, 20.0f); // Hit area at least 20px
        return std::abs(dist - width_) < tolerance;
    } else {
        // Rect/Pill
        // Assume horizontal for simplicity if width > height, vertical otherwise? 
        // Or user just rotates dimensions. Let's assume defined bounds (x,y) to (x+w, y+h)
        // But for Pill shape, usually centered line thickness.
        // Let's assume standard UI: Box bounding (x, y, w, h)
        // BUT user said "straight long pill shape".
        // Let's treat (x,y) as Top-Left.
        
        // Wait, thickness expands. The hit box should be the expanded size.
        float hit_h = std::max(height_ * 2.0f, 20.0f);
        
        // Center Y for hit test
        float cy = y_ + height_ / 2.0f;
        
        return (mx_f >= x_ && mx_f <= x_ + width_ && 
                my_f >= cy - hit_h/2.0f && my_f <= cy + hit_h/2.0f);
    }
}

void Slider::update_value_from_mouse(int mx, int my) {
    if (shape_ == SliderShape::Selector) {
        float scale = pixels_per_segment_ * current_zoom_;
        float delta_px = (float)(drag_start_mouse_x_ - mx);
        float visual_delta = delta_px / scale;
        float current_v = value_to_visual(drag_start_value_) + visual_delta;
        
        float max_v = 0.0f;
        if (!stops_.empty()) max_v = (float)(stops_.size() - 1);
        
        if (current_v < 0.0f) {
            set_value(min_);
            float excess_visual = current_v; // Negative
            float excess_px = excess_visual * scale;
            drag_overshoot_ = excess_px * 0.5f; // Rubber band resistance
        } else if (current_v > max_v) {
            set_value(max_);
            float excess_visual = current_v - max_v; // Positive
            float excess_px = excess_visual * scale;
            drag_overshoot_ = excess_px * 0.5f; // Rubber band resistance
        } else {
            float new_val = visual_to_value(current_v);
            set_value(new_val);
            drag_overshoot_ = 0.0f;
        }
        return;
    }

    float t = 0.0f; // 0.0 to 1.0

    if (shape_ == SliderShape::Arc) {
        // Angle calculation
        float dx = (float)mx - x_;
        float dy = (float)my - y_;
        float angle_rad = std::atan2(dy, dx);
        float angle_deg = angle_rad * (180.0f / M_PI);
        
        // Normalize angle relative to start_angle
        // Making sure we handle wrap-around correctly is hard for arbitrary sweep
        // Simple approach: unwrap
        
        float relative_angle = angle_deg - start_angle_;
        while (relative_angle < 0) relative_angle += 360.0f;
        while (relative_angle >= 360.0f) relative_angle -= 360.0f;
        
        // Handle wrap-around snap
        if (relative_angle > sweep_angle_) {
             // We are in the "dead zone" (assuming sweep < 360)
             // Check if closer to start (0) or end (sweep)
             float dist_to_start = 360.0f - relative_angle;
             float dist_to_end = relative_angle - sweep_angle_;
             
             if (dist_to_start < dist_to_end) {
                 t = 0.0f;
             } else {
                 t = 1.0f;
             }
        } else {
             t = relative_angle / sweep_angle_;
             t = std::clamp(t, 0.0f, 1.0f);
        }

    } else {
        // Linear
        float local_x = (float)mx - x_;
        
        // Calculate raw T (unclamped)
        float raw_t = local_x / width_;
        
        t = std::clamp(raw_t, 0.0f, 1.0f);
        
        // Calculate overshoot pixels with a hard limit
        const float max_stretch = 50.0f;
        
        // If raw_t < 0: overshoot is negative (left)
        // If raw_t > 1: overshoot is positive (right)
        if (raw_t < 0.0f) {
            drag_overshoot_ = std::clamp(local_x, -max_stretch, 0.0f); 
        } else if (raw_t > 1.0f) {
            drag_overshoot_ = std::clamp(local_x - width_, 0.0f, max_stretch);
        } else {
            drag_overshoot_ = 0.0f;
        }
    }

    float new_val = min_ + t * (max_ - min_);
    set_value(new_val);
}

void Slider::draw(palladium::GPUSurface& surface) {
    if (shape_ == SliderShape::Selector) {
        // Selector Draw Logic
        // Background
        // surface.fill_rect(x_, y_, width_, height_, bg_color_); // Optional explicit bg
        
        // 1. Draw Large Value
        std::stringstream ss;
        ss << std::fixed << std::setprecision(0) << current_value_display_ << "x";
        std::string val_txt = ss.str();
        float font_size = 48.0f;
        float tx = x_ + width_ / 2.0f - (val_txt.length() * font_size * 0.25f); // Centered
        float ty = y_ + 10.0f; // Top margin
        surface.draw_text(val_txt, tx, ty, "Roboto Bold", font_size, text_color_);
        
        // 2. Draw Indicator (Triangle)
        float cx = x_ + width_ / 2.0f;
        float indicator_y = ty + font_size + 10.0f;
        // Simple triangle using lines
        surface.draw_line((int)cx - 10, (int)indicator_y, (int)cx, (int)indicator_y + 10, nativeui::Color(255, 255, 255, 255), 2.0f);
        surface.draw_line((int)cx, (int)indicator_y + 10, (int)cx + 10, (int)indicator_y, nativeui::Color(255, 255, 255, 255), 2.0f);
        // Vertical line down
        surface.draw_line((int)cx, (int)indicator_y + 10, (int)cx, (int)y_ + height_ - 20, nativeui::Color(255, 255, 255, 255), 2.0f);

        // 3. Draw Tape
        float tape_y = indicator_y + 15.0f;
        float tape_h = height_ - (tape_y - y_);
        
        // Clip Tape area
        // Clip Tape area (Extend height to include labels)
        bool use_clip = (tape_h > 1.0f && width_ > 1.0f);
        if (use_clip) {
             surface.push_axis_aligned_clip((int)x_, (int)tape_y, (int)width_, (int)tape_h + 30);
        }
        
        // Include overshoot in visual position
        float scale = pixels_per_segment_ * current_zoom_;
        float v_center = value_to_visual(current_value_display_) + current_overshoot_ / scale;
        float visible_v_range = width_ / scale;
        
        // Dynamic Ticks
        float step_density = 10.0f;
        // Keep high density while zoomed (even while animating out)
        if (fine_control_enabled_ || current_zoom_ > 1.01f) step_density = 50.0f; 
        
        int v_start_idx = (int)std::floor((v_center - visible_v_range * 0.6f) * step_density);
        int v_end_idx = (int)std::ceil((v_center + visible_v_range * 0.6f) * step_density);
        
        // Smoother Sub-tick Fade (Linear over range 1.0 -> 4.0)
        float sub_tick_alpha = std::clamp((current_zoom_ - 1.0f) / 3.0f, 0.0f, 1.0f);
        // Minor Label Fade (Start at 2.0 -> 4.0)
        float minor_label_alpha = std::clamp((current_zoom_ - 2.0f) / 2.0f, 0.0f, 1.0f);
        
        for (int i = v_start_idx; i <= v_end_idx; ++i) {
            float v = i / step_density;
            float px = cx + (v - v_center) * scale;
            
            // Determine Tick Type
            float v_10 = v * 10.0f;
            float v_1 = v;
            bool is_major = (std::abs(v_1 - std::round(v_1)) < 0.005f);
            bool is_minor = (std::abs(v_10 - std::round(v_10)) < 0.005f);
            bool is_sub = !is_major && !is_minor;
            
            if (is_sub && sub_tick_alpha <= 0.01f) continue;

            float base_h = is_major ? 20.0f : (is_minor ? 10.0f : 6.0f);
            
            nativeui::Color tick_col = text_color_;
            float dist = std::abs(px - cx);
            
            // Continuous Fade
            // Fade out completely before hitting the edge (0.4w instead of 0.5w)
            float alpha = 1.0f - std::clamp(dist / (width_ * 0.4f), 0.0f, 1.0f);
            alpha = alpha * alpha; // Quadratic falloff 
            
            if (is_sub) alpha *= sub_tick_alpha;

            tick_col.a = (uint8_t)(text_color_.a * alpha); 
            
            if (alpha > 0.01f) {
                // Dynamic Height Boost
                float proximity = 1.0f - std::clamp((dist / (width_ * 0.15f)), 0.0f, 1.0f);
                float height_boost = 15.0f * (proximity * proximity); 
                float final_h = base_h + height_boost * (is_sub ? 0.3f : 1.0f);
                
                float tape_center_y = tape_y + 20.0f;
                float y1 = tape_center_y - final_h / 2.0f;
                float y2 = tape_center_y + final_h / 2.0f;

                float thickness = is_major ? 3.0f : (is_minor ? 2.0f : 1.0f);
                
                surface.draw_line((int)px, (int)y1, (int)px, (int)y2, tick_col, thickness);
                
                // Draw Labels (Major + Minor if zoomed)
                if (is_major || (is_minor && minor_label_alpha > 0.05f)) {
                    float val_at_tick = visual_to_value(v);
                    std::stringstream ss_lbl;
                    ss_lbl << std::fixed << std::setprecision(0) << val_at_tick << "x";
                    std::string lbl = ss_lbl.str();
                    
                    float lbl_size = is_major ? 14.0f : 12.0f;
                    nativeui::Color lbl_col = tick_col;
                    if (is_minor && !is_major) lbl_col.a = (uint8_t)(lbl_col.a * minor_label_alpha);
                    
                    if (lbl_col.a > 10) {
                        surface.draw_text(lbl, px - (lbl.length() * lbl_size * 0.3f), y2 + 5, "Roboto Bold", lbl_size, lbl_col);
                    }
                }
            }
        }
        
        if (use_clip) surface.pop_clip();
        return;
    }

    float t = (current_value_display_ - min_) / (max_ - min_);
    t = std::clamp(t, 0.0f, 1.0f);

    if (shape_ == SliderShape::Arc) {
        // Draw Track
        surface.draw_arc(x_, y_, width_, start_angle_, sweep_angle_, bg_color_, current_thickness_, true);
        
        // Draw Fill
        if (t > 0.001f) {
            surface.draw_arc(x_, y_, width_, start_angle_, sweep_angle_ * t, fill_color_, current_thickness_, true);
        }
        
    } else { // Rect or Pill
        // Apply elasticity calculations
        // Effect: Stretch length, thin out thickness
        
        // Damping factor for stretch visual
        float stretch_amount = current_overshoot_ * 0.5f; // Visual stretch is half of mouse overshoot
        
        // Adjust geometry
        float draw_x = x_;
        float draw_width = width_;
        
        if (stretch_amount < 0.0f) { // Stretching Left
            // Offset X to the left, increase width
            draw_x += stretch_amount; // shifts left (negative)
            draw_width -= stretch_amount; // increases width (makes positive adds to width)
        } else { // Stretching Right
            // X stays same, width increases
            draw_width += stretch_amount;
        }
        
        // Squash thickness based on stretch
        // Conservation of volume roughly: w * h = new_w * new_h
        // new_h = (w * h) / new_w
        // But let's limit the squash so it doesn't disappear
        float original_area = width_ * current_thickness_;
        float new_thickness = original_area / draw_width;
        new_thickness = std::clamp(new_thickness, current_thickness_ * 0.4f, current_thickness_); // max squash 40%
        
        // Center Y with new thickness
        float draw_y = y_ + height_ / 2.0f - new_thickness / 2.0f;
        
        if (shape_ == SliderShape::Pill) {
            // Background
            surface.fill_rounded_rect(draw_x, draw_y, draw_width, new_thickness, new_thickness / 2.0f, bg_color_);
            
            // Fill
            float fill_width = draw_width * t;
            if (t >= 0.99f && stretch_amount > 0) {
                 fill_width = draw_width; 
            }
            // If stretching left (min), fill is 0.
            
            if (fill_width > 0.001f) {
                surface.fill_rounded_rect(draw_x, draw_y, fill_width, new_thickness, new_thickness / 2.0f, fill_color_);
            }
            
        } else { // Rectangle
            // Background
            surface.fill_rect(draw_x, draw_y, draw_width, new_thickness, bg_color_);
            
            // Fill
            float fill_width = draw_width * t;
            if (t >= 0.99f && stretch_amount > 0) fill_width = draw_width;
            
            if (fill_width > 0.001f) {
                surface.fill_rect(draw_x, draw_y, fill_width, new_thickness, fill_color_);
            }
        }
        
        // Unified Value Text Display
        if (show_value_ && (is_hovered_ || is_dragging_)) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(0) << current_value_display_;
            std::string txt = ss.str();
            float font_size = 14.0f;
            
            float tx, ty;

            if (shape_ == SliderShape::Arc) {
                // Arc is handled separately below or doesn't support linear clip
                tx = x_ - (txt.length() * font_size * 0.3f); 
                ty = y_ - font_size * 0.5f;
                surface.draw_text(txt, tx, ty, "Roboto Bold", font_size, text_color_);
            } else { // Rect / Pill
                // Recalculate Geometry (Scope is outside specific shape blocks)
                float stretch_amount = current_overshoot_ * 0.5f;
                float dx = x_;
                float dw = width_;
                
                if (stretch_amount < 0.0f) {
                    dx += stretch_amount;
                    dw -= stretch_amount;
                } else {
                    dw += stretch_amount;
                }
                
                float original_area = width_ * current_thickness_;
                float d_thick = original_area / dw;
                d_thick = std::clamp(d_thick, current_thickness_ * 0.4f, current_thickness_);
                float dy = y_ + height_ / 2.0f - d_thick / 2.0f;

                tx = dx + dw / 2.0f - (txt.length() * font_size * 0.3f);
                ty = y_ + height_ / 2.0f - font_size * 0.5f;
                
                // Difference Mask Logic
                // 1. Draw Text for Empty Area (using fill_color_ or dimmed text)
                // Appears on the gray background. Using fill_color_ approximates "inverted" look.
                surface.draw_text(txt, tx, ty, "Roboto Bold", font_size, fill_color_); 
                
                // 2. Draw Text for Filled Area (using text_color_)
                float fill_w = dw * t;
                if (t >= 0.99f && stretch_amount > 0) fill_w = dw;
                
                // Only clip if there is a fill to clip to
                if (fill_w > 1.0f) {
                   if (shape_ == SliderShape::Pill) {
                        surface.push_rounded_clip((int)dx, (int)dy, (int)fill_w, (int)d_thick, d_thick * 0.5f);
                   } else {
                        surface.push_axis_aligned_clip((int)dx, (int)dy, (int)fill_w, (int)d_thick);
                   }
                   
                   // Inner text (White/Text Color) on Top of Fill
                   surface.draw_text(txt, tx, ty, "Roboto Bold", font_size, text_color_);
                   
                   if (shape_ == SliderShape::Pill) {
                        surface.pop_rounded_clip();
                   } else {
                        surface.pop_clip();
                   }
                }
            }
        }
    }
}


// CPU Draw Implementation
void Slider::draw(nativeui::Surface& surface) {
    if (shape_ == SliderShape::Selector) {
        // Selector 
        // 1. Large Value Text
        std::stringstream ss;
        ss << std::fixed << std::setprecision(0) << current_value_display_ << "x";
        std::string val_txt = ss.str();
        
        int text_size = 48;
        auto font_large = nativeui::FontCache::get("Roboto Bold", text_size);
        int ty_top = (int)y_ + 10;
        float cx = x_ + width_ / 2.0f;

        if (font_large) {
            auto text_surf = font_large->render(val_txt, text_color_);
            if (text_surf) {
                surface.blit_alpha(*text_surf, (int)(cx - text_surf->get_width()/2), ty_top);
            }
        }
        
        // 2. Indicator
        int ind_y = ty_top + text_size + 10;
        surface.draw_line((int)cx - 10, ind_y, (int)cx, ind_y + 10, nativeui::Color(255, 255, 255, 255));
        surface.draw_line((int)cx, ind_y + 10, (int)cx + 10, ind_y, nativeui::Color(255, 255, 255, 255));
        surface.draw_line((int)cx, ind_y + 10, (int)cx, (int)y_ + (int)height_ - 20, nativeui::Color(255, 255, 255, 255));

        // 3. Tape
        float tape_y = (float)ind_y + 15.0f;
        
        float scale = pixels_per_segment_ * current_zoom_;
        float v_center = value_to_visual(current_value_display_) + current_overshoot_ / scale;
        float visible_v_range = width_ / scale;
        
        // Dynamic Ticks
        float step_density = 10.0f;
        // Keep high density while zoomed (even while animating out)
        if (fine_control_enabled_ || current_zoom_ > 1.01f) step_density = 50.0f; 
        
        int v_start_idx = (int)std::floor((v_center - visible_v_range * 0.6f) * step_density);
        int v_end_idx = (int)std::ceil((v_center + visible_v_range * 0.6f) * step_density);
        
        // Smoother Sub-tick Fade (Linear over range 1.0 -> 4.0)
        float sub_tick_alpha = std::clamp((current_zoom_ - 1.0f) / 3.0f, 0.0f, 1.0f);
        // Minor Label Fade (Start at 2.0 -> 4.0)
        float minor_label_alpha = std::clamp((current_zoom_ - 2.0f) / 2.0f, 0.0f, 1.0f);

        auto font_major = nativeui::FontCache::get("Roboto Bold", 14);
        auto font_minor = nativeui::FontCache::get("Roboto Bold", 12);

        for (int i = v_start_idx; i <= v_end_idx; ++i) {
            float v = i / step_density;
            float px = cx + (v - v_center) * scale;
            
            // Manual Clip
            if (px < x_ || px > x_ + width_) continue;
            
             // Determine Tick Type
            float v_10 = v * 10.0f;
            float v_1 = v;
            bool is_major = (std::abs(v_1 - std::round(v_1)) < 0.005f);
            bool is_minor = (std::abs(v_10 - std::round(v_10)) < 0.005f);
            bool is_sub = !is_major && !is_minor;
            
            if (is_sub && sub_tick_alpha <= 0.01f) continue;
            
            float base_h = is_major ? 20.0f : (is_minor ? 10.0f : 6.0f);
            
            nativeui::Color tick_col = text_color_;
            float dist = std::abs(px - cx);
            
            // Continuous Fade (CPU)
            // Fade out completely before hitting the edge (0.4w instead of 0.5w)
            float alpha = 1.0f - std::clamp(dist / (width_ * 0.4f), 0.0f, 1.0f);
            alpha = alpha * alpha; 
            
            if (is_sub) alpha *= sub_tick_alpha;
            
            tick_col.a = (uint8_t)(text_color_.a * alpha);
            
             if (alpha > 0.01f) {
                // Dynamic Height Boost
                float proximity = 1.0f - std::clamp((dist / (width_ * 0.15f)), 0.0f, 1.0f);
                float height_boost = 15.0f * (proximity * proximity); 
                float final_h = base_h + height_boost * (is_sub ? 0.3f : 1.0f);
                
                float tape_center_y = tape_y + 20.0f;
                float y1 = tape_center_y - final_h / 2.0f;
                
                int thickness = is_major ? 3 : (is_minor ? 2 : 1);
                // Use fill_rect to simulate thickness on CPU
                surface.fill_rect((int)(px - thickness/2.0f), (int)y1, thickness, (int)final_h, tick_col);
                
                // Draw Labels
                if (is_major || (is_minor && minor_label_alpha > 0.05f)) {
                    float val_at_tick = visual_to_value(v);
                    std::stringstream ss_lbl;
                    ss_lbl << std::fixed << std::setprecision(0) << val_at_tick << "x";
                    
                    nativeui::Color lbl_col = tick_col;
                    auto font = font_major;
                    if (is_minor && !is_major) {
                         lbl_col.a = (uint8_t)(lbl_col.a * minor_label_alpha);
                         font = font_minor;
                    }
                    
                    if (font && lbl_col.a > 10) {
                        auto lbl_surf = font->render(ss_lbl.str(), lbl_col);
                        if (lbl_surf) {
                             surface.blit_alpha(*lbl_surf, (int)px - lbl_surf->get_width()/2, (int)(tape_center_y + final_h/2.0f + 5));
                        }
                    }
                }
             }
        }
        return;
    }

    float t = (current_value_display_ - min_) / (max_ - min_);
    t = std::clamp(t, 0.0f, 1.0f);

    if (shape_ == SliderShape::Arc) {
        // CPU Arc Approximation (Brush method)
        // Step size based on radius to Ensure smooth curve
        float step_deg = 1.0f; // 1 degree per step is usually fine
        if (width_ > 100) step_deg = 0.5f;

        // Draw Track
        for (float a = 0.0f; a <= sweep_angle_; a += step_deg) {
            float rad = (start_angle_ + a) * (M_PI / 180.0f);
            float px = x_ + width_ * std::cos(rad);
            float py = y_ + width_ * std::sin(rad);
            surface.fill_circle_aa(px, py, current_thickness_ / 2.0f, bg_color_);
        }

        // Draw Fill
        if (t > 0.001f) {
            float fill_sweep = sweep_angle_ * t;
            for (float a = 0.0f; a <= fill_sweep; a += step_deg) {
                float rad = (start_angle_ + a) * (M_PI / 180.0f);
                float px = x_ + width_ * std::cos(rad);
                float py = y_ + width_ * std::sin(rad);
                surface.fill_circle_aa(px, py, current_thickness_ / 2.0f, fill_color_);
            }
        }

    } else { // Rect or Pill
        // Apply elasticity calculations (Duplicated from GPU draw)
        float stretch_amount = current_overshoot_ * 0.5f;
        float draw_x = x_;
        float draw_width = width_;
        
        if (stretch_amount < 0.0f) {
            draw_x += stretch_amount;
            draw_width -= stretch_amount;
        } else {
            draw_width += stretch_amount;
        }
        
        float original_area = width_ * current_thickness_;
        float new_thickness = original_area / draw_width;
        new_thickness = std::clamp(new_thickness, current_thickness_ * 0.4f, current_thickness_);
        float draw_y = y_ + height_ / 2.0f - new_thickness / 2.0f;
        
        float fill_width = draw_width * t;
        if (t >= 0.99f && stretch_amount > 0) fill_width = draw_width;

        if (shape_ == SliderShape::Pill) {
            surface.fill_pill(draw_x, draw_y, draw_width, new_thickness, bg_color_);
            if (fill_width > 0.001f) {
                surface.fill_pill(draw_x, draw_y, fill_width, new_thickness, fill_color_);
            }
        } else { // Rectangle
            surface.fill_rect(draw_x, draw_y, draw_width, new_thickness, bg_color_);
            if (fill_width > 0.001f) {
                surface.fill_rect(draw_x, draw_y, fill_width, new_thickness, fill_color_);
            }
        }
        
        // CPU Text rendering (Linear Split)
        if (show_value_ && (is_hovered_ || is_dragging_)) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(0) << current_value_display_;
            std::string txt = ss.str();
            
            int font_size = 14;
            auto font = nativeui::FontCache::get("Roboto Bold", font_size);
            if (font) {
                auto surf_u = font->render(txt, fill_color_); // Unfilled (Primary)
                auto surf_f = font->render(txt, bg_color_);   // Filled (Secondary)
                
                if (surf_u && surf_f) {
                    float tx = draw_x + draw_width / 2.0f - (surf_u->get_width() * 0.5f);
                    float ty = y_ + height_ / 2.0f - (surf_u->get_height() * 0.5f);
                    
                    float fill_end_x = draw_x + fill_width;
                    float text_x = tx;
                    float text_w = (float)surf_u->get_width();
                    float text_h = (float)surf_u->get_height();
                    
                    if (fill_end_x >= text_x + text_w) {
                         surface.blit_alpha(*surf_f, (int)tx, (int)ty);
                    } else if (fill_end_x <= text_x) {
                         surface.blit_alpha(*surf_u, (int)tx, (int)ty);
                    } else {
                        int split = (int)(fill_end_x - text_x);
                        if (split > 0) {
                            auto left_sub = surf_f->subsurface(0, 0, split, (int)text_h);
                            if (left_sub) surface.blit_alpha(*left_sub, (int)tx, (int)ty);
                        }
                        if (split < text_w) {
                            auto right_sub = surf_u->subsurface(split, 0, (int)text_w - split, (int)text_h);
                            if (right_sub) surface.blit_alpha(*right_sub, (int)tx + split, (int)ty);
                        }
                    }
                }
            }
        }
    }
    
    // CPU Text rendering (Fallback/Arc)
    if (show_value_ && (is_hovered_ || is_dragging_)) {
        if (shape_ == SliderShape::Arc) {
             std::stringstream ss;
            ss << std::fixed << std::setprecision(0) << current_value_display_;
            std::string txt = ss.str();
            
            int font_size = 14;
            auto font = nativeui::FontCache::get("Roboto Bold", font_size);
            if (font) {
                auto text_surf = font->render(txt, text_color_);
                if (text_surf) {
                    float tx = x_ - (text_surf->get_width() * 0.5f); 
                    float ty = y_ - (text_surf->get_height() * 0.5f);
                    surface.blit_alpha(*text_surf, (int)tx, (int)ty);
                }
            }
        }
    }
} 

void Slider::set_exponential_stops(const std::vector<float>& stops) {
    stops_ = stops;
}

float Slider::value_to_visual(float val) const {
    if (stops_.empty()) return (val - min_) / (max_ - min_); // Fallback linear
    
    // Find segment
    for (size_t i = 0; i < stops_.size() - 1; ++i) {
        if (val >= stops_[i] && val <= stops_[i+1]) {
            float t = (val - stops_[i]) / (stops_[i+1] - stops_[i]);
            return (float)i + t; // Visual units
        }
    }
    // Out of bounds
    if (val < stops_.front()) return 0.0f;
    if (val > stops_.back()) return (float)(stops_.size() - 1);
    return 0.0f;
}

float Slider::visual_to_value(float v) const {
    if (stops_.empty()) return min_ + v * (max_ - min_); // Fallback linear bound to 0-1
    
    v = std::clamp(v, 0.0f, (float)(stops_.size() - 1));
    int i = (int)v;
    if (i >= stops_.size() - 1) i = stops_.size() - 2;
    
    float t = v - (float)i;
    float start = stops_[i];
    float end = stops_[i+1];
    return start + t * (end - start);
}

} // namespace nativeui
