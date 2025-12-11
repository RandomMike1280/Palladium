#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "gpu_effects.hpp"
#include <d2d1effects_2.h>
#include <algorithm>
#include <cmath>

// Define effect CLSIDs - these are not automatically defined in some SDK versions
#include <initguid.h>
DEFINE_GUID(CLSID_D2D1GaussianBlur, 0x1feb6d69, 0x2fe6, 0x4ac9, 0x8c, 0x58, 0x1d, 0x7f, 0x93, 0xe7, 0xa6, 0xa5);
DEFINE_GUID(CLSID_D2D1Shadow, 0xC67EA361, 0x1863, 0x4e69, 0x89, 0xDB, 0x69, 0x5D, 0x3E, 0x9A, 0x5B, 0x6B);
DEFINE_GUID(CLSID_D2D1Saturation, 0x5CB2D9CF, 0x327D, 0x459f, 0xA0, 0xCE, 0x40, 0xC0, 0xB2, 0x08, 0x6B, 0xF7);
DEFINE_GUID(CLSID_D2D1Brightness, 0x8CEA8D1E, 0x77B0, 0x4986, 0x99, 0x73, 0x68, 0xD8, 0x2D, 0xA0, 0xEC, 0xF1);
DEFINE_GUID(CLSID_D2D1ColorMatrix, 0x921F03D6, 0x641C, 0x47df, 0x85, 0x2D, 0xB4, 0xBB, 0x61, 0x53, 0xAE, 0x11);
DEFINE_GUID(CLSID_D2D1Composite, 0x48fc9f51, 0xf6ac, 0x48f1, 0x8b, 0x58, 0x3b, 0x28, 0xac, 0x46, 0xf7, 0x6d);
DEFINE_GUID(CLSID_D2D12DAffineTransform, 0x6AA97485, 0x6354, 0x4cfc, 0x90, 0x8C, 0xE4, 0xA7, 0x4F, 0x62, 0xC9, 0x6C);

namespace palladium {

void GPUEffects::apply_effect_in_place(GPUSurface& surface, ID2D1Effect* effect) {
    auto context = surface.get_context();
    int w = surface.get_width();
    int h = surface.get_height();
    
    // Create a temporary bitmap to hold the result
    D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );
    
    ComPtr<ID2D1Bitmap1> temp_bitmap;
    HRESULT hr = context->CreateBitmap(D2D1::SizeU(w, h), nullptr, 0, props, &temp_bitmap);
    if (FAILED(hr)) return;
    
    // Set temp as target and draw effect output
    ID2D1Image* old_target = nullptr;
    context->GetTarget(&old_target);
    
    context->SetTarget(temp_bitmap.Get());
    context->BeginDraw();
    context->Clear(D2D1::ColorF(0, 0, 0, 0));
    context->DrawImage(effect);
    context->EndDraw();
    
    // Copy result back to original bitmap
    context->SetTarget(old_target);
    if (old_target) old_target->Release();
    
    D2D1_POINT_2U dest_point = D2D1::Point2U(0, 0);
    surface.get_bitmap()->CopyFromBitmap(&dest_point, temp_bitmap.Get(), nullptr);
}

void GPUEffects::gaussian_blur(GPUSurface& surface, float radius) {
    if (radius <= 0.0f) return;
    
    // Clamp radius (Direct2D limit is ~250)
    radius = std::min(radius, 250.0f);
    
    auto context = surface.get_context();
    int w = surface.get_width();
    int h = surface.get_height();
    
    // Calculate padding to prevent edge artifacts
    // Use radius * 3 to ensure full Gaussian kernel coverage (3-sigma rule)
    int padding = static_cast<int>(std::ceil(radius * 3.0f));
    int padded_w = w + padding * 2;
    int padded_h = h + padding * 2;
    
    // Create Gaussian blur effect
    ComPtr<ID2D1Effect> blur_effect;
    HRESULT hr = context->CreateEffect(CLSID_D2D1GaussianBlur, &blur_effect);
    if (FAILED(hr)) {
        return;
    }
    
    // Create padded bitmap for blur input (allows blur to extend beyond original bounds)
    D2D1_BITMAP_PROPERTIES1 copy_props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );
    
    ComPtr<ID2D1Bitmap1> padded_bitmap;
    hr = context->CreateBitmap(D2D1::SizeU(padded_w, padded_h), nullptr, 0, copy_props, &padded_bitmap);
    if (FAILED(hr)) return;
    
    // Save old target
    ID2D1Image* old_target = nullptr;
    context->GetTarget(&old_target);
    
    // Draw original content centered in padded bitmap (with transparent padding)
    context->SetTarget(padded_bitmap.Get());
    context->BeginDraw();
    context->Clear(D2D1::ColorF(0, 0, 0, 0));  // Transparent padding
    
    // Create drawable copy of source for drawing
    D2D1_BITMAP_PROPERTIES1 draw_props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_NONE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );
    ComPtr<ID2D1Bitmap1> source_drawable;
    hr = context->CreateBitmap(D2D1::SizeU(w, h), nullptr, 0, draw_props, &source_drawable);
    if (SUCCEEDED(hr)) {
        D2D1_POINT_2U pt = D2D1::Point2U(0, 0);
        source_drawable->CopyFromBitmap(&pt, surface.get_bitmap(), nullptr);
        
        // Draw source centered in padded area
        D2D1_RECT_F dest_rect = D2D1::RectF(
            static_cast<float>(padding),
            static_cast<float>(padding),
            static_cast<float>(padding + w),
            static_cast<float>(padding + h)
        );
        context->DrawBitmap(source_drawable.Get(), dest_rect, 1.0f, D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
    }
    context->EndDraw();
    
    // Apply blur to padded bitmap
    blur_effect->SetInput(0, padded_bitmap.Get());
    blur_effect->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, radius / 3.0f);
    blur_effect->SetValue(D2D1_GAUSSIANBLUR_PROP_BORDER_MODE, D2D1_BORDER_MODE_SOFT);
    
    // Draw blurred result back to original surface (cropping to original size)
    context->SetTarget(surface.get_bitmap());
    context->BeginDraw();
    context->Clear(D2D1::ColorF(0, 0, 0, 0));
    
    // Draw only the center portion (excluding padding)
    D2D1_RECT_F src_rect = D2D1::RectF(
        static_cast<float>(padding),
        static_cast<float>(padding),
        static_cast<float>(padding + w),
        static_cast<float>(padding + h)
    );
    D2D1_RECT_F dst_rect = D2D1::RectF(0, 0, static_cast<float>(w), static_cast<float>(h));
    
    // Get effect output and draw cropped region
    ComPtr<ID2D1Image> effect_output;
    blur_effect->GetOutput(&effect_output);
    context->DrawImage(effect_output.Get(), D2D1::Point2F(static_cast<float>(-padding), static_cast<float>(-padding)));
    
    context->EndDraw();
    
    // Restore old target
    context->SetTarget(old_target);
    if (old_target) old_target->Release();
}

std::unique_ptr<GPUSurface> GPUEffects::gaussian_blur_copy(const GPUSurface& source, float radius) {
    auto result = std::make_unique<GPUSurface>(source.get_width(), source.get_height());
    
    // Copy source to result
    auto context = result->get_context();
    
    D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_NONE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );
    
    ComPtr<ID2D1Bitmap1> source_copy;
    HRESULT hr = context->CreateBitmap(
        D2D1::SizeU(source.get_width(), source.get_height()),
        nullptr, 0, props, &source_copy
    );
    if (SUCCEEDED(hr)) {
        D2D1_POINT_2U dest_point = D2D1::Point2U(0, 0);
        source_copy->CopyFromBitmap(&dest_point, source.get_bitmap(), nullptr);
        result->get_bitmap()->CopyFromBitmap(&dest_point, source_copy.Get(), nullptr);
    }
    
    gaussian_blur(*result, radius);
    return result;
}

void GPUEffects::drop_shadow(GPUSurface& surface, float offset_x, float offset_y,
                              float blur_radius, const nativeui::Color& color) {
    auto context = surface.get_context();
    
    // Create shadow effect
    ComPtr<ID2D1Effect> shadow_effect;
    HRESULT hr = context->CreateEffect(CLSID_D2D1Shadow, &shadow_effect);
    if (FAILED(hr)) return;
    
    // Create composite effect to combine shadow and original
    ComPtr<ID2D1Effect> composite_effect;
    hr = context->CreateEffect(CLSID_D2D1Composite, &composite_effect);
    if (FAILED(hr)) return;
    
    // Create transform effect for offset
    ComPtr<ID2D1Effect> transform_effect;
    hr = context->CreateEffect(CLSID_D2D12DAffineTransform, &transform_effect);
    if (FAILED(hr)) return;
    
    // Create drawable copy
    D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_NONE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );
    
    ComPtr<ID2D1Bitmap1> source_copy;
    hr = context->CreateBitmap(
        D2D1::SizeU(surface.get_width(), surface.get_height()),
        nullptr, 0, props, &source_copy
    );
    if (FAILED(hr)) return;
    
    D2D1_POINT_2U dest_point = D2D1::Point2U(0, 0);
    source_copy->CopyFromBitmap(&dest_point, surface.get_bitmap(), nullptr);
    
    // Configure shadow
    shadow_effect->SetInput(0, source_copy.Get());
    shadow_effect->SetValue(D2D1_SHADOW_PROP_BLUR_STANDARD_DEVIATION, blur_radius / 3.0f);
    shadow_effect->SetValue(D2D1_SHADOW_PROP_COLOR, D2D1::Vector4F(
        color.r / 255.0f,
        color.g / 255.0f,
        color.b / 255.0f,
        color.a / 255.0f
    ));
    
    // Configure offset
    D2D1_MATRIX_3X2_F matrix = D2D1::Matrix3x2F::Translation(offset_x, offset_y);
    transform_effect->SetInputEffect(0, shadow_effect.Get());
    transform_effect->SetValue(D2D1_2DAFFINETRANSFORM_PROP_TRANSFORM_MATRIX, matrix);
    
    // Composite: shadow behind original
    composite_effect->SetInputEffect(0, transform_effect.Get());
    composite_effect->SetInput(1, source_copy.Get());
    composite_effect->SetValue(D2D1_COMPOSITE_PROP_MODE, D2D1_COMPOSITE_MODE_SOURCE_OVER);
    
    apply_effect_in_place(surface, composite_effect.Get());
}

void GPUEffects::saturation(GPUSurface& surface, float saturation_amount) {
    auto context = surface.get_context();
    
    ComPtr<ID2D1Effect> sat_effect;
    HRESULT hr = context->CreateEffect(CLSID_D2D1Saturation, &sat_effect);
    if (FAILED(hr)) return;
    
    D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_NONE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );
    
    ComPtr<ID2D1Bitmap1> source_copy;
    hr = context->CreateBitmap(
        D2D1::SizeU(surface.get_width(), surface.get_height()),
        nullptr, 0, props, &source_copy
    );
    if (FAILED(hr)) return;
    
    D2D1_POINT_2U dest_point = D2D1::Point2U(0, 0);
    source_copy->CopyFromBitmap(&dest_point, surface.get_bitmap(), nullptr);
    
    sat_effect->SetInput(0, source_copy.Get());
    sat_effect->SetValue(D2D1_SATURATION_PROP_SATURATION, saturation_amount);
    
    apply_effect_in_place(surface, sat_effect.Get());
}

void GPUEffects::brightness(GPUSurface& surface, float brightness_amount) {
    auto context = surface.get_context();
    
    ComPtr<ID2D1Effect> brightness_effect;
    HRESULT hr = context->CreateEffect(CLSID_D2D1Brightness, &brightness_effect);
    if (FAILED(hr)) return;
    
    D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_NONE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );
    
    ComPtr<ID2D1Bitmap1> source_copy;
    hr = context->CreateBitmap(
        D2D1::SizeU(surface.get_width(), surface.get_height()),
        nullptr, 0, props, &source_copy
    );
    if (FAILED(hr)) return;
    
    D2D1_POINT_2U dest_point = D2D1::Point2U(0, 0);
    source_copy->CopyFromBitmap(&dest_point, surface.get_bitmap(), nullptr);
    
    brightness_effect->SetInput(0, source_copy.Get());
    // D2D brightness uses white point/black point
    float white = 1.0f + brightness_amount;
    float black = brightness_amount > 0 ? 0.0f : -brightness_amount;
    brightness_effect->SetValue(D2D1_BRIGHTNESS_PROP_WHITE_POINT, D2D1::Vector2F(white, 1.0f));
    brightness_effect->SetValue(D2D1_BRIGHTNESS_PROP_BLACK_POINT, D2D1::Vector2F(black, 0.0f));
    
    apply_effect_in_place(surface, brightness_effect.Get());
}

void GPUEffects::tint(GPUSurface& surface, const nativeui::Color& color) {
    auto context = surface.get_context();
    
    // Use color matrix effect for tinting
    ComPtr<ID2D1Effect> color_matrix;
    HRESULT hr = context->CreateEffect(CLSID_D2D1ColorMatrix, &color_matrix);
    if (FAILED(hr)) return;
    
    D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_NONE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );
    
    ComPtr<ID2D1Bitmap1> source_copy;
    hr = context->CreateBitmap(
        D2D1::SizeU(surface.get_width(), surface.get_height()),
        nullptr, 0, props, &source_copy
    );
    if (FAILED(hr)) return;
    
    D2D1_POINT_2U dest_point = D2D1::Point2U(0, 0);
    source_copy->CopyFromBitmap(&dest_point, surface.get_bitmap(), nullptr);
    
    color_matrix->SetInput(0, source_copy.Get());
    
    // Create tint matrix (multiply RGB by tint color)
    float r = color.r / 255.0f;
    float g = color.g / 255.0f;
    float b = color.b / 255.0f;
    float a = color.a / 255.0f;
    
    D2D1_MATRIX_5X4_F matrix = {
        r, 0, 0, 0,
        0, g, 0, 0,
        0, 0, b, 0,
        0, 0, 0, a,
        0, 0, 0, 0
    };
    
    color_matrix->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, matrix);
    
    apply_effect_in_place(surface, color_matrix.Get());
}

} // namespace palladium

#endif // _WIN32
