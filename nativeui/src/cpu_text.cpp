#include "cpu_text.hpp"
#include <iostream>
#include <cmath>

namespace palladium {

CPUText::CPUText(const std::string& content, const std::string& font, float size)
    : text_(content), font_(font), size_(size), color_(255, 255, 255, 255) {
}

void CPUText::set_text(const std::string& text) {
    if (text_ != text) {
        text_ = text;
        dirty_ = true;
    }
}

void CPUText::set_font(const std::string& font) {
    if (font_ != font) {
        font_ = font;
        dirty_ = true;
    }
}

void CPUText::set_size(float size) {
    if (size_ != size) {
        size_ = size;
        dirty_ = true;
    }
}

void CPUText::set_color(const nativeui::Color& color) {
    if (color_.r != color.r || color_.g != color.g || 
        color_.b != color.b || color_.a != color.a) {
        color_ = color;
        dirty_ = true;
    }
}

void CPUText::set_position(float x, float y) {
    x_ = x;
    y_ = y;
}

void CPUText::set_width(float width) {
    if (width_ != width) {
        width_ = width;
        dirty_ = true;
    }
}

void CPUText::set_line_spacing(float spacing) {
    if (line_spacing_ != spacing) {
        line_spacing_ = spacing;
        dirty_ = true;
    }
}

void CPUText::set_align(TextAlign align) {
    if (align_ != align) {
        align_ = align;
        dirty_ = true; // Alignment affects wrapping visualization if I implement it
    }
}

void CPUText::set_valign(TextVAlign valign) {
    if (valign_ != valign) {
        valign_ = valign;
        dirty_ = true;
    }
}

void CPUText::set_shadow(const nativeui::Color& color, float off_x, float off_y, float blur) {
    shadow_.color = color;
    shadow_.offset_x = off_x;
    shadow_.offset_y = off_y;
    shadow_.blur = blur;
    shadow_.enabled = (color.a > 0);
    // Shadow is applied during draw or composition. 
    // If we composite deeply, need rebuild.
    dirty_ = true; 
}

void CPUText::set_outline(const nativeui::Color& color, float width) {
    outline_.color = color;
    outline_.width = width;
    outline_.enabled = (color.a > 0 && width > 0);
    dirty_ = true;
}

void CPUText::rebuild_cache() {
    cached_surface_ = nullptr;
    shadow_surface_ = nullptr;
    outline_surface_ = nullptr;

    if (text_.empty()) return;

    // Load font
    // Note: CPU Font size is int
    auto font = nativeui::FontCache::get(font_, static_cast<int>(size_));
    if (!font) return;

    // Main Text
    if (width_ > 0) {
        cached_surface_ = font->render_wrapped(text_, color_, static_cast<int>(width_));
    } else {
        cached_surface_ = font->render(text_, color_);
    }

    if (!cached_surface_) return;

    // Shadow Surface (re-render with shadow color)
    if (shadow_.enabled) {
        // Just render same text with shadow color
        if (width_ > 0) {
            shadow_surface_ = font->render_wrapped(text_, shadow_.color, static_cast<int>(width_));
        } else {
            shadow_surface_ = font->render(text_, shadow_.color);
        }
    }

    // Outline Surface (Poor man's outline: Render with outline color)
    if (outline_.enabled) {
        // We render the text in outline color. 
        // We will draw it at offsets during draw().
        if (width_ > 0) {
            outline_surface_ = font->render_wrapped(text_, outline_.color, static_cast<int>(width_));
        } else {
            outline_surface_ = font->render(text_, outline_.color);
        }
    }

    dirty_ = false;
}

void CPUText::draw(std::shared_ptr<nativeui::Surface> surface) {
    if (dirty_) rebuild_cache();
    if (!cached_surface_) return;
    if (!surface) return;

    int ix = static_cast<int>(x_);
    int iy = static_cast<int>(y_);

    // Draw Shadow
    if (shadow_.enabled && shadow_surface_) {
        // Simple offset. No blur on CPU for now.
        surface->blit(*shadow_surface_, ix + static_cast<int>(shadow_.offset_x), iy + static_cast<int>(shadow_.offset_y));
    }

    // Draw Outline (Poor man's stroke - 4 directions)
    if (outline_.enabled && outline_surface_) {
        int w = static_cast<int>(outline_.width);
        if (w < 1) w = 1;
        
        // Very basic stroke
        surface->blit(*outline_surface_, ix - w, iy);
        surface->blit(*outline_surface_, ix + w, iy);
        surface->blit(*outline_surface_, ix, iy - w);
        surface->blit(*outline_surface_, ix, iy + w);
        
        // Diagonals for thicker stroke?
        if (w > 1) {
            surface->blit(*outline_surface_, ix - w, iy - w);
            surface->blit(*outline_surface_, ix + w, iy - w);
            surface->blit(*outline_surface_, ix - w, iy + w);
            surface->blit(*outline_surface_, ix + w, iy + w);
        }
    }

    // Draw Main Text
    surface->blit(*cached_surface_, ix, iy);
}

float CPUText::get_render_width() const {
    if (dirty_) const_cast<CPUText*>(this)->rebuild_cache();
    return cached_surface_ ? static_cast<float>(cached_surface_->get_width()) : 0.0f;
}

float CPUText::get_render_height() const {
    if (dirty_) const_cast<CPUText*>(this)->rebuild_cache();
    return cached_surface_ ? static_cast<float>(cached_surface_->get_height()) : 0.0f;
}

} // namespace palladium
