#include "effects.hpp"
#include <cmath>

namespace nativeui {

std::mt19937& Effects::get_rng()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return gen;
}

void Effects::horizontal_box_blur(Surface& surface, int radius)
{
    int width = surface.get_width();
    int height = surface.get_height();
    
    std::vector<uint8_t> temp(width * height * 4);
    const uint8_t* src = surface.get_data();
    
    int kernel_size = 2 * radius + 1;
    float inv_kernel = 1.0f / kernel_size;
    
    for (int y = 0; y < height; ++y) {
        // Initialize accumulator with left edge padding
        int r_sum = 0, g_sum = 0, b_sum = 0, a_sum = 0;
        
        for (int i = -radius; i <= radius; ++i) {
            int x = std::max(0, std::min(width - 1, i));
            size_t offset = (y * width + x) * 4;
            r_sum += src[offset];
            g_sum += src[offset + 1];
            b_sum += src[offset + 2];
            a_sum += src[offset + 3];
        }
        
        for (int x = 0; x < width; ++x) {
            size_t dst_offset = (y * width + x) * 4;
            temp[dst_offset] = static_cast<uint8_t>(r_sum * inv_kernel);
            temp[dst_offset + 1] = static_cast<uint8_t>(g_sum * inv_kernel);
            temp[dst_offset + 2] = static_cast<uint8_t>(b_sum * inv_kernel);
            temp[dst_offset + 3] = static_cast<uint8_t>(a_sum * inv_kernel);
            
            // Slide window
            int left_x = std::max(0, x - radius);
            int right_x = std::min(width - 1, x + radius + 1);
            
            size_t left_offset = (y * width + left_x) * 4;
            size_t right_offset = (y * width + right_x) * 4;
            
            r_sum += src[right_offset] - src[left_offset];
            g_sum += src[right_offset + 1] - src[left_offset + 1];
            b_sum += src[right_offset + 2] - src[left_offset + 2];
            a_sum += src[right_offset + 3] - src[left_offset + 3];
        }
    }
    
    // Copy back to surface
    std::memcpy(surface.get_data(), temp.data(), temp.size());
}

void Effects::vertical_box_blur(Surface& surface, int radius)
{
    int width = surface.get_width();
    int height = surface.get_height();
    
    std::vector<uint8_t> temp(width * height * 4);
    const uint8_t* src = surface.get_data();
    
    int kernel_size = 2 * radius + 1;
    float inv_kernel = 1.0f / kernel_size;
    
    for (int x = 0; x < width; ++x) {
        int r_sum = 0, g_sum = 0, b_sum = 0, a_sum = 0;
        
        for (int i = -radius; i <= radius; ++i) {
            int y = std::max(0, std::min(height - 1, i));
            size_t offset = (y * width + x) * 4;
            r_sum += src[offset];
            g_sum += src[offset + 1];
            b_sum += src[offset + 2];
            a_sum += src[offset + 3];
        }
        
        for (int y = 0; y < height; ++y) {
            size_t dst_offset = (y * width + x) * 4;
            temp[dst_offset] = static_cast<uint8_t>(r_sum * inv_kernel);
            temp[dst_offset + 1] = static_cast<uint8_t>(g_sum * inv_kernel);
            temp[dst_offset + 2] = static_cast<uint8_t>(b_sum * inv_kernel);
            temp[dst_offset + 3] = static_cast<uint8_t>(a_sum * inv_kernel);
            
            int top_y = std::max(0, y - radius);
            int bottom_y = std::min(height - 1, y + radius + 1);
            
            size_t top_offset = (top_y * width + x) * 4;
            size_t bottom_offset = (bottom_y * width + x) * 4;
            
            r_sum += src[bottom_offset] - src[top_offset];
            g_sum += src[bottom_offset + 1] - src[top_offset + 1];
            b_sum += src[bottom_offset + 2] - src[top_offset + 2];
            a_sum += src[bottom_offset + 3] - src[top_offset + 3];
        }
    }
    
    std::memcpy(surface.get_data(), temp.data(), temp.size());
}

void Effects::box_blur(Surface& surface, int radius)
{
    if (radius <= 0) return;
    
    // Separable box blur (two 1D passes)
    horizontal_box_blur(surface, radius);
    vertical_box_blur(surface, radius);
}

std::vector<float> Effects::generate_gaussian_kernel(float sigma)
{
    int radius = static_cast<int>(std::ceil(sigma * 3));
    int size = 2 * radius + 1;
    std::vector<float> kernel(size);
    
    float sum = 0.0f;
    float inv_sigma_sq_2 = 1.0f / (2.0f * sigma * sigma);
    
    for (int i = 0; i < size; ++i) {
        int x = i - radius;
        kernel[i] = std::exp(-x * x * inv_sigma_sq_2);
        sum += kernel[i];
    }
    
    // Normalize
    for (float& val : kernel) {
        val /= sum;
    }
    
    return kernel;
}

void Effects::gaussian_blur(Surface& surface, float sigma)
{
    if (sigma <= 0.0f) return;
    
    // Use multi-pass box blur as approximation
    // More passes = smoother gradient, reduces banding at high blur radii
    int radius = static_cast<int>(std::ceil(sigma));
    
    // Scale passes based on blur radius for quality
    // Small blur: 3 passes, Large blur: up to 6 passes
    int passes = 3 + std::min(3, static_cast<int>(sigma / 10.0f));
    
    // Adjust radius per pass to maintain effective blur strength
    // Total variance = passes * (radius^2 / 3), so adjust accordingly
    float adjusted_radius = sigma / std::sqrt(passes / 3.0f);
    int blur_radius = std::max(1, static_cast<int>(std::ceil(adjusted_radius)));
    
    for (int i = 0; i < passes; ++i) {
        box_blur(surface, blur_radius);
    }
}

void Effects::blur_region(Surface& surface, int x, int y, int w, int h, int radius)
{
    auto region = surface.subsurface(x, y, w, h);
    box_blur(*region, radius);
    
    for (int py = 0; py < h; ++py) {
        for (int px = 0; px < w; ++px) {
            surface.set_pixel(x + px, y + py, region->get_pixel(px, py));
        }
    }
}

void Effects::frosted_glass(Surface& surface, int blur_radius, float noise_amount, float sat)
{
    // Apply blur
    gaussian_blur(surface, static_cast<float>(blur_radius));
    
    // Add noise
    noise(surface, noise_amount);
    
    // Adjust saturation
    saturation(surface, sat);
}

void Effects::frosted_glass_region(Surface& surface, int x, int y, int w, int h, int blur_radius)
{
    auto region = surface.subsurface(x, y, w, h);
    frosted_glass(*region, blur_radius);
    
    for (int py = 0; py < h; ++py) {
        for (int px = 0; px < w; ++px) {
            surface.set_pixel(x + px, y + py, region->get_pixel(px, py));
        }
    }
}

void Effects::displace(Surface& surface, const Surface& displacement_map, float strength)
{
    int width = surface.get_width();
    int height = surface.get_height();
    
    auto original = surface.copy();
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Color disp = displacement_map.get_pixel(x, y);
            
            // Use R for X displacement, G for Y displacement
            float dx = ((disp.r / 255.0f) - 0.5f) * 2.0f * strength;
            float dy = ((disp.g / 255.0f) - 0.5f) * 2.0f * strength;
            
            int src_x = std::clamp(static_cast<int>(x + dx), 0, width - 1);
            int src_y = std::clamp(static_cast<int>(y + dy), 0, height - 1);
            
            surface.set_pixel(x, y, original->get_pixel(src_x, src_y));
        }
    }
}

void Effects::wave_distort(Surface& surface, float amplitude, float frequency, float phase)
{
    int width = surface.get_width();
    int height = surface.get_height();
    
    auto original = surface.copy();
    
    for (int y = 0; y < height; ++y) {
        float offset = amplitude * std::sin(frequency * y + phase);
        
        for (int x = 0; x < width; ++x) {
            int src_x = std::clamp(static_cast<int>(x + offset), 0, width - 1);
            surface.set_pixel(x, y, original->get_pixel(src_x, y));
        }
    }
}

void Effects::ripple(Surface& surface, int center_x, int center_y, float amplitude, float wavelength, float phase)
{
    int width = surface.get_width();
    int height = surface.get_height();
    
    auto original = surface.copy();
    float two_pi_over_wavelength = 6.28318530718f / wavelength;
    
    // Bilinear interpolation helper
    auto sample_bilinear = [&](float fx, float fy) -> Color {
        // Clamp to valid range
        fx = std::clamp(fx, 0.0f, static_cast<float>(width - 1));
        fy = std::clamp(fy, 0.0f, static_cast<float>(height - 1));
        
        int x0 = static_cast<int>(std::floor(fx));
        int y0 = static_cast<int>(std::floor(fy));
        int x1 = std::min(x0 + 1, width - 1);
        int y1 = std::min(y0 + 1, height - 1);
        
        float tx = fx - x0;
        float ty = fy - y0;
        
        Color c00 = original->get_pixel(x0, y0);
        Color c10 = original->get_pixel(x1, y0);
        Color c01 = original->get_pixel(x0, y1);
        Color c11 = original->get_pixel(x1, y1);
        
        // Bilinear interpolate each channel
        auto lerp = [](float a, float b, float t) { return a + t * (b - a); };
        
        float r = lerp(lerp(c00.r, c10.r, tx), lerp(c01.r, c11.r, tx), ty);
        float g = lerp(lerp(c00.g, c10.g, tx), lerp(c01.g, c11.g, tx), ty);
        float b = lerp(lerp(c00.b, c10.b, tx), lerp(c01.b, c11.b, tx), ty);
        float a = lerp(lerp(c00.a, c10.a, tx), lerp(c01.a, c11.a, tx), ty);
        
        return Color(
            static_cast<uint8_t>(std::clamp(r, 0.0f, 255.0f)),
            static_cast<uint8_t>(std::clamp(g, 0.0f, 255.0f)),
            static_cast<uint8_t>(std::clamp(b, 0.0f, 255.0f)),
            static_cast<uint8_t>(std::clamp(a, 0.0f, 255.0f))
        );
    };
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float dx = static_cast<float>(x - center_x);
            float dy = static_cast<float>(y - center_y);
            float distance = std::sqrt(dx * dx + dy * dy);
            
            if (distance > 0.0f) {
                float factor = amplitude * std::sin(distance * two_pi_over_wavelength + phase);
                float src_x = x + dx / distance * factor;
                float src_y = y + dy / distance * factor;
                
                // Use bilinear interpolation for smooth sampling
                surface.set_pixel(x, y, sample_bilinear(src_x, src_y));
            }
        }
    }
}

void Effects::brightness(Surface& surface, float amount)
{
    int width = surface.get_width();
    int height = surface.get_height();
    int adjustment = static_cast<int>(amount * 255);
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Color c = surface.get_pixel(x, y);
            c.r = static_cast<uint8_t>(std::clamp(c.r + adjustment, 0, 255));
            c.g = static_cast<uint8_t>(std::clamp(c.g + adjustment, 0, 255));
            c.b = static_cast<uint8_t>(std::clamp(c.b + adjustment, 0, 255));
            surface.set_pixel(x, y, c);
        }
    }
}

void Effects::contrast(Surface& surface, float amount)
{
    int width = surface.get_width();
    int height = surface.get_height();
    float factor = (259.0f * (amount * 255.0f + 255.0f)) / (255.0f * (259.0f - amount * 255.0f));
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Color c = surface.get_pixel(x, y);
            c.r = static_cast<uint8_t>(std::clamp(factor * (c.r - 128) + 128, 0.0f, 255.0f));
            c.g = static_cast<uint8_t>(std::clamp(factor * (c.g - 128) + 128, 0.0f, 255.0f));
            c.b = static_cast<uint8_t>(std::clamp(factor * (c.b - 128) + 128, 0.0f, 255.0f));
            surface.set_pixel(x, y, c);
        }
    }
}

void Effects::saturation(Surface& surface, float amount)
{
    int width = surface.get_width();
    int height = surface.get_height();
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Color c = surface.get_pixel(x, y);
            float gray = 0.299f * c.r + 0.587f * c.g + 0.114f * c.b;
            
            c.r = static_cast<uint8_t>(std::clamp(gray + amount * (c.r - gray), 0.0f, 255.0f));
            c.g = static_cast<uint8_t>(std::clamp(gray + amount * (c.g - gray), 0.0f, 255.0f));
            c.b = static_cast<uint8_t>(std::clamp(gray + amount * (c.b - gray), 0.0f, 255.0f));
            surface.set_pixel(x, y, c);
        }
    }
}

void Effects::hue_shift(Surface& surface, float degrees)
{
    int width = surface.get_width();
    int height = surface.get_height();
    
    float rad = degrees * 3.14159265358979f / 180.0f;
    float cos_h = std::cos(rad);
    float sin_h = std::sin(rad);
    
    // Rotation matrix for hue shift in YIQ color space
    float m[3][3] = {
        {0.299f + 0.701f * cos_h + 0.168f * sin_h, 0.587f - 0.587f * cos_h + 0.330f * sin_h, 0.114f - 0.114f * cos_h - 0.497f * sin_h},
        {0.299f - 0.299f * cos_h - 0.328f * sin_h, 0.587f + 0.413f * cos_h + 0.035f * sin_h, 0.114f - 0.114f * cos_h + 0.292f * sin_h},
        {0.299f - 0.300f * cos_h + 1.250f * sin_h, 0.587f - 0.588f * cos_h - 1.050f * sin_h, 0.114f + 0.886f * cos_h - 0.203f * sin_h}
    };
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Color c = surface.get_pixel(x, y);
            float r = c.r, g = c.g, b = c.b;
            
            c.r = static_cast<uint8_t>(std::clamp(m[0][0] * r + m[0][1] * g + m[0][2] * b, 0.0f, 255.0f));
            c.g = static_cast<uint8_t>(std::clamp(m[1][0] * r + m[1][1] * g + m[1][2] * b, 0.0f, 255.0f));
            c.b = static_cast<uint8_t>(std::clamp(m[2][0] * r + m[2][1] * g + m[2][2] * b, 0.0f, 255.0f));
            surface.set_pixel(x, y, c);
        }
    }
}

void Effects::invert(Surface& surface)
{
    int width = surface.get_width();
    int height = surface.get_height();
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Color c = surface.get_pixel(x, y);
            c.r = 255 - c.r;
            c.g = 255 - c.g;
            c.b = 255 - c.b;
            surface.set_pixel(x, y, c);
        }
    }
}

void Effects::grayscale(Surface& surface)
{
    saturation(surface, 0.0f);
}

void Effects::sepia(Surface& surface, float strength)
{
    int width = surface.get_width();
    int height = surface.get_height();
    float inv_strength = 1.0f - strength;
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Color c = surface.get_pixel(x, y);
            float r = c.r, g = c.g, b = c.b;
            
            float sepia_r = std::min(255.0f, 0.393f * r + 0.769f * g + 0.189f * b);
            float sepia_g = std::min(255.0f, 0.349f * r + 0.686f * g + 0.168f * b);
            float sepia_b = std::min(255.0f, 0.272f * r + 0.534f * g + 0.131f * b);
            
            c.r = static_cast<uint8_t>(r * inv_strength + sepia_r * strength);
            c.g = static_cast<uint8_t>(g * inv_strength + sepia_g * strength);
            c.b = static_cast<uint8_t>(b * inv_strength + sepia_b * strength);
            surface.set_pixel(x, y, c);
        }
    }
}

void Effects::blend(Surface& dest, const Surface& source, float alpha)
{
    int width = std::min(dest.get_width(), source.get_width());
    int height = std::min(dest.get_height(), source.get_height());
    float inv_alpha = 1.0f - alpha;
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Color d = dest.get_pixel(x, y);
            Color s = source.get_pixel(x, y);
            
            d.r = static_cast<uint8_t>(d.r * inv_alpha + s.r * alpha);
            d.g = static_cast<uint8_t>(d.g * inv_alpha + s.g * alpha);
            d.b = static_cast<uint8_t>(d.b * inv_alpha + s.b * alpha);
            dest.set_pixel(x, y, d);
        }
    }
}

void Effects::linear_gradient(Surface& surface, int x1, int y1, int x2, int y2,
                               const Color& color1, const Color& color2)
{
    int width = surface.get_width();
    int height = surface.get_height();
    
    float dx = static_cast<float>(x2 - x1);
    float dy = static_cast<float>(y2 - y1);
    float len_sq = dx * dx + dy * dy;
    
    if (len_sq == 0.0f) {
        surface.fill(color1);
        return;
    }
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float t = ((x - x1) * dx + (y - y1) * dy) / len_sq;
            t = std::clamp(t, 0.0f, 1.0f);
            
            Color c;
            c.r = static_cast<uint8_t>(color1.r + (color2.r - color1.r) * t);
            c.g = static_cast<uint8_t>(color1.g + (color2.g - color1.g) * t);
            c.b = static_cast<uint8_t>(color1.b + (color2.b - color1.b) * t);
            c.a = static_cast<uint8_t>(color1.a + (color2.a - color1.a) * t);
            surface.set_pixel(x, y, c);
        }
    }
}

void Effects::radial_gradient(Surface& surface, int cx, int cy, int radius,
                               const Color& inner_color, const Color& outer_color)
{
    int width = surface.get_width();
    int height = surface.get_height();
    float radius_f = static_cast<float>(radius);
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float dx = static_cast<float>(x - cx);
            float dy = static_cast<float>(y - cy);
            float distance = std::sqrt(dx * dx + dy * dy);
            float t = std::min(distance / radius_f, 1.0f);
            
            Color c;
            c.r = static_cast<uint8_t>(inner_color.r + (outer_color.r - inner_color.r) * t);
            c.g = static_cast<uint8_t>(inner_color.g + (outer_color.g - inner_color.g) * t);
            c.b = static_cast<uint8_t>(inner_color.b + (outer_color.b - inner_color.b) * t);
            c.a = static_cast<uint8_t>(inner_color.a + (outer_color.a - inner_color.a) * t);
            surface.set_pixel(x, y, c);
        }
    }
}

void Effects::noise(Surface& surface, float amount)
{
    int width = surface.get_width();
    int height = surface.get_height();
    
    std::uniform_int_distribution<int> dist(-128, 127);
    auto& rng = get_rng();
    
    int noise_intensity = static_cast<int>(amount * 255);
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Color c = surface.get_pixel(x, y);
            int noise_val = (dist(rng) * noise_intensity) / 127;
            
            c.r = static_cast<uint8_t>(std::clamp(c.r + noise_val, 0, 255));
            c.g = static_cast<uint8_t>(std::clamp(c.g + noise_val, 0, 255));
            c.b = static_cast<uint8_t>(std::clamp(c.b + noise_val, 0, 255));
            surface.set_pixel(x, y, c);
        }
    }
}

void Effects::perlin_noise(Surface& surface, float scale, int octaves)
{
    // Simple Perlin-like noise implementation
    int width = surface.get_width();
    int height = surface.get_height();
    
    auto& rng = get_rng();
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    // Generate random gradient vectors for a grid
    int grid_size = 64;
    std::vector<std::pair<float, float>> gradients(grid_size * grid_size);
    for (auto& grad : gradients) {
        float angle = dist(rng) * 6.28318530718f;
        grad.first = std::cos(angle);
        grad.second = std::sin(angle);
    }
    
    auto dot_grid_gradient = [&](int ix, int iy, float x, float y) {
        float dx = x - ix;
        float dy = y - iy;
        int idx = ((iy % grid_size) * grid_size + (ix % grid_size));
        return dx * gradients[idx].first + dy * gradients[idx].second;
    };
    
    auto lerp = [](float a, float b, float t) { return a + t * (b - a); };
    auto fade = [](float t) { return t * t * t * (t * (t * 6 - 15) + 10); };
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float value = 0.0f;
            float amplitude = 1.0f;
            float frequency = 1.0f;
            float max_value = 0.0f;
            
            for (int o = 0; o < octaves; ++o) {
                float nx = x * scale * frequency / width;
                float ny = y * scale * frequency / height;
                
                int x0 = static_cast<int>(std::floor(nx));
                int x1 = x0 + 1;
                int y0 = static_cast<int>(std::floor(ny));
                int y1 = y0 + 1;
                
                float sx = fade(nx - x0);
                float sy = fade(ny - y0);
                
                float n00 = dot_grid_gradient(x0, y0, nx, ny);
                float n10 = dot_grid_gradient(x1, y0, nx, ny);
                float n01 = dot_grid_gradient(x0, y1, nx, ny);
                float n11 = dot_grid_gradient(x1, y1, nx, ny);
                
                float ix0 = lerp(n00, n10, sx);
                float ix1 = lerp(n01, n11, sx);
                float noise_value = lerp(ix0, ix1, sy);
                
                value += noise_value * amplitude;
                max_value += amplitude;
                amplitude *= 0.5f;
                frequency *= 2.0f;
            }
            
            value = (value / max_value + 1.0f) * 0.5f;
            uint8_t gray = static_cast<uint8_t>(std::clamp(value * 255, 0.0f, 255.0f));
            surface.set_pixel(x, y, Color(gray, gray, gray, 255));
        }
    }
}

std::shared_ptr<Surface> Effects::drop_shadow(const Surface& source, int offset_x, int offset_y,
                                                int blur_radius, const Color& shadow_color)
{
    int width = source.get_width() + std::abs(offset_x) + blur_radius * 2;
    int height = source.get_height() + std::abs(offset_y) + blur_radius * 2;
    
    auto result = std::make_shared<Surface>(width, height);
    result->fill(Color(0, 0, 0, 0));
    
    // Draw shadow
    int shadow_x = std::max(0, offset_x) + blur_radius;
    int shadow_y = std::max(0, offset_y) + blur_radius;
    
    for (int y = 0; y < source.get_height(); ++y) {
        for (int x = 0; x < source.get_width(); ++x) {
            Color src = source.get_pixel(x, y);
            if (src.a > 0) {
                Color shadow = shadow_color;
                shadow.a = static_cast<uint8_t>((shadow_color.a * src.a) / 255);
                result->set_pixel(shadow_x + x, shadow_y + y, shadow);
            }
        }
    }
    
    // Blur shadow
    gaussian_blur(*result, static_cast<float>(blur_radius));
    
    // Draw original on top
    int src_x = std::max(0, -offset_x) + blur_radius;
    int src_y = std::max(0, -offset_y) + blur_radius;
    result->blit(source, src_x, src_y);
    
    return result;
}

// ============ BlurredSurface Implementation ============

BlurredSurface::BlurredSurface(int width, int height)
    : surface_(std::make_shared<Surface>(width, height))
    , current_radius_(0.0f)
    , animating_(false)
    , start_radius_(0.0f)
    , target_radius_(0.0f)
    , duration_(0.0f)
    , elapsed_(0.0f)
    , easing_type_(0)
{
}

BlurredSurface::BlurredSurface(std::shared_ptr<Surface> surface)
    : surface_(surface)
    , current_radius_(0.0f)
    , animating_(false)
    , start_radius_(0.0f)
    , target_radius_(0.0f)
    , duration_(0.0f)
    , elapsed_(0.0f)
    , easing_type_(0)
{
}

void BlurredSurface::set_blur_radius(float radius)
{
    current_radius_ = std::max(0.0f, radius);
    animating_ = false;
}

void BlurredSurface::animate_blur_radius(float target_radius, float duration, int easing_type)
{
    if (duration <= 0.0f) {
        set_blur_radius(target_radius);
        return;
    }
    
    start_radius_ = current_radius_;
    target_radius_ = std::max(0.0f, target_radius);
    duration_ = duration;
    elapsed_ = 0.0f;
    easing_type_ = easing_type;
    animating_ = true;
}

float BlurredSurface::apply_easing(float t) const
{
    t = std::clamp(t, 0.0f, 1.0f);
    
    switch (easing_type_) {
        case 0:  // Linear
            return t;
        case 1:  // EaseInQuad
            return t * t;
        case 2:  // EaseOutQuad
            return t * (2.0f - t);
        case 3:  // EaseInOutQuad
            return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
        case 4:  // EaseInCubic
            return t * t * t;
        case 5:  // EaseOutCubic
            { float f = t - 1.0f; return f * f * f + 1.0f; }
        case 6:  // EaseInOutCubic
            return t < 0.5f ? 4.0f * t * t * t : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;
        case 7:  // EaseInExpo
            return t == 0.0f ? 0.0f : std::pow(2.0f, 10.0f * (t - 1.0f));
        case 8:  // EaseOutExpo
            return t == 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);
        case 9:  // EaseInOutExpo
            if (t == 0.0f) return 0.0f;
            if (t == 1.0f) return 1.0f;
            if (t < 0.5f) return 0.5f * std::pow(2.0f, 20.0f * t - 10.0f);
            return 1.0f - 0.5f * std::pow(2.0f, -20.0f * t + 10.0f);
        default:
            return t;
    }
}

void BlurredSurface::update(float dt)
{
    if (!animating_) return;
    
    elapsed_ += dt;
    
    if (elapsed_ >= duration_) {
        current_radius_ = target_radius_;
        animating_ = false;
        return;
    }
    
    float t = elapsed_ / duration_;
    float eased_t = apply_easing(t);
    current_radius_ = start_radius_ + (target_radius_ - start_radius_) * eased_t;
}

std::shared_ptr<Surface> BlurredSurface::render() const
{
    if (current_radius_ <= 0.5f) {
        // No blur, just return a copy
        return surface_->copy();
    }
    
    // Calculate padding needed for blur (3x sigma is typical for gaussian)
    int padding = static_cast<int>(std::ceil(current_radius_ * 3.0f));
    int new_width = surface_->get_width() + padding * 2;
    int new_height = surface_->get_height() + padding * 2;
    
    // Create expanded surface with transparent background
    auto result = std::make_shared<Surface>(new_width, new_height);
    result->fill(Color(0, 0, 0, 0));  // Transparent
    
    // Blit original surface centered
    result->blit(*surface_, padding, padding);
    
    // Apply blur to the expanded surface
    Effects::gaussian_blur(*result, current_radius_);
    
    return result;
}

void BlurredSurface::render_to(Surface& dest, int x, int y) const
{
    auto blurred = render();
    
    // Offset by padding to keep centered at requested position
    if (current_radius_ > 0.5f) {
        int padding = static_cast<int>(std::ceil(current_radius_ * 3.0f));
        dest.blit(*blurred, x - padding, y - padding);
    } else {
        dest.blit(*blurred, x, y);
    }
}

} // namespace nativeui

