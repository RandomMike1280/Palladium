#include "button.hpp"
#include "font.hpp"
#include <iostream>
#include <cmath> // For std::exp
#include <algorithm> // For min/max

namespace nativeui {

ButtonStyle ButtonStyle::lerp(const ButtonStyle& a, const ButtonStyle& b, float t) {
    ButtonStyle result;
    result.color = Color(
        static_cast<uint8_t>(a.color.r + (b.color.r - a.color.r) * t),
        static_cast<uint8_t>(a.color.g + (b.color.g - a.color.g) * t),
        static_cast<uint8_t>(a.color.b + (b.color.b - a.color.b) * t),
        static_cast<uint8_t>(a.color.a + (b.color.a - a.color.a) * t)
    );
    result.opacity = a.opacity + (b.opacity - a.opacity) * t;
    result.scale = a.scale + (b.scale - a.scale) * t;
    result.blur_radius = a.blur_radius + (b.blur_radius - a.blur_radius) * t;
    return result;
}

Button::Button(int width, int height, ButtonShape shape, int radius)
    : Layer(width, height)
    , shape_(shape)
    , radius_(radius)
    , is_hovered_(false)
    , is_pressed_(false)
    , transition_anim_(0.0f, 1.0f, 0.1f)
    , anim_type_(ButtonAnimType::Linear)
{
    // Defaults
    styles_["normal"] = ButtonStyle();
    styles_["hover"] = ButtonStyle(); styles_["hover"].color = Color(220, 220, 220);
    styles_["pressed"] = ButtonStyle(); styles_["pressed"].color = Color(150, 150, 150);
    
    current_style_ = styles_["normal"];
    target_style_ = styles_["normal"];
    
    redraw();
}

void Button::set_style(const std::string& state, const ButtonStyle& style) {
    styles_[state] = style;
    if (state == "normal" && !is_hovered_ && !is_pressed_) {
        current_style_ = style; // Snap if current
        update_target_style();
        redraw();
    }
}

const ButtonStyle& Button::get_style(const std::string& state) const {
    auto it = styles_.find(state);
    if (it != styles_.end()) return it->second;
    static ButtonStyle default_style;
    return default_style;
}

void Button::set_hover_animation(ButtonAnimType type, float duration) {
    anim_type_ = type; // Shared for now
    transition_anim_.set_duration(duration);
}

void Button::set_click_animation(ButtonAnimType type, float duration) {
    // Separate logic needed for click duration vs hover?
    // For now simple implementation: just shared
}

void Button::process_event(const Event& event) {
    if (event.type == EventType::MouseMotion) {
        bool hit = hit_test(event.mouse_x, event.mouse_y);
        if (hit != is_hovered_) {
            is_hovered_ = hit;
            update_target_style();
        }
    } else if (event.type == EventType::MouseButtonDown) {
        if (is_hovered_) {
            is_pressed_ = true;
            update_target_style();
        }
    } else if (event.type == EventType::MouseButtonUp) {
        if (is_pressed_ && is_hovered_) {
            if (on_click) on_click();
        }
        if (is_pressed_) {
            is_pressed_ = false;
            update_target_style();
        }
    }
}

void Button::update_target_style() {
    if (is_pressed_) {
        if (styles_.count("pressed")) target_style_ = styles_["pressed"];
        else target_style_ = styles_["hover"]; // Fallback
    } else if (is_hovered_) {
        target_style_ = styles_["hover"];
    } else {
        target_style_ = styles_["normal"];
    }
    
    // Start animation
    if (anim_type_ == ButtonAnimType::Instant) {
        current_style_ = target_style_;
        redraw();
    } else {
        transition_anim_.reset();
        transition_anim_.set_start_value(0.0f);
        transition_anim_.set_end_value(1.0f);
        transition_anim_.restart();
    }
}

void Button::update(float dt) {
    if (anim_type_ != ButtonAnimType::Instant && transition_anim_.is_running()) {
        transition_anim_.update(dt);
        float t = transition_anim_.get_value();
        
        // We need to lerp from PREVIOUS state to TARGET state.
        // Issue: 'current_style_' is updated continuously.
        // Better: Store 'start_interp_style_' when animation starts.
        // For simplicity: We simulate simple exponential ease towards target each frame?
        // Or standard lerp: Current = Lerp(Current, Target, dt * speed).
        // Let's use the latter for smooth transitions without complex state tracking.
        
        float speed = 10.0f; // Approx 
        if (anim_type_ == ButtonAnimType::Linear) speed = 5.0f;
        if (anim_type_ == ButtonAnimType::Exponential) speed = 15.0f;
        
        // Manual dt-based lerp
        float alpha = 1.0f - std::exp(-speed * dt); // frame independent lerp factor
        current_style_ = ButtonStyle::lerp(current_style_, target_style_, alpha);
        
        redraw();
        
        // Stop if close
        // (Optional optimization)
    }
}

void Button::set_text_style(const ButtonTextStyle& style) {
    text_style_ = style;
    redraw();
}

void Button::draw_text(Surface& s) {
    if (!text_style_.has_text()) return;

    // Load font
    auto font = FontCache::get(text_style_.font_name, text_style_.font_size);
    if (!font) return;

    // Render text
    // Note: This renders new surface every frame -> Inefficient for static text.
    // Optimization: Cache rendered text surface if style hasn't changed.
    // For now: Direct rendering.
    auto text_surf = font->render(text_style_.text, text_style_.color);
    if (!text_surf) return;

    // Calculate Position
    int btn_w = s.get_width();
    int btn_h = s.get_height();
    int txt_w = text_surf->get_width();
    int txt_h = text_surf->get_height();

    int x = 0;
    int y = 0;
    
    // Parse position
    std::string pos = text_style_.position;
    int pad_h = text_style_.padding_h;
    int pad_v = text_style_.padding_v;

    // Logic:
    // 9 points: top-left, top-center, top-right, left, center, right, bottom-left, bottom-center, bottom-right
    
    // Horizontal alignment
    if (pos.find("left") != std::string::npos) {
        x = pad_h;
    } else if (pos.find("right") != std::string::npos) {
        x = btn_w - txt_w - pad_h;
    } else { 
        // center (includes "top center", "bottom center", "center")
        x = (btn_w - txt_w) / 2;
    }

    // Vertical alignment
    if (pos.find("top") != std::string::npos) {
        y = pad_v;
    } else if (pos.find("bottom") != std::string::npos) {
        y = btn_h - txt_h - pad_v;
    } else {
        // center (includes "left", "right", "center")
        y = (btn_h - txt_h) / 2;
    }
    
    // Specific exclusions for center alignment as per request:
    // "Vertical padding will be ignored for left and right" -> handled by using center y logic above (ignoring pad_v)
    // "Horizontal padding will be ignored for top center and bottom center" -> handled by using center x logic above
    // "all padding will be ignored for center" -> handled by both
    
    // Blit
    // (If rotation needed, we'd rotate text_surf here)
    // Currently no rotation support in Surface::blit, simple copy
    
    // Simple alpha blending blit
    for (int ty = 0; ty < txt_h; ++ty) {
        for (int tx = 0; tx < txt_w; ++tx) {
            Color c = text_surf->get_pixel(tx, ty);
            if (c.a > 0) {
                // Apply opacity from button style if needed? 
                // Usually text opacity is part of text color.
                // But if button fades out, text should too? 
                // Let's assume text color alpha is master, multiplied by button opacity.
                
                float global_opacity = get_opacity();
                if (global_opacity < 1.0f) {
                     c.a = static_cast<uint8_t>(c.a * global_opacity);
                }
                
                // Blend
                s.blend_pixel(x + tx, y + ty, c);
            }
        }
    }
}

void Button::redraw() {
    Surface& s = get_surface();
    s.clear(); // Clear local surface
    
    // Apply style props to Layer properties
    set_opacity(current_style_.opacity);
    set_scale(current_style_.scale);
    
    // Material
    if (current_style_.blur_radius > 0.0f) {
        set_material(Material::frosted_glass(current_style_.blur_radius));
    } else {
        set_material(Material::solid());
    }
    
    int w = s.get_width();
    int h = s.get_height();
    Color base_color = current_style_.color;
    
    // SDF rendering parameters
    float half_w = w * 0.5f;
    float half_h = h * 0.5f;
    float cx = half_w; 
    float cy = half_h;
    
    // Iterate pixels
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float px = x + 0.5f;
            float py = y + 0.5f;
            float d = 0.0f;
            
            // Centered coordinates
            float rel_x = px - cx;
            float rel_y = py - cy;
            
            switch (shape_) {
                case ButtonShape::Rectangle:
                    // sdBox
                    {
                        float dx = std::abs(rel_x) - half_w;
                        float dy = std::abs(rel_y) - half_h;
                        d = std::min(std::max(dx, dy), 0.0f) + std::sqrt(std::max(dx, 0.0f)*std::max(dx, 0.0f) + std::max(dy, 0.0f)*std::max(dy, 0.0f));
                        // Shift inward by 0.5 for boundary? No, standard SDF is d=0 at edge.
                        // We want coverage where d <= 0.
                        // To account for center alignment exactness: 
                        // Actually box is exact match.
                    }
                    break;
                    
                case ButtonShape::RoundedRect:
                    // sdRoundedBox
                    {
                        // Radius logic: clamp to size
                        float r = std::min(static_cast<float>(radius_), std::min(half_w, half_h));
                        float bx = half_w - r;
                        float by = half_h - r;
                        float dx = std::abs(rel_x) - bx;
                        float dy = std::abs(rel_y) - by;
                        float length_max = std::sqrt(std::max(dx, 0.0f)*std::max(dx, 0.0f) + std::max(dy, 0.0f)*std::max(dy, 0.0f));
                        d = length_max + std::min(std::max(dx, dy), 0.0f) - r;
                    }
                    break;
                    
                case ButtonShape::Circle:
                    // sdCircle
                    {
                        float r = std::min(half_w, half_h);
                        float len = std::sqrt(rel_x*rel_x + rel_y*rel_y);
                        d = len - r;
                    }
                    break;
                    
                case ButtonShape::Pill:
                    // Capsule shape: RoundedRect with full radius
                    {
                        float r = std::min(half_w, half_h);
                        float bx = half_w - r;
                        float by = half_h - r;
                        float dx = std::abs(rel_x) - bx;
                        float dy = std::abs(rel_y) - by;
                        float length_max = std::sqrt(std::max(dx, 0.0f)*std::max(dx, 0.0f) + std::max(dy, 0.0f)*std::max(dy, 0.0f));
                        d = length_max + std::min(std::max(dx, dy), 0.0f) - r;
                    }
                    break;
                    
                case ButtonShape::Squircle:
                    // Approximate superellipse x^4 + y^4 = 1
                    // SDF approximation
                    {
                        // Normalized coords
                        float nx = std::abs(rel_x) / half_w;
                        float ny = std::abs(rel_y) / half_h;
                        float val = nx*nx*nx*nx + ny*ny*ny*ny;
                        
                        // Need distance in pixels. 
                        // Gradients at edge are steep. 
                        // Approx: (val - 1) / |grad|. |grad| ~ 4 at axes.
                        // Let's just use a softer fade for squircle.
                        // d = (val - 1.0f) * (std::min(half_w, half_h) * 0.25f);
                         // Try simplistic conversion to dist
                         d = val - 1.0f;
                         // Scale d to be roughly pixel units at edge
                         // At edge x=half_w, d changes by 4/half_w per pixel
                         d *= (std::min(half_w, half_h) / 4.0f);
                    }
                    break;
            }
            
            // AA
            // dist <= -0.5 -> alpha 1
            // dist >= 0.5 -> alpha 0
            // alpha = 0.5 - d
            float alpha_f = 0.5f - d;
            alpha_f = std::max(0.0f, std::min(1.0f, alpha_f));
            
            if (alpha_f > 0.0f) {
                Color c = base_color;
                c.a = static_cast<uint8_t>(c.a * alpha_f);
                s.set_pixel(x, y, c);
            }
        }
    }
    
    // Draw Text
    draw_text(s);
}

} // namespace nativeui
