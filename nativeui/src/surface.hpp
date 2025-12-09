#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>

namespace nativeui {

/**
 * Anti-aliasing type enumeration
 */
enum class AAType {
    Off = 0,      // No anti-aliasing
    Basic = 1,    // 2x supersampling (default)
    MSAA4 = 2,    // 4x MSAA-style
    MSAA8 = 3     // 8x MSAA-style
};

/**
 * Global anti-aliasing settings (singleton)
 */
class AntiAliasingSettings {
public:
    static AntiAliasingSettings& instance() {
        static AntiAliasingSettings inst;
        return inst;
    }
    
    void on() { enabled_ = true; }
    void off() { enabled_ = false; }
    bool is_enabled() const { return enabled_; }
    
    void set_type(AAType t) { type_ = t; if (t == AAType::Off) enabled_ = false; else enabled_ = true; }
    void set_type(int t) { set_type(static_cast<AAType>(std::clamp(t, 0, 3))); }
    void set_type(const std::string& t) {
        if (t == "off") set_type(AAType::Off);
        else if (t == "basic") set_type(AAType::Basic);
        else if (t == "msaa4") set_type(AAType::MSAA4);
        else if (t == "msaa8") set_type(AAType::MSAA8);
    }
    AAType get_type() const { return type_; }
    int get_samples() const {
        switch (type_) {
            case AAType::Off: return 1;
            case AAType::Basic: return 2;
            case AAType::MSAA4: return 4;
            case AAType::MSAA8: return 8;
            default: return 2;
        }
    }

private:
    AntiAliasingSettings() : enabled_(true), type_(AAType::Basic) {}
    bool enabled_;
    AAType type_;
};

/**
 * RGBA Color structure (32-bit)
 */
struct Color {
    uint8_t r, g, b, a;
    
    Color() : r(0), g(0), b(0), a(255) {}
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) 
        : r(r), g(g), b(b), a(a) {}
    
    uint32_t to_uint32() const {
        return (static_cast<uint32_t>(a) << 24) |
               (static_cast<uint32_t>(b) << 16) |
               (static_cast<uint32_t>(g) << 8) |
               static_cast<uint32_t>(r);
    }
    
    static Color from_uint32(uint32_t val) {
        return Color(
            val & 0xFF,
            (val >> 8) & 0xFF,
            (val >> 16) & 0xFF,
            (val >> 24) & 0xFF
        );
    }
    
    // Blend with alpha
    Color with_alpha(uint8_t new_alpha) const {
        return Color(r, g, b, new_alpha);
    }
};

/**
 * Surface - A 2D pixel buffer supporting RGBA pixels
 */
class Surface {
public:
    Surface(int width, int height);
    Surface(const Surface& other);
    Surface& operator=(const Surface& other);
    ~Surface() = default;
    
    // Dimensions
    int get_width() const { return width_; }
    int get_height() const { return height_; }
    
    // Direct pixel access
    void set_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    void set_pixel(int x, int y, const Color& color);
    void blend_pixel(int x, int y, const Color& color);  // Alpha-blend pixel
    Color get_pixel(int x, int y) const;
    
    // Fill operations
    void fill(const Color& color);
    void fill_rect(int x, int y, int w, int h, const Color& color);
    void clear();
    
    // Drawing primitives (auto-selects AA based on global settings)
    void draw_line(int x1, int y1, int x2, int y2, const Color& color);
    void draw_rect(int x, int y, int w, int h, const Color& color);
    void draw_circle(int cx, int cy, int radius, const Color& color);
    void fill_circle(int cx, int cy, int radius, const Color& color);
    
    // Explicit non-AA drawing (for performance-critical code)
    void draw_line_no_aa(int x1, int y1, int x2, int y2, const Color& color);
    void draw_circle_no_aa(int cx, int cy, int radius, const Color& color);
    void fill_circle_no_aa(int cx, int cy, int radius, const Color& color);
    
    // Anti-aliased drawing (Xiaolin Wu algorithm)
    void draw_line_aa(int x1, int y1, int x2, int y2, const Color& color);
    void draw_circle_aa(int cx, int cy, int radius, const Color& color);
    void fill_circle_aa(int cx, int cy, int radius, const Color& color);
    
    // Advanced Shapes
    void draw_round_rect(int x, int y, int w, int h, int radius, const Color& color);
    void fill_round_rect(int x, int y, int w, int h, int radius, const Color& color);
    void draw_pill(int x, int y, int w, int h, const Color& color);
    void fill_pill(int x, int y, int w, int h, const Color& color);
    void draw_squircle(int x, int y, int w, int h, const Color& color);
    void fill_squircle(int x, int y, int w, int h, const Color& color);
    
    // Blitting
    void blit(const Surface& source, int dest_x, int dest_y);
    void blit_scaled(const Surface& source, int dest_x, int dest_y, int dest_w, int dest_h);
    void blit_alpha(const Surface& source, int dest_x, int dest_y, float alpha = 1.0f);
    
    // Raw data access (for SDL texture updates)
    const uint8_t* get_data() const { return pixels_.data(); }
    uint8_t* get_data() { return pixels_.data(); }
    size_t get_pitch() const { return width_ * 4; }
    
    // Create a copy
    std::shared_ptr<Surface> copy() const;
    
    // Subsurface (view into a region)
    std::shared_ptr<Surface> subsurface(int x, int y, int w, int h) const;

private:
    int width_;
    int height_;
    std::vector<uint8_t> pixels_;  // RGBA format, 4 bytes per pixel
    
    inline size_t pixel_offset(int x, int y) const {
        return (y * width_ + x) * 4;
    }
    
    inline bool in_bounds(int x, int y) const {
        return x >= 0 && x < width_ && y >= 0 && y < height_;
    }
    
    // AA helpers
    void plot_aa_pixel(int x, int y, const Color& color, float brightness);
};

} // namespace nativeui

