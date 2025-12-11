#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "gpu_surface.hpp"
#include <algorithm>
#include <vector>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

namespace palladium {

GPUSurface::GPUSurface(int width, int height)
    : width_(width), height_(height)
{
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("GPUSurface dimensions must be positive");
    }
    
    auto& ctx = D2DContext::instance();
    if (!ctx.is_available()) {
        throw std::runtime_error("GPU acceleration not available");
    }
    
    // Create device context
    context_ = ctx.create_device_context();
    
    // Create bitmap as render target that can also be used as effect input
    // Note: D2D1_BITMAP_OPTIONS_TARGET allows rendering TO this bitmap
    // Without CANNOT_DRAW, it can also be used as a source for effects
    D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );
    
    D2D1_SIZE_U size = D2D1::SizeU(width, height);
    
    HRESULT hr = context_->CreateBitmap(size, nullptr, 0, props, &bitmap_);
    throw_if_failed(hr, "Failed to create GPU bitmap");
    
    // Set as render target
    context_->SetTarget(bitmap_.Get());
    
    // Set high quality rendering
    context_->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    context_->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
}

GPUSurface::~GPUSurface() {
    if (is_drawing_) {
        context_->EndDraw();
    }
}

GPUSurface::GPUSurface(GPUSurface&& other) noexcept
    : width_(other.width_)
    , height_(other.height_)
    , is_drawing_(other.is_drawing_)
    , context_(std::move(other.context_))
    , bitmap_(std::move(other.bitmap_))
    , solid_brush_(std::move(other.solid_brush_))
{
    other.width_ = 0;
    other.height_ = 0;
    other.is_drawing_ = false;
}

GPUSurface& GPUSurface::operator=(GPUSurface&& other) noexcept {
    if (this != &other) {
        if (is_drawing_) {
            context_->EndDraw();
        }
        
        width_ = other.width_;
        height_ = other.height_;
        is_drawing_ = other.is_drawing_;
        context_ = std::move(other.context_);
        bitmap_ = std::move(other.bitmap_);
        solid_brush_ = std::move(other.solid_brush_);
        
        other.width_ = 0;
        other.height_ = 0;
        other.is_drawing_ = false;
    }
    return *this;
}

void GPUSurface::begin_draw() {
    if (!is_drawing_) {
        context_->BeginDraw();
        is_drawing_ = true;
    }
}

void GPUSurface::end_draw() {
    if (is_drawing_) {
        HRESULT hr = context_->EndDraw();
        is_drawing_ = false;
        throw_if_failed(hr, "GPU drawing failed");
    }
}

void GPUSurface::ensure_brush() {
    if (!solid_brush_) {
        HRESULT hr = context_->CreateSolidColorBrush(
            D2D1::ColorF(D2D1::ColorF::White),
            &solid_brush_
        );
        throw_if_failed(hr, "Failed to create brush");
    }
}

D2D1_COLOR_F GPUSurface::to_d2d_color(const nativeui::Color& color) const {
    return D2D1::ColorF(
        color.r / 255.0f,
        color.g / 255.0f,
        color.b / 255.0f,
        color.a / 255.0f
    );
}

void GPUSurface::clear(const nativeui::Color& color) {
    bool was_drawing = is_drawing_;
    if (!was_drawing) begin_draw();
    
    context_->Clear(to_d2d_color(color));
    
    if (!was_drawing) end_draw();
}

void GPUSurface::fill(const nativeui::Color& color) {
    fill_rect(0, 0, width_, height_, color);
}

void GPUSurface::fill_rect(int x, int y, int w, int h, const nativeui::Color& color) {
    bool was_drawing = is_drawing_;
    if (!was_drawing) begin_draw();
    
    ensure_brush();
    solid_brush_->SetColor(to_d2d_color(color));
    
    D2D1_RECT_F rect = D2D1::RectF(
        static_cast<float>(x),
        static_cast<float>(y),
        static_cast<float>(x + w),
        static_cast<float>(y + h)
    );
    
    context_->FillRectangle(rect, solid_brush_.Get());
    
    if (!was_drawing) end_draw();
}

void GPUSurface::draw_rect(int x, int y, int w, int h, const nativeui::Color& color, float stroke_width) {
    bool was_drawing = is_drawing_;
    if (!was_drawing) begin_draw();
    
    ensure_brush();
    solid_brush_->SetColor(to_d2d_color(color));
    
    D2D1_RECT_F rect = D2D1::RectF(
        static_cast<float>(x),
        static_cast<float>(y),
        static_cast<float>(x + w),
        static_cast<float>(y + h)
    );
    
    context_->DrawRectangle(rect, solid_brush_.Get(), stroke_width);
    
    if (!was_drawing) end_draw();
}

void GPUSurface::draw_circle(int cx, int cy, int radius, const nativeui::Color& color, float stroke_width) {
    bool was_drawing = is_drawing_;
    if (!was_drawing) begin_draw();
    
    ensure_brush();
    solid_brush_->SetColor(to_d2d_color(color));
    
    D2D1_ELLIPSE ellipse = D2D1::Ellipse(
        D2D1::Point2F(static_cast<float>(cx), static_cast<float>(cy)),
        static_cast<float>(radius),
        static_cast<float>(radius)
    );
    
    context_->DrawEllipse(ellipse, solid_brush_.Get(), stroke_width);
    
    if (!was_drawing) end_draw();
}

void GPUSurface::fill_circle(int cx, int cy, int radius, const nativeui::Color& color) {
    bool was_drawing = is_drawing_;
    if (!was_drawing) begin_draw();
    
    ensure_brush();
    solid_brush_->SetColor(to_d2d_color(color));
    
    D2D1_ELLIPSE ellipse = D2D1::Ellipse(
        D2D1::Point2F(static_cast<float>(cx), static_cast<float>(cy)),
        static_cast<float>(radius),
        static_cast<float>(radius)
    );
    
    context_->FillEllipse(ellipse, solid_brush_.Get());
    
    if (!was_drawing) end_draw();
}

void GPUSurface::draw_rounded_rect(int x, int y, int w, int h, int radius, const nativeui::Color& color, float stroke_width) {
    bool was_drawing = is_drawing_;
    if (!was_drawing) begin_draw();
    
    ensure_brush();
    solid_brush_->SetColor(to_d2d_color(color));
    
    D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(
        D2D1::RectF(
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(x + w),
            static_cast<float>(y + h)
        ),
        static_cast<float>(radius),
        static_cast<float>(radius)
    );
    
    context_->DrawRoundedRectangle(rr, solid_brush_.Get(), stroke_width);
    
    if (!was_drawing) end_draw();
}

void GPUSurface::fill_rounded_rect(int x, int y, int w, int h, int radius, const nativeui::Color& color) {
    bool was_drawing = is_drawing_;
    if (!was_drawing) begin_draw();
    
    ensure_brush();
    solid_brush_->SetColor(to_d2d_color(color));
    
    D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(
        D2D1::RectF(
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(x + w),
            static_cast<float>(y + h)
        ),
        static_cast<float>(radius),
        static_cast<float>(radius)
    );
    
    context_->FillRoundedRectangle(rr, solid_brush_.Get());
    
    if (!was_drawing) end_draw();
}

void GPUSurface::draw_line(int x1, int y1, int x2, int y2, const nativeui::Color& color, float stroke_width) {
    bool was_drawing = is_drawing_;
    if (!was_drawing) begin_draw();
    
    ensure_brush();
    solid_brush_->SetColor(to_d2d_color(color));
    
    context_->DrawLine(
        D2D1::Point2F(static_cast<float>(x1), static_cast<float>(y1)),
        D2D1::Point2F(static_cast<float>(x2), static_cast<float>(y2)),
        solid_brush_.Get(),
        stroke_width
    );
    
    if (!was_drawing) end_draw();
}

void GPUSurface::blit(const GPUSurface& source, int dest_x, int dest_y, float opacity) {
    blit_scaled(source, dest_x, dest_y, source.width_, source.height_, opacity);
}

void GPUSurface::blit_scaled(const GPUSurface& source, int dest_x, int dest_y, int dest_w, int dest_h, float opacity) {
    if (!context_ || !source.get_bitmap()) return;
    ensure_brush();
    
    bool was_drawing = is_drawing_;
    if (!was_drawing) begin_draw();
    
    D2D1_RECT_F dest_rect = D2D1::RectF(
        static_cast<float>(dest_x),
        static_cast<float>(dest_y),
        static_cast<float>(dest_x + dest_w),
        static_cast<float>(dest_y + dest_h)
    );
    
    D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_NONE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );

    ComPtr<ID2D1Bitmap1> drawable_bitmap;
    HRESULT hr = context_->CreateBitmap(
        D2D1::SizeU(source.width_, source.height_),
        nullptr, 0, props, &drawable_bitmap
    );
    
    if (SUCCEEDED(hr)) {
        D2D1_POINT_2U dest_point = D2D1::Point2U(0, 0);
        D2D1_RECT_U src_rect = D2D1::RectU(0, 0, source.width_, source.height_);
        hr = drawable_bitmap->CopyFromBitmap(&dest_point, source.bitmap_.Get(), &src_rect);
        
        if (SUCCEEDED(hr)) {
            context_->DrawBitmap(
                drawable_bitmap.Get(),
                dest_rect,
                opacity,
                D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC
            );
        }
    }
    
    if (!was_drawing) end_draw();
}

void GPUSurface::push_axis_aligned_clip(int x, int y, int w, int h) {
    if (!context_) return;
    begin_draw();
    D2D1_RECT_F rect = D2D1::RectF(
        static_cast<float>(x),
        static_cast<float>(y),
        static_cast<float>(x + w),
        static_cast<float>(y + h)
    );
    // D2D1_ANTIALIAS_MODE_PER_PRIMITIVE is default for alias mode
    context_->PushAxisAlignedClip(rect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
}

void GPUSurface::pop_clip() {
    if (!context_) return;
    context_->PopAxisAlignedClip();
}

void GPUSurface::push_rounded_clip(int x, int y, int w, int h, float radius) {
    if (!context_) return;
    begin_draw();
    
    ComPtr<ID2D1Factory> factory;
    context_->GetFactory(&factory);
    
    ComPtr<ID2D1RoundedRectangleGeometry> geometry;
    D2D1_ROUNDED_RECT rrect = D2D1::RoundedRect(
        D2D1::RectF(
            static_cast<float>(x), 
            static_cast<float>(y), 
            static_cast<float>(x + w), 
            static_cast<float>(y + h)
        ), 
        radius, 
        radius
    );
    factory->CreateRoundedRectangleGeometry(rrect, &geometry);
    
    // Create Layout
    ComPtr<ID2D1Layer> layer;
    context_->CreateLayer(nullptr, &layer);
    
    // Create Layer Parameters
    D2D1_LAYER_PARAMETERS params = D2D1::LayerParameters();
    params.contentBounds = D2D1::InfiniteRect();
    params.geometricMask = geometry.Get();
    params.maskAntialiasMode = D2D1_ANTIALIAS_MODE_PER_PRIMITIVE;
    params.maskTransform = D2D1::Matrix3x2F::Identity();
    params.opacity = 1.0f;
    params.layerOptions = D2D1_LAYER_OPTIONS_NONE;
    
    context_->PushLayer(params, layer.Get());
}

void GPUSurface::pop_rounded_clip() {
    if (!context_) return;
    context_->PopLayer();
}

void GPUSurface::upload_from(const nativeui::Surface& cpu_surface) {
    // Create a staging bitmap for upload
    D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_NONE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );
    
    // CPU surface is RGBA, D2D expects BGRA, so we need to convert
    int w = std::min(width_, cpu_surface.get_width());
    int h = std::min(height_, cpu_surface.get_height());
    
    std::vector<uint8_t> bgra_data(w * h * 4);
    const uint8_t* rgba = cpu_surface.get_data();
    
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            size_t src_idx = (y * cpu_surface.get_width() + x) * 4;
            size_t dst_idx = (y * w + x) * 4;
            
            // RGBA -> BGRA with premultiplied alpha
            float alpha = rgba[src_idx + 3] / 255.0f;
            bgra_data[dst_idx + 0] = static_cast<uint8_t>(rgba[src_idx + 2] * alpha); // B
            bgra_data[dst_idx + 1] = static_cast<uint8_t>(rgba[src_idx + 1] * alpha); // G
            bgra_data[dst_idx + 2] = static_cast<uint8_t>(rgba[src_idx + 0] * alpha); // R
            bgra_data[dst_idx + 3] = rgba[src_idx + 3]; // A
        }
    }
    
    D2D1_RECT_U rect = D2D1::RectU(0, 0, w, h);
    bitmap_->CopyFromMemory(&rect, bgra_data.data(), w * 4);
}

std::shared_ptr<nativeui::Surface> GPUSurface::download_to_cpu() const {
    auto result = std::make_shared<nativeui::Surface>(width_, height_);
    
    // Create a CPU-readable bitmap
    D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_CPU_READ | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );
    
    ComPtr<ID2D1Bitmap1> staging;
    HRESULT hr = context_->CreateBitmap(
        D2D1::SizeU(width_, height_),
        nullptr, 0, props, &staging
    );
    
    if (FAILED(hr)) return result;
    
    D2D1_POINT_2U dest_point = D2D1::Point2U(0, 0);
    hr = staging->CopyFromBitmap(&dest_point, bitmap_.Get(), nullptr);
    if (FAILED(hr)) return result;
    
    D2D1_MAPPED_RECT mapped;
    hr = staging->Map(D2D1_MAP_OPTIONS_READ, &mapped);
    if (FAILED(hr)) return result;
    
    // Convert BGRA premultiplied -> RGBA straight
    for (int y = 0; y < height_; ++y) {
        const uint8_t* src_row = mapped.bits + y * mapped.pitch;
        for (int x = 0; x < width_; ++x) {
            size_t idx = x * 4;
            uint8_t b = src_row[idx + 0];
            uint8_t g = src_row[idx + 1];
            uint8_t r = src_row[idx + 2];
            uint8_t a = src_row[idx + 3];
            
            // Unpremultiply
            if (a > 0) {
                float inv_alpha = 255.0f / a;
                r = static_cast<uint8_t>(std::min(255.0f, r * inv_alpha));
                g = static_cast<uint8_t>(std::min(255.0f, g * inv_alpha));
                b = static_cast<uint8_t>(std::min(255.0f, b * inv_alpha));
            }
            
            result->set_pixel(x, y, r, g, b, a);
        }
    }
    
    staging->Unmap();
    return result;
}

} // namespace palladium

// Helper in anonymous namespace or just helper
namespace {
    std::wstring to_wstring(const std::string& str) {
        if (str.empty()) return std::wstring();
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
        std::wstring wstrTo(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
        return wstrTo;
    }
}

namespace palladium {


void GPUSurface::draw_arc(float cx, float cy, float radius, float start_angle, float sweep_angle, const nativeui::Color& color, float stroke_width, bool rounded_caps) {
    if (sweep_angle == 0.0f) return;
    begin_draw();

    // Factory for geometry
    ComPtr<ID2D1Factory> factory;
    context_->GetFactory(&factory);

    ComPtr<ID2D1PathGeometry> path_geometry;
    HRESULT hr = factory->CreatePathGeometry(&path_geometry);
    if (FAILED(hr)) return;

    ComPtr<ID2D1GeometrySink> sink;
    hr = path_geometry->Open(&sink);
    if (FAILED(hr)) return;

    // Convert angles to radians
    float start_rad = start_angle * (M_PI / 180.0f);
    float sweep_rad = sweep_angle * (M_PI / 180.0f);
    float end_rad = start_rad + sweep_rad;

    D2D1_POINT_2F start_point = D2D1::Point2F(
        cx + radius * std::cos(start_rad),
        cy + radius * std::sin(start_rad)
    );

    D2D1_POINT_2F end_point = D2D1::Point2F(
        cx + radius * std::cos(end_rad),
        cy + radius * std::sin(end_rad)
    );

    sink->BeginFigure(start_point, D2D1_FIGURE_BEGIN_HOLLOW);
    
    // Add Arc
    // D2D1_ARC_SEGMENT
    // point: end point
    // size: radius
    // rotationAngle: 0
    // sweepDirection: CLOCKWISE (assuming positive sweep adds to angle)
    // arcSize: SMALL if sweep < 180, LARGE if >= 180
    
    sink->AddArc(D2D1::ArcSegment(
        end_point,
        D2D1::SizeF(radius, radius),
        0.0f,
        sweep_angle > 0 ? D2D1_SWEEP_DIRECTION_CLOCKWISE : D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE,
        std::abs(sweep_angle) >= 180.0f ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL
    ));

    sink->EndFigure(D2D1_FIGURE_END_OPEN);
    sink->Close();

    ensure_brush();
    solid_brush_->SetColor(to_d2d_color(color));

    ComPtr<ID2D1StrokeStyle> stroke_style;
    if (rounded_caps) {
        D2D1_STROKE_STYLE_PROPERTIES props = D2D1::StrokeStyleProperties(
            D2D1_CAP_STYLE_ROUND,
            D2D1_CAP_STYLE_ROUND,
            D2D1_CAP_STYLE_ROUND,
            D2D1_LINE_JOIN_ROUND,
            10.0f,
            D2D1_DASH_STYLE_SOLID,
            0.0f
        );
        factory->CreateStrokeStyle(props, nullptr, 0, &stroke_style);
    }

    context_->DrawGeometry(path_geometry.Get(), solid_brush_.Get(), stroke_width, stroke_style.Get());
}

void GPUSurface::draw_text(const std::string& text, float x, float y, const std::string& font_name, float font_size, const nativeui::Color& color) {
    if (text.empty()) return;
    begin_draw();
    
    auto dwrite_factory = D2DContext::instance().get_dwrite_factory();
    ComPtr<IDWriteTextFormat> text_format;
    
    std::string family = font_name;
    DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL;
    
    // Simple bold detection
    if (family.size() > 5 && family.substr(family.size() - 5) == " Bold") {
        family = family.substr(0, family.size() - 5);
        weight = DWRITE_FONT_WEIGHT_BOLD;
    }
    
    std::wstring wfont = to_wstring(family);
    // Use system font collection
    HRESULT hr = dwrite_factory->CreateTextFormat(
        wfont.c_str(),
        nullptr,
        weight,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        font_size,
        L"en-us",
        &text_format
    );
    
    if (FAILED(hr)) return;
    
    std::wstring wtext = to_wstring(text);
    
    ensure_brush();
    solid_brush_->SetColor(to_d2d_color(color));
    
    D2D1_RECT_F rect = D2D1::RectF((float)x, (float)y, (float)width_, (float)height_);
    
    context_->DrawText(
        wtext.c_str(),
        (UINT32)wtext.length(),
        text_format.Get(),
        rect,
        solid_brush_.Get(),
        D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT
    );
}

void GPUSurface::draw_text_layout(IDWriteTextLayout* layout, float x, float y, const nativeui::Color& color) {
    if (!layout) return;
    begin_draw();
    
    ensure_brush();
    solid_brush_->SetColor(to_d2d_color(color));
    
    D2D1_POINT_2F origin = D2D1::Point2F((float)x, (float)y);
    
    context_->DrawTextLayout(
        origin,
        layout,
        solid_brush_.Get(),
        D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT
    );
}

} // namespace palladium

#endif // _WIN32
