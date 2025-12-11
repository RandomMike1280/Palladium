#pragma once

#include "surface.hpp"
#include "gpu_surface.hpp" // For drawing
#include "window.hpp" // For events
#include <functional>
#include <string>

namespace palladium { class GPUSurface; }

namespace nativeui {

enum class SliderShape {
    Rectangle,
    Pill,
    Arc
};

class Slider {
public:
    Slider(SliderShape shape = SliderShape::Pill);
    ~Slider() = default;

    // Core Properties
    void set_range(float min, float max);
    void set_value(float value);
    float get_value() const { return value_; }
    
    void set_shape(SliderShape shape);
    SliderShape get_shape() const { return shape_; }
    
    // Position & Dimensions
    // For Linear: width = length, height = thickness
    // For Arc: width = radius, height = thickness (and sweep angle)
    void set_position(float x, float y);
    void set_dimensions(float width, float height); // Width/Length, Height/Thickness
    void set_arc_angles(float start, float sweep); // For Arc shape
    
    // Colors
    void set_colors(const nativeui::Color& bg, const nativeui::Color& fill, const nativeui::Color& text);
    
    // Config
    void set_show_value(bool show) { show_value_ = show; }
    
    // Callbacks
    void on_change(std::function<void(float)> callback) { on_change_ = callback; }

    // Update / Draw
    void update(float dt);
    void handle_event(const nativeui::Event& event);
    void draw(palladium::GPUSurface& surface);
    void draw(nativeui::Surface& surface); // CPU Draw support
    
    // Accessors
    float get_x() const { return x_; }
    float get_y() const { return y_; }
    float get_width() const { return width_; }
    float get_height() const { return height_; }
    
private:
    // Config
    SliderShape shape_;
    float min_ = 0.0f;
    float max_ = 100.0f;
    float value_ = 0.0f;
    bool show_value_ = true;
    
    // Layout
    float x_ = 0.0f;
    float y_ = 0.0f;
    float width_ = 200.0f; // Length or Radius
    float height_ = 10.0f; // Thickness
    
    // Arc Specific
    float start_angle_ = 0.0f;
    float sweep_angle_ = 360.0f; // Default full circle
    
    // Colors
    nativeui::Color bg_color_ = nativeui::Color(50, 50, 50, 255);
    nativeui::Color fill_color_ = nativeui::Color(0, 120, 215, 255);
    nativeui::Color text_color_ = nativeui::Color(255, 255, 255, 255);
    
    // State
    bool is_hovered_ = false;
    bool is_dragging_ = false;
    
    // Anim state
    float current_value_display_ = 0.0f; // Smoothed value
    float current_thickness_ = 10.0f;   // Smoothed thickness
    
    // Animation velocities
    // Animation velocities
    float value_velocity_ = 0.0f;
    float thickness_velocity_ = 0.0f;
    
    // Elasticity
    float drag_overshoot_ = 0.0f; // Target overshoot (from mouse)
    float current_overshoot_ = 0.0f; // Animated overshoot
    float overshoot_velocity_ = 0.0f;
    
    // Callback
    std::function<void(float)> on_change_;
    
    // Helpers
    void update_value_from_mouse(int mx, int my);
    bool hit_test(int mx, int my) const;
};

} // namespace nativeui
