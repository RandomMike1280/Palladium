#ifdef _WIN32

#include "gpu_text.hpp"
#include "string_utils.hpp"
#include <dwrite.h>
#include <iostream>

namespace palladium {

GPUText::GPUText(const std::string& content, const std::string& font, float size)
    : text_(content), font_(font), size_(size), color_(255, 255, 255, 255)
{
    rebuild_layout();
}

void GPUText::set_text(const std::string& text) {
    if (text_ != text) {
        text_ = text;
        dirty_ = true;
    }
}

void GPUText::set_font(const std::string& font) {
    if (font_ != font) {
        font_ = font;
        dirty_ = true;
    }
}

void GPUText::set_size(float size) {
    if (size_ != size) {
        size_ = size;
        dirty_ = true;
    }
}

void GPUText::set_color(const nativeui::Color& color) {
    color_ = color;
}

void GPUText::set_position(float x, float y) {
    x_ = x;
    y_ = y;
}

void GPUText::set_width(float width) {
    if (width_ != width) {
        width_ = width;
        dirty_ = true;
    }
}

void GPUText::set_line_spacing(float spacing) {
    if (line_spacing_ != spacing) {
        line_spacing_ = spacing;
        dirty_ = true;
    }
}

void GPUText::set_align(TextAlign align) {
    if (align_ != align) {
        align_ = align;
        dirty_ = true;
    }
}

void GPUText::set_valign(TextVAlign valign) {
    if (valign_ != valign) {
        valign_ = valign;
        dirty_ = true;
    }
}

void GPUText::set_shadow(const nativeui::Color& color, float off_x, float off_y, float blur) {
    shadow_.color = color;
    shadow_.offset_x = off_x;
    shadow_.offset_y = off_y;
    shadow_.blur = blur;
    shadow_.enabled = (color.a > 0);
}

void GPUText::set_outline(const nativeui::Color& color, float width) {
    outline_.color = color;
    outline_.width = width;
    outline_.enabled = (color.a > 0 && width > 0);
}

void GPUText::rebuild_layout() {
    auto factory = D2DContext::instance().get_dwrite_factory();
    if (!factory) return;
    
    std::wstring wfont = to_wstring(font_);
    std::wstring wtext = to_wstring(text_);
    
    // Create Format
    HRESULT hr = factory->CreateTextFormat(
        wfont.c_str(),
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        size_,
        L"en-us",
        &format_
    );
    
    if (FAILED(hr)) return;
    
    // Set Alignment
    DWRITE_TEXT_ALIGNMENT align_val = DWRITE_TEXT_ALIGNMENT_LEADING;
    switch (align_) {
        case TextAlign::Center: align_val = DWRITE_TEXT_ALIGNMENT_CENTER; break;
        case TextAlign::Right: align_val = DWRITE_TEXT_ALIGNMENT_TRAILING; break;
        case TextAlign::Justified: align_val = DWRITE_TEXT_ALIGNMENT_JUSTIFIED; break;
    }
    format_->SetTextAlignment(align_val);
    
    // Set Vertical Alignment
    DWRITE_PARAGRAPH_ALIGNMENT valign_val = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
    switch (valign_) {
        case TextVAlign::Middle: valign_val = DWRITE_PARAGRAPH_ALIGNMENT_CENTER; break;
        case TextVAlign::Bottom: valign_val = DWRITE_PARAGRAPH_ALIGNMENT_FAR; break;
    }
    format_->SetParagraphAlignment(valign_val);

    // Create Layout
    float max_width = (width_ > 0) ? width_ : 50000.0f; // Large logical width if no wrapping
    float max_height = 50000.0f;
    
    hr = factory->CreateTextLayout(
        wtext.c_str(),
        (UINT32)wtext.length(),
        format_.Get(),
        max_width,
        max_height,
        &layout_
    );
    
    if (FAILED(hr)) return;
    
    // Set Word Wrapping
    if (width_ > 0) {
        layout_->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
    } else {
        layout_->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    }
    
    // Set Line Spacing
    if (line_spacing_ != 1.0f) {
        // Method: DWRITE_LINE_SPACING_METHOD_UNIFORM
        // lineSpacing: size_ * spacing
        // baseline: size_ * spacing * 0.8 (approx)
        layout_->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, size_ * line_spacing_, size_ * line_spacing_ * 0.8f);
    }
    
    dirty_ = false;
}

float GPUText::get_render_width() const {
    if (!layout_) const_cast<GPUText*>(this)->rebuild_layout();
    if (!layout_) return 1.0f;
    
    DWRITE_TEXT_METRICS metrics;
    layout_->GetMetrics(&metrics);
    return metrics.width; // or layoutWidth depending on wrapping
}

float GPUText::get_render_height() const {
    if (!layout_) const_cast<GPUText*>(this)->rebuild_layout();
    if (!layout_) return 1.0f;
    
    DWRITE_TEXT_METRICS metrics;
    layout_->GetMetrics(&metrics);
    return metrics.height;
}

void GPUText::update(float dt) {
    // Animation logic would go here
}

void GPUText::draw(GPUSurface& surface) {
    if (dirty_) rebuild_layout();
    if (!layout_) return;
    
    // Shadow
    if (shadow_.enabled) {
        // Multi-pass blur simulation? 
        // 4-tap or 8-tap offset is cheap implementation of blur
        // But for now, simple offset
        if (shadow_.blur > 0.0f) {
            // Very primitive blur: draw at small offsets with lower alpha
            int steps = 4;
            float step_rad = shadow_.blur / 2.0f;
            nativeui::Color c = shadow_.color;
            c.a /= steps; // Distribute alpha
            
            surface.draw_text_layout(layout_.Get(), x_ + shadow_.offset_x - step_rad, y_ + shadow_.offset_y - step_rad, c);
            surface.draw_text_layout(layout_.Get(), x_ + shadow_.offset_x + step_rad, y_ + shadow_.offset_y - step_rad, c);
            surface.draw_text_layout(layout_.Get(), x_ + shadow_.offset_x - step_rad, y_ + shadow_.offset_y + step_rad, c);
            surface.draw_text_layout(layout_.Get(), x_ + shadow_.offset_x + step_rad, y_ + shadow_.offset_y + step_rad, c);
        } else {
            surface.draw_text_layout(layout_.Get(), x_ + shadow_.offset_x, y_ + shadow_.offset_y, shadow_.color);
        }
    }
    
    // Outline (Poor man's stroke)
    if (outline_.enabled) {
        nativeui::Color c = outline_.color;
        float w = outline_.width;
        // 8-way sample or 4-way
        surface.draw_text_layout(layout_.Get(), x_ - w, y_, c);
        surface.draw_text_layout(layout_.Get(), x_ + w, y_, c);
        surface.draw_text_layout(layout_.Get(), x_, y_ - w, c);
        surface.draw_text_layout(layout_.Get(), x_, y_ + w, c);
        if (w > 1.0f) {
             // Diagonals
            float d = w * 0.707f;
            surface.draw_text_layout(layout_.Get(), x_ - d, y_ - d, c);
            surface.draw_text_layout(layout_.Get(), x_ + d, y_ - d, c);
            surface.draw_text_layout(layout_.Get(), x_ - d, y_ + d, c);
            surface.draw_text_layout(layout_.Get(), x_ + d, y_ + d, c);
        }
    }
    
    // Main Text
    surface.draw_text_layout(layout_.Get(), x_, y_, color_);
}

} // namespace palladium

#endif // _WIN32
