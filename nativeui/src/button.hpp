#pragma once

#include "layer.hpp"
#include "animation.hpp"
#include "window.hpp" // For Event
#include <functional>
#include <map>
#include <string>

namespace nativeui {

enum class ButtonAnimType {
    Instant,
    Linear,
    Exponential
};

enum class ButtonShape {
    Rectangle,
    RoundedRect,
    Circle,
    Pill,
    Squircle
};

struct ButtonStyle {
    Color color;
    float opacity = 1.0f;
    float scale = 1.0f;
    float blur_radius = 0.0f; // For frosted glass
    
    // Default constructor
    ButtonStyle() : color(200, 200, 200), opacity(1.0f), scale(1.0f), blur_radius(0.0f) {}
    
    // Helper to interpolate
    static ButtonStyle lerp(const ButtonStyle& a, const ButtonStyle& b, float t);
};

struct ButtonTextStyle {
    std::string text = "";
    std::string font_name = "Roboto";
    int font_size = 14;
    Color color = Color(255, 255, 255, 255);
    std::string position = "center"; // 9-point alignment
    int padding_v = 0;
    int padding_h = 0;
    float rotation = 0.0f;
    
    bool has_text() const { return !text.empty(); }
};

class Button : public Layer {
public:
    Button(int width, int height, ButtonShape shape = ButtonShape::RoundedRect, int radius = 10);
    
    // State management
    void set_style(const std::string& state, const ButtonStyle& style);
    const ButtonStyle& get_style(const std::string& state) const;
    
    // Text management
    void set_text_style(const ButtonTextStyle& style);
    
    // Defaults
    void set_normal_style(const ButtonStyle& style) { set_style("normal", style); }
    void set_hover_style(const ButtonStyle& style) { set_style("hover", style); }
    void set_pressed_style(const ButtonStyle& style) { set_style("pressed", style); }
    
    // Animation settings
    void set_hover_animation(ButtonAnimType type, float duration = 0.1f);
    void set_click_animation(ButtonAnimType type, float duration = 0.05f);
    
    // Interaction
    void process_event(const Event& event);
    void update(float dt);
    
    // Callback
    std::function<void()> on_click;
    
    // Redraw
    void redraw();
    
private:
    ButtonShape shape_;
    int radius_; // Corner radius
    
    // State
    bool is_hovered_;
    bool is_pressed_;
    
    std::map<std::string, ButtonStyle> styles_;
    ButtonTextStyle text_style_;
    
    // Current animated state
    ButtonStyle current_style_;
    ButtonStyle target_style_;
    
    // Animation
    Animation transition_anim_;
    ButtonAnimType anim_type_;
    
    void update_target_style();
    void apply_style(const ButtonStyle& style);
    void draw_text(Surface& surface);
};

} // namespace nativeui
