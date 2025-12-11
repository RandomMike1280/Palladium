#pragma once

#ifdef _WIN32

#include <string>
#include <memory>
#include "d2d_context.hpp"
#include "surface.hpp"
#include "gpu_surface.hpp"
#include "text_common.hpp"

namespace palladium {

// Enums moved to text_common.hpp

class GPUText {
public:
    GPUText(const std::string& content = "", const std::string& font = "Arial", float size = 16.0f);
    ~GPUText() = default;

    // Core Properties
    void set_text(const std::string& text);
    std::string get_text() const { return text_; }
    
    void set_font(const std::string& font);
    std::string get_font() const { return font_; }
    
    void set_size(float size);
    float get_size() const { return size_; }
    
    void set_color(const nativeui::Color& color);
    nativeui::Color get_color() const { return color_; }
    
    void set_position(float x, float y);
    float get_x() const { return x_; }
    float get_y() const { return y_; }
    
    // Layout Properties
    void set_width(float width); // 0 or negative = no wrap
    float get_width() const { return width_; }
    
    void set_line_spacing(float spacing);
    
    void set_align(TextAlign align);
    void set_valign(TextVAlign valign);
    
    // Effects
    void set_shadow(const nativeui::Color& color, float off_x, float off_y, float blur);
    void set_outline(const nativeui::Color& color, float width);
    
    // Rendering
    void draw(GPUSurface& surface);
    void update(float dt); // Placeholder for future animations
    
    // Metrics (from layout)
    float get_render_width() const;
    float get_render_height() const;

private:
    void rebuild_layout();
    
    std::string text_;
    std::string font_;
    float size_;
    nativeui::Color color_;
    float x_ = 0.0f;
    float y_ = 0.0f;
    float width_ = 0.0f;
    float line_spacing_ = 1.0f;
    TextAlign align_ = TextAlign::Left;
    TextVAlign valign_ = TextVAlign::Top;
    
    TextShadow shadow_;
    TextOutline outline_;
    
    bool dirty_ = true;
    
    // DirectWrite Resources
    ComPtr<IDWriteTextLayout> layout_;
    ComPtr<IDWriteTextFormat> format_;
};

} // namespace palladium

#endif // _WIN32
