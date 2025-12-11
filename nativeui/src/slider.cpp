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
    // Spring physics constants
    const float tension = 150.0f;
    const float friction = 25.0f; // Increased to ~critical damping (2 * sqrt(150) ~= 24.5) to prevent overshoot

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
}

void Slider::handle_event(const nativeui::Event& event) {
    if (event.type == nativeui::EventType::MouseMotion) {
        if (is_dragging_) {
            update_value_from_mouse(event.mouse_x, event.mouse_y);
        } else {
            is_hovered_ = hit_test(event.mouse_x, event.mouse_y);
        }
    } else if (event.type == nativeui::EventType::MouseButtonDown && event.mouse_button == 1) { // Left click
        if (hit_test(event.mouse_x, event.mouse_y)) {
            is_dragging_ = true;
            update_value_from_mouse(event.mouse_x, event.mouse_y);
        }
    } else if (event.type == nativeui::EventType::MouseButtonUp && event.mouse_button == 1) {
        if (is_dragging_) {
            is_dragging_ = false;
            // Overshoot will animate back to 0 in update()
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
        
        // Linear Text Inversion Logic
        if (show_value_ && (is_hovered_ || is_dragging_)) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(0) << current_value_display_;
            std::string txt = ss.str();
            float font_size = 14.0f;
            
            // Calculate Text Position
            float stretch = current_overshoot_ * 0.5f;
            float dx = x_;
            float dw = width_;
            if (stretch < 0) { dx += stretch; dw -= stretch; }
            else { dw += stretch; }
            float tx = dx + dw / 2.0f - (txt.length() * font_size * 0.3f);
            float ty = y_ + height_ / 2.0f - font_size * 0.5f;
            
            // 1. Draw Text in Fill Color (Primary) - appears over background
            surface.draw_text(txt, tx, ty, "Roboto Bold", font_size, fill_color_);
            
            // 2. Re-draw fill logic to get the clip rect
            // Note: We already drew the fill above. 
            // We need to define the clip rect for the "filled" part.
            float fill_width = draw_width * t;
             if (t >= 0.99f && stretch_amount > 0) fill_width = draw_width;
             
            if (fill_width > 1.0f) {
                if (shape_ == SliderShape::Pill) {
                    surface.push_rounded_clip((int)draw_x, (int)draw_y, (int)fill_width, (int)new_thickness, new_thickness * 0.5f);
                } else {
                    surface.push_axis_aligned_clip((int)draw_x, (int)draw_y, (int)fill_width, (int)new_thickness);
                }
                
                // 3. Draw Text in Background Color (Secondary) - appears over fill
                surface.draw_text(txt, tx, ty, "Roboto Bold", font_size, bg_color_);
                
                if (shape_ == SliderShape::Pill) {
                     surface.pop_rounded_clip();
                } else {
                     surface.pop_clip();
                }
            }
        }
        return; // Skip default text drawing for linear
    }

    // Text Display (Fallback for Arc)
    if (show_value_ && (is_hovered_ || is_dragging_)) {
        // Construct string
        std::stringstream ss;
        ss << std::fixed << std::setprecision(0) << current_value_display_;
        std::string txt = ss.str();
        
        float font_size = 14.0f;
        // Position
        float tx, ty;
        
        if (shape_ == SliderShape::Arc) {
            tx = x_ - (txt.length() * font_size * 0.3f); // Approx centered
            ty = y_ - font_size * 0.5f;
            // Draw Text (Simple contrast: always white or text_color)
             surface.draw_text(txt, tx, ty, "Roboto Bold", font_size, text_color_);
        }
    }
}


// CPU Draw Implementation
void Slider::draw(nativeui::Surface& surface) {
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

} // namespace nativeui
