#include "layer.hpp"
#include "effects.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace nativeui {

Layer::Layer(int width, int height)
    : surface_(std::make_shared<Surface>(width, height))
    , x_(0), y_(0)
    , scale_x_(1.0f), scale_y_(1.0f)
    , rotation_(0.0f)
    , opacity_(1.0f)
    , visible_(true)
    , blend_mode_(BlendMode::Normal)
    , material_(Material::solid())
    , name_("")
{
}

Layer::Layer(std::shared_ptr<Surface> surface)
    : surface_(surface)
    , x_(0), y_(0)
    , scale_x_(1.0f), scale_y_(1.0f)
    , rotation_(0.0f)
    , opacity_(1.0f)
    , visible_(true)
    , blend_mode_(BlendMode::Normal)
    , material_(Material::solid())
    , name_("")
{
}

bool Layer::hit_test(int x, int y)
{
    // Default hit test: Bounding box check
    // If rotation is 0, simple AABB
    if (rotation_ == 0.0f) {
        return x >= x_ && x < x_ + surface_->get_width() * scale_x_ &&
               y >= y_ && y < y_ + surface_->get_height() * scale_y_;
    }
    
    // If rotated, we should inverse transform point to local space
    // Simple approach for now: Check AABB of rotated layer?
    // Or just treat as non-rotated for v1?
    // Let's stick to AABB of the non-rotated rect for simplicity in this iteration unless easy to fix.
    // Actually, inverse transform is easy:
    // Translate -> Rotate
    // Inverse: Rotate(-a) -> Translate(-pos)
    
    // Center relative
    float cx = x_ + surface_->get_width() * scale_x_ * 0.5f;
    float cy = y_ + surface_->get_height() * scale_y_ * 0.5f;
    
    float dx = x - cx;
    float dy = y - cy;
    
    // Rotate point by -angle
    float rad = -rotation_ * 3.14159f / 180.0f;
    float c = std::cos(rad);
    float s = std::sin(rad);
    
    float nx = dx * c - dy * s;
    float ny = dx * s + dy * c;
    
    // New point relative to center. Check against half-extents.
    float hw = surface_->get_width() * scale_x_ * 0.5f;
    float hh = surface_->get_height() * scale_y_ * 0.5f;
    
    return std::abs(nx) <= hw && std::abs(ny) <= hh;
}

// LayerStack implementation

LayerStack::LayerStack(int width, int height)
    : width_(width)
    , height_(height)
    , background_(Color(0, 0, 0, 255))
    , composite_surface_(std::make_shared<Surface>(width, height))
{
}

std::shared_ptr<Layer> LayerStack::create_layer(const std::string& name)
{
    auto layer = std::make_shared<Layer>(width_, height_);
    layer->set_name(name);
    layers_.push_back(layer);
    return layer;
}

std::shared_ptr<Layer> LayerStack::create_layer_from_surface(std::shared_ptr<Surface> surface, const std::string& name)
{
    auto layer = std::make_shared<Layer>(surface);
    layer->set_name(name);
    layers_.push_back(layer);
    return layer;
}

void LayerStack::add_layer(std::shared_ptr<Layer> layer)
{
    layers_.push_back(layer);
}

void LayerStack::remove_layer(std::shared_ptr<Layer> layer)
{
    layers_.erase(
        std::remove(layers_.begin(), layers_.end(), layer),
        layers_.end()
    );
}

void LayerStack::remove_layer(size_t index)
{
    if (index < layers_.size()) {
        layers_.erase(layers_.begin() + index);
    }
}

void LayerStack::clear_layers()
{
    layers_.clear();
}

std::shared_ptr<Layer> LayerStack::get_layer(size_t index)
{
    if (index >= layers_.size()) {
        return nullptr;
    }
    return layers_[index];
}

std::shared_ptr<Layer> LayerStack::get_layer_by_name(const std::string& name)
{
    for (auto& layer : layers_) {
        if (layer->get_name() == name) {
            return layer;
        }
    }
    return nullptr;
}

void LayerStack::move_layer_up(std::shared_ptr<Layer> layer)
{
    for (size_t i = 0; i < layers_.size() - 1; ++i) {
        if (layers_[i] == layer) {
            std::swap(layers_[i], layers_[i + 1]);
            return;
        }
    }
}

void LayerStack::move_layer_down(std::shared_ptr<Layer> layer)
{
    for (size_t i = 1; i < layers_.size(); ++i) {
        if (layers_[i] == layer) {
            std::swap(layers_[i], layers_[i - 1]);
            return;
        }
    }
}

void LayerStack::move_layer_to_top(std::shared_ptr<Layer> layer)
{
    remove_layer(layer);
    layers_.push_back(layer);
}

void LayerStack::move_layer_to_bottom(std::shared_ptr<Layer> layer)
{
    remove_layer(layer);
    layers_.insert(layers_.begin(), layer);
}

void LayerStack::set_layer_index(std::shared_ptr<Layer> layer, size_t new_index)
{
    remove_layer(layer);
    if (new_index > layers_.size()) {
        new_index = layers_.size();
    }
    layers_.insert(layers_.begin() + new_index, layer);
}

Color LayerStack::blend_pixels(const Color& bottom, const Color& top, BlendMode mode, float opacity)
{
    float alpha = (top.a / 255.0f) * opacity;
    float inv_alpha = 1.0f - alpha;
    
    float br = bottom.r / 255.0f;
    float bg = bottom.g / 255.0f;
    float bb = bottom.b / 255.0f;
    float tr = top.r / 255.0f;
    float tg = top.g / 255.0f;
    float tb = top.b / 255.0f;
    
    float rr, rg, rb;
    
    switch (mode) {
        case BlendMode::Normal:
            rr = tr;
            rg = tg;
            rb = tb;
            break;
            
        case BlendMode::Multiply:
            rr = br * tr;
            rg = bg * tg;
            rb = bb * tb;
            break;
            
        case BlendMode::Screen:
            rr = 1.0f - (1.0f - br) * (1.0f - tr);
            rg = 1.0f - (1.0f - bg) * (1.0f - tg);
            rb = 1.0f - (1.0f - bb) * (1.0f - tb);
            break;
            
        case BlendMode::Overlay:
            rr = br < 0.5f ? 2.0f * br * tr : 1.0f - 2.0f * (1.0f - br) * (1.0f - tr);
            rg = bg < 0.5f ? 2.0f * bg * tg : 1.0f - 2.0f * (1.0f - bg) * (1.0f - tg);
            rb = bb < 0.5f ? 2.0f * bb * tb : 1.0f - 2.0f * (1.0f - bb) * (1.0f - tb);
            break;
            
        case BlendMode::Add:
            rr = std::min(1.0f, br + tr);
            rg = std::min(1.0f, bg + tg);
            rb = std::min(1.0f, bb + tb);
            break;
            
        case BlendMode::Subtract:
            rr = std::max(0.0f, br - tr);
            rg = std::max(0.0f, bg - tg);
            rb = std::max(0.0f, bb - tb);
            break;
            
        case BlendMode::Difference:
            rr = std::abs(br - tr);
            rg = std::abs(bg - tg);
            rb = std::abs(bb - tb);
            break;
            
        case BlendMode::ColorDodge:
            rr = tr >= 1.0f ? 1.0f : std::min(1.0f, br / (1.0f - tr));
            rg = tg >= 1.0f ? 1.0f : std::min(1.0f, bg / (1.0f - tg));
            rb = tb >= 1.0f ? 1.0f : std::min(1.0f, bb / (1.0f - tb));
            break;
            
        case BlendMode::ColorBurn:
            rr = tr <= 0.0f ? 0.0f : std::max(0.0f, 1.0f - (1.0f - br) / tr);
            rg = tg <= 0.0f ? 0.0f : std::max(0.0f, 1.0f - (1.0f - bg) / tg);
            rb = tb <= 0.0f ? 0.0f : std::max(0.0f, 1.0f - (1.0f - bb) / tb);
            break;
            
        default:
            rr = tr;
            rg = tg;
            rb = tb;
    }
    
    // Apply alpha blending
    return Color(
        static_cast<uint8_t>(std::clamp((rr * alpha + br * inv_alpha) * 255.0f, 0.0f, 255.0f)),
        static_cast<uint8_t>(std::clamp((rg * alpha + bg * inv_alpha) * 255.0f, 0.0f, 255.0f)),
        static_cast<uint8_t>(std::clamp((rb * alpha + bb * inv_alpha) * 255.0f, 0.0f, 255.0f)),
        static_cast<uint8_t>(std::clamp((alpha + (bottom.a / 255.0f) * inv_alpha) * 255.0f, 0.0f, 255.0f))
    );
}

std::shared_ptr<Surface> LayerStack::composite()
{
    composite_to(*composite_surface_);
    return composite_surface_;
}

void LayerStack::composite_to(Surface& dest)
{
    // Fill with background
    dest.fill(background_);
    
    // Composite each layer
    for (const auto& layer : layers_) {
        if (!layer->is_visible() || layer->get_opacity() <= 0.0f) {
            continue;
        }
        
        const Surface& src = layer->get_surface();
        int lx = layer->get_x();
        int ly = layer->get_y();
        float opacity = layer->get_opacity();
        BlendMode blend_mode = layer->get_blend_mode();
        auto material = layer->get_material();
        
        // Calculate Scaled/Rotated Render State
        float scale_x = layer->get_scale_x();
        float scale_y = layer->get_scale_y();
        int scaled_w = static_cast<int>(src.get_width() * scale_x);
        int scaled_h = static_cast<int>(src.get_height() * scale_y);
        
        // Center Pivot Calculation
        int offset_x = (src.get_width() - scaled_w) / 2;
        int offset_y = (src.get_height() - scaled_h) / 2;
        
        int draw_x = lx + offset_x;
        int draw_y = ly + offset_y;
        
        // Apply frosted glass effect BEFORE blitting this layer
        if (material && material->is_frosted_glass() && material->get_blur_radius() > 0.5f) {
             // Pass the source surface for masking, with scaling params
            apply_frosted_glass(dest, draw_x, draw_y, scaled_w, scaled_h,
                               src, scale_x, scale_y, material->get_blur_radius());
        }
        
        // Render
        if (scale_x == 1.0f && scale_y == 1.0f && layer->get_rotation() == 0.0f) {
            // Optimized unscaled path
             for (int sy = 0; sy < src.get_height(); ++sy) {
                for (int sx = 0; sx < src.get_width(); ++sx) {
                    int dx = lx + sx;
                    int dy = ly + sy;
                    
                    if (dx < 0 || dx >= width_ || dy < 0 || dy >= height_) continue;
                    
                    Color src_color = src.get_pixel(sx, sy);
                    if (src_color.a == 0) continue;
                    
                    Color dst_color = dest.get_pixel(dx, dy);
                    Color blended = blend_pixels(dst_color, src_color, blend_mode, opacity);
                    dest.set_pixel(dx, dy, blended);
                }
            }
        } else {
             // Scaled path with BILINEAR interpolation for AA preservation
            int src_w = src.get_width();
            int src_h = src.get_height();
            
            for (int dy = 0; dy < scaled_h; ++dy) {
                for (int dx = 0; dx < scaled_w; ++dx) {
                    int dest_px = draw_x + dx;
                    int dest_py = draw_y + dy;
                    
                    if (dest_px < 0 || dest_px >= width_ || dest_py < 0 || dest_py >= height_) continue;
                    
                    // Calculate floating-point source coordinates
                    float src_xf = dx / scale_x;
                    float src_yf = dy / scale_y;
                    
                    // Bilinear interpolation
                    int x0 = static_cast<int>(src_xf);
                    int y0 = static_cast<int>(src_yf);
                    int x1 = std::min(x0 + 1, src_w - 1);
                    int y1 = std::min(y0 + 1, src_h - 1);
                    
                    float fx = src_xf - x0;
                    float fy = src_yf - y0;
                    
                    // Sample 4 neighboring pixels
                    Color c00 = src.get_pixel(x0, y0);
                    Color c10 = src.get_pixel(x1, y0);
                    Color c01 = src.get_pixel(x0, y1);
                    Color c11 = src.get_pixel(x1, y1);
                    
                    // Interpolate
                    auto lerp_channel = [](uint8_t a, uint8_t b, float t) -> uint8_t {
                        return static_cast<uint8_t>(a + (b - a) * t);
                    };
                    
                    Color top(
                        lerp_channel(c00.r, c10.r, fx),
                        lerp_channel(c00.g, c10.g, fx),
                        lerp_channel(c00.b, c10.b, fx),
                        lerp_channel(c00.a, c10.a, fx)
                    );
                    Color bottom(
                        lerp_channel(c01.r, c11.r, fx),
                        lerp_channel(c01.g, c11.g, fx),
                        lerp_channel(c01.b, c11.b, fx),
                        lerp_channel(c01.a, c11.a, fx)
                    );
                    Color src_color(
                        lerp_channel(top.r, bottom.r, fy),
                        lerp_channel(top.g, bottom.g, fy),
                        lerp_channel(top.b, bottom.b, fy),
                        lerp_channel(top.a, bottom.a, fy)
                    );
                    
                    if (src_color.a == 0) continue;
                    
                    Color dst_color = dest.get_pixel(dest_px, dest_py);
                    Color blended = blend_pixels(dst_color, src_color, blend_mode, opacity);
                    dest.set_pixel(dest_px, dest_py, blended);
                }
            }
        }
    }
}

void LayerStack::apply_frosted_glass(Surface& dest, int x, int y, int w, int h, 
                                     const Surface& mask, float scale_x, float scale_y, 
                                     float blur_radius)
{
    // 1. Calculate padded bounds to avoid edge artifacts
    // Blur radius * 3 is standard for Gaussian kernel coverage
    int padding = static_cast<int>(std::ceil(blur_radius * 3.0f));
    
    // Padded region in destination coordinates
    int pad_x = x - padding;
    int pad_y = y - padding;
    int pad_w = w + padding * 2;
    int pad_h = h + padding * 2;
    
    // Create temporary surface for blurring
    Surface padded_surface(pad_w, pad_h);
    
    // Copy pixels from dest to padded_surface
    // Note: get_pixel handles out-of-bounds by returning transparent (0,0,0,0)
    for (int py = 0; py < pad_h; ++py) {
        for (int px = 0; px < pad_w; ++px) {
            padded_surface.set_pixel(px, py, dest.get_pixel(pad_x + px, pad_y + py));
        }
    }
    
    // Apply Gaussian Blur to the padded surface
    Effects::gaussian_blur(padded_surface, blur_radius);
    
    // Threshold: only apply blur where mask alpha is significant
    const uint8_t alpha_threshold = 10;

    // Determine write bounds (clamped to destination)
    int start_x = std::max(0, x);
    int start_y = std::max(0, y);
    int end_x = std::min(dest.get_width(), x + w);
    int end_y = std::min(dest.get_height(), y + h);

    // Copy blurred region back, masking by alpha
    for (int dest_y = start_y; dest_y < end_y; ++dest_y) {
        for (int dest_x = start_x; dest_x < end_x; ++dest_x) {
            // Local coords relative to the unpadded layer rect
            int local_x = dest_x - x;
            int local_y = dest_y - y;
            
            // Map to mask surface coords
            int src_x = static_cast<int>(local_x / scale_x);
            int src_y = static_cast<int>(local_y / scale_y);
            
            // Clamp to mask bounds
            src_x = std::max(0, std::min(mask.get_width() - 1, src_x));
            src_y = std::max(0, std::min(mask.get_height() - 1, src_y));
            
            uint8_t mask_alpha = mask.get_pixel(src_x, src_y).a;
            
            if (mask_alpha >= alpha_threshold) {
                 Color orig = dest.get_pixel(dest_x, dest_y);
                 
                 // Sample from PADDED blurred surface
                 // Pixel corresponding to local_x is at offset padding in padded_surface
                 Color blurred = padded_surface.get_pixel(local_x + padding, local_y + padding);
                 
                 // Smooth transition
                 // Map alpha 10..35 to 0..1 blur opacity (Full blur at alpha 35+)
                 float t = (static_cast<float>(mask_alpha) - alpha_threshold) / 25.0f;
                 if (t < 0.0f) t = 0.0f;
                 if (t > 1.0f) t = 1.0f;
                 
                 Color result(
                     static_cast<uint8_t>(orig.r + (blurred.r - orig.r) * t),
                     static_cast<uint8_t>(orig.g + (blurred.g - orig.g) * t),
                     static_cast<uint8_t>(orig.b + (blurred.b - orig.b) * t),
                     orig.a
                 );
                 
                 dest.set_pixel(dest_x, dest_y, result);
            }
        }
    }
}

} // namespace nativeui
