#pragma once

#include <cmath>
#include <memory>
#include <random>
#include "surface.hpp"

namespace nativeui {

// Forward declaration for easing
enum class EasingType;

/**
 * Effects - Visual effects that can be applied to surfaces
 */
class Effects {
public:
    // Blur effects
    static void box_blur(Surface& surface, int radius);
    static void gaussian_blur(Surface& surface, float sigma);
    static void blur_region(Surface& surface, int x, int y, int w, int h, int radius);
    
    // Frosted glass effect
    static void frosted_glass(Surface& surface, int blur_radius = 10, float noise_amount = 0.05f, float saturation = 0.8f);
    static void frosted_glass_region(Surface& surface, int x, int y, int w, int h, int blur_radius = 10);
    
    // Pixel displacement
    static void displace(Surface& surface, const Surface& displacement_map, float strength = 10.0f);
    static void wave_distort(Surface& surface, float amplitude, float frequency, float phase = 0.0f);
    static void ripple(Surface& surface, int center_x, int center_y, float amplitude, float wavelength, float phase = 0.0f);
    
    // Color adjustments
    static void brightness(Surface& surface, float amount);  // -1.0 to 1.0
    static void contrast(Surface& surface, float amount);    // 0.0 to 2.0
    static void saturation(Surface& surface, float amount);  // 0.0 = grayscale, 1.0 = normal
    static void hue_shift(Surface& surface, float degrees);  // 0 to 360
    static void invert(Surface& surface);
    static void grayscale(Surface& surface);
    static void sepia(Surface& surface, float strength = 1.0f);
    
    // Blend two surfaces together
    static void blend(Surface& dest, const Surface& source, float alpha);
    
    // Gradient fills
    static void linear_gradient(Surface& surface, int x1, int y1, int x2, int y2, 
                                 const Color& color1, const Color& color2);
    static void radial_gradient(Surface& surface, int cx, int cy, int radius,
                                 const Color& inner_color, const Color& outer_color);
    
    // Noise generation
    static void noise(Surface& surface, float amount);  // 0.0 to 1.0
    static void perlin_noise(Surface& surface, float scale, int octaves = 4);
    
    // Shadow effect
    static std::shared_ptr<Surface> drop_shadow(const Surface& source, int offset_x, int offset_y, 
                                                  int blur_radius, const Color& shadow_color);

private:
    // Helper functions
    static void horizontal_box_blur(Surface& surface, int radius);
    static void vertical_box_blur(Surface& surface, int radius);
    static std::vector<float> generate_gaussian_kernel(float sigma);
    
    // Random number generator for noise
    static std::mt19937& get_rng();
};

/**
 * BlurredSurface - A surface that renders with gaussian blur
 * Supports animated blur radius with easing
 */
class BlurredSurface {
public:
    BlurredSurface(int width, int height);
    BlurredSurface(std::shared_ptr<Surface> surface);
    
    // Access the underlying surface for drawing
    Surface& get_surface() { return *surface_; }
    const Surface& get_surface() const { return *surface_; }
    std::shared_ptr<Surface> get_surface_ptr() { return surface_; }
    
    // Blur radius property
    float get_blur_radius() const { return current_radius_; }
    void set_blur_radius(float radius);  // Immediate
    
    // Animated blur radius
    void animate_blur_radius(float target_radius, float duration, int easing_type = 0);
    
    // Update animation (call each frame)
    void update(float dt);
    
    // Check if blur animation is running
    bool is_animating() const { return animating_; }
    
    // Render with current blur
    std::shared_ptr<Surface> render() const;
    void render_to(Surface& dest, int x, int y) const;
    
    // Dimensions
    int get_width() const { return surface_->get_width(); }
    int get_height() const { return surface_->get_height(); }

private:
    std::shared_ptr<Surface> surface_;
    
    // Current blur state
    float current_radius_;
    
    // Animation state
    bool animating_;
    float start_radius_;
    float target_radius_;
    float duration_;
    float elapsed_;
    int easing_type_;  // Maps to EasingType enum
    
    // Apply easing
    float apply_easing(float t) const;
};

} // namespace nativeui

