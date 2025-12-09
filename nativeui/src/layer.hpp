#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include "surface.hpp"
#include "material.hpp"

namespace nativeui {

/**
 * Blend modes for layer compositing
 */
enum class BlendMode {
    Normal,     // Standard alpha blending
    Multiply,   // Darkens: result = top * bottom
    Screen,     // Lightens: result = 1 - (1-top) * (1-bottom)
    Overlay,    // Combines multiply and screen
    Add,        // result = top + bottom (clamped)
    Subtract,   // result = bottom - top (clamped)
    Difference, // result = |top - bottom|
    ColorDodge, // Brightens dramatically
    ColorBurn   // Darkens dramatically
};

/**
 * Layer - Represents a single layer in the compositing stack
 */
class Layer {
public:
    Layer(int width, int height);
    Layer(std::shared_ptr<Surface> surface);
    
    // Surface access
    Surface& get_surface() { return *surface_; }
    const Surface& get_surface() const { return *surface_; }
    std::shared_ptr<Surface> get_surface_ptr() { return surface_; }
    
    // Position
    int get_x() const { return x_; }
    int get_y() const { return y_; }
    void set_position(int x, int y) { x_ = x; y_ = y; }
    void move(int dx, int dy) { x_ += dx; y_ += dy; }
    
    // Transform
    float get_scale_x() const { return scale_x_; }
    float get_scale_y() const { return scale_y_; }
    void set_scale(float sx, float sy) { scale_x_ = sx; scale_y_ = sy; }
    void set_scale(float s) { scale_x_ = scale_y_ = s; }
    
    float get_rotation() const { return rotation_; }
    void set_rotation(float degrees) { rotation_ = degrees; }
    
    // Opacity and visibility
    float get_opacity() const { return opacity_; }
    void set_opacity(float opacity) { opacity_ = std::clamp(opacity, 0.0f, 1.0f); }
    
    bool is_visible() const { return visible_; }
    void set_visible(bool visible) { visible_ = visible; }
    
    // Blend mode
    BlendMode get_blend_mode() const { return blend_mode_; }
    void set_blend_mode(BlendMode mode) { blend_mode_ = mode; }
    
    // Material
    std::shared_ptr<Material> get_material() const { return material_; }
    void set_material(std::shared_ptr<Material> material) { material_ = material; }
    
    // Name (for debugging/identification)
    // Interaction
    virtual bool hit_test(int x, int y);
    
    const std::string& get_name() const { return name_; }
    void set_name(const std::string& name) { name_ = name; }

private:
    std::shared_ptr<Surface> surface_;
    int x_, y_;
    float scale_x_, scale_y_;
    float rotation_;
    float opacity_;
    bool visible_;
    BlendMode blend_mode_;
    std::shared_ptr<Material> material_;
    std::string name_;
};

/**
 * LayerStack - Manages multiple layers and composites them
 */
class LayerStack {
public:
    LayerStack(int width, int height);
    
    // Layer management
    std::shared_ptr<Layer> create_layer(const std::string& name = "");
    std::shared_ptr<Layer> create_layer_from_surface(std::shared_ptr<Surface> surface, const std::string& name = "");
    void add_layer(std::shared_ptr<Layer> layer);
    void remove_layer(std::shared_ptr<Layer> layer);
    void remove_layer(size_t index);
    void clear_layers();
    
    // Layer access
    std::shared_ptr<Layer> get_layer(size_t index);
    std::shared_ptr<Layer> get_layer_by_name(const std::string& name);
    size_t get_layer_count() const { return layers_.size(); }
    
    // Layer ordering
    void move_layer_up(std::shared_ptr<Layer> layer);
    void move_layer_down(std::shared_ptr<Layer> layer);
    void move_layer_to_top(std::shared_ptr<Layer> layer);
    void move_layer_to_bottom(std::shared_ptr<Layer> layer);
    void set_layer_index(std::shared_ptr<Layer> layer, size_t new_index);
    
    // Compositing
    std::shared_ptr<Surface> composite();
    void composite_to(Surface& dest);
    
    // Background color
    void set_background(const Color& color) { background_ = color; }
    const Color& get_background() const { return background_; }
    
    // Dimensions
    int get_width() const { return width_; }
    int get_height() const { return height_; }

private:
    int width_, height_;
    std::vector<std::shared_ptr<Layer>> layers_;
    Color background_;
    std::shared_ptr<Surface> composite_surface_;
    
    // Blend a single pixel using the specified blend mode
    static Color blend_pixels(const Color& bottom, const Color& top, BlendMode mode, float opacity);
    
    // Apply frosted glass effect to a region of the composite
    void apply_frosted_glass(Surface& dest, int x, int y, int w, int h, 
                            const Surface& mask, float scale_x, float scale_y, 
                            float blur_radius);
};

} // namespace nativeui

