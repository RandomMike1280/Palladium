#pragma once

#include <string>
#include <memory>
#include "surface.hpp"
#include "font.hpp"
#include "text_common.hpp"

// Enums are in text_common.hpp

// Forward declaration
namespace nativeui {
    class Surface;
    struct Color;
}



namespace palladium {

class CPUText {
public:
    CPUText(const std::string& content = "", const std::string& font = "Arial", float size = 16.0f);
    ~CPUText() = default;

    // properties
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
    
    void set_width(float width);
    float get_width() const { return width_; }
    
    void set_line_spacing(float spacing);
    
    void set_align(TextAlign align);
    void set_valign(TextVAlign valign);
    
    void set_shadow(const nativeui::Color& color, float off_x, float off_y, float blur);
    void set_outline(const nativeui::Color& color, float width);
    
    void draw(std::shared_ptr<nativeui::Surface> surface);
    
    float get_render_width() const;
    float get_render_height() const;

private:
    void rebuild_cache();
    
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
    
    // Cache
    std::shared_ptr<nativeui::Surface> cached_surface_;
    std::shared_ptr<nativeui::Surface> shadow_surface_;
    std::shared_ptr<nativeui::Surface> outline_surface_;
};

} // namespace palladium
