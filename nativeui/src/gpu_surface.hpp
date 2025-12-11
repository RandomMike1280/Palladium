#pragma once

#ifdef _WIN32

#include "d2d_context.hpp"
#include "surface.hpp"
#include <d2d1effects.h>

namespace palladium {

/**
 * GPUSurface - Hardware-accelerated surface using Direct2D
 * 
 * Provides similar API to Surface but with GPU-accelerated rendering.
 * All drawing operations are performed on the GPU.
 */
class GPUSurface {
public:
    GPUSurface(int width, int height);
    ~GPUSurface();
    
    // Non-copyable, movable
    GPUSurface(const GPUSurface&) = delete;
    GPUSurface& operator=(const GPUSurface&) = delete;
    GPUSurface(GPUSurface&& other) noexcept;
    GPUSurface& operator=(GPUSurface&& other) noexcept;
    
    // Dimensions
    int get_width() const { return width_; }
    int get_height() const { return height_; }
    
    // Begin/End drawing (must call before/after drawing operations)
    void begin_draw();
    void end_draw();
    
    // Clear/Fill operations
    void clear(const nativeui::Color& color = nativeui::Color(0, 0, 0, 0));
    void fill(const nativeui::Color& color);
    void fill_rect(int x, int y, int w, int h, const nativeui::Color& color);
    
    // Shape drawing
    void draw_rect(int x, int y, int w, int h, const nativeui::Color& color, float stroke_width = 1.0f);
    void draw_circle(int cx, int cy, int radius, const nativeui::Color& color, float stroke_width = 1.0f);
    void fill_circle(int cx, int cy, int radius, const nativeui::Color& color);
    void draw_rounded_rect(int x, int y, int w, int h, int radius, const nativeui::Color& color, float stroke_width = 1.0f);
    void fill_rounded_rect(int x, int y, int w, int h, int radius, const nativeui::Color& color);
    
    // Line drawing
    void draw_line(int x1, int y1, int x2, int y2, const nativeui::Color& color, float stroke_width = 1.0f);
    void draw_arc(float cx, float cy, float radius, float start_angle, float sweep_angle, const nativeui::Color& color, float stroke_width, bool rounded_caps = true);
    
    // Text drawing
    void draw_text(const std::string& text, float x, float y, const std::string& font_name, float font_size, const nativeui::Color& color);
    void draw_text_layout(IDWriteTextLayout* layout, float x, float y, const nativeui::Color& color);
    
    // Blit another GPU surface
    void blit(const GPUSurface& source, int dest_x, int dest_y, float opacity = 1.0f);
    void blit_scaled(const GPUSurface& source, int dest_x, int dest_y, int dest_w, int dest_h, float opacity = 1.0f);
    
    // Clipping
    void push_axis_aligned_clip(int x, int y, int w, int h);
    void pop_clip();
    
    // Advanced Clipping
    void push_rounded_clip(int x, int y, int w, int h, float radius);
    void pop_rounded_clip();
    
    // Convert from CPU Surface (upload to GPU)
    void upload_from(const nativeui::Surface& cpu_surface);
    
    // Convert to CPU Surface (download from GPU)
    std::shared_ptr<nativeui::Surface> download_to_cpu() const;
    
    // Direct2D access (for advanced usage)
    ID2D1Bitmap1* get_bitmap() const { return bitmap_.Get(); }
    ID2D1DeviceContext* get_context() const { return context_.Get(); }

private:
    int width_;
    int height_;
    bool is_drawing_ = false;
    
    ComPtr<ID2D1DeviceContext> context_;
    ComPtr<ID2D1Bitmap1> bitmap_;
    
    // Cached brushes (created on demand)
    ComPtr<ID2D1SolidColorBrush> solid_brush_;
    
    void ensure_brush();
    D2D1_COLOR_F to_d2d_color(const nativeui::Color& color) const;
};

} // namespace palladium

#endif // _WIN32
