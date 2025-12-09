#include "surface.hpp"
#include <cmath>

namespace nativeui {

Surface::Surface(int width, int height)
    : width_(width), height_(height), pixels_(width * height * 4, 0)
{
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("Surface dimensions must be positive");
    }
}

Surface::Surface(const Surface& other)
    : width_(other.width_), height_(other.height_), pixels_(other.pixels_)
{
}

Surface& Surface::operator=(const Surface& other)
{
    if (this != &other) {
        width_ = other.width_;
        height_ = other.height_;
        pixels_ = other.pixels_;
    }
    return *this;
}

void Surface::set_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    if (!in_bounds(x, y)) return;
    
    size_t offset = pixel_offset(x, y);
    pixels_[offset] = r;
    pixels_[offset + 1] = g;
    pixels_[offset + 2] = b;
    pixels_[offset + 3] = a;
}

void Surface::set_pixel(int x, int y, const Color& color)
{
    set_pixel(x, y, color.r, color.g, color.b, color.a);
}

void Surface::blend_pixel(int x, int y, const Color& color)
{
    if (!in_bounds(x, y) || color.a == 0) return;
    
    if (color.a == 255) {
        set_pixel(x, y, color);
        return;
    }
    
    Color dst = get_pixel(x, y);
    float alpha = color.a / 255.0f;
    float inv_alpha = 1.0f - alpha;
    
    uint8_t r = static_cast<uint8_t>(color.r * alpha + dst.r * inv_alpha);
    uint8_t g = static_cast<uint8_t>(color.g * alpha + dst.g * inv_alpha);
    uint8_t b = static_cast<uint8_t>(color.b * alpha + dst.b * inv_alpha);
    uint8_t a = static_cast<uint8_t>(std::min(255.0f, color.a + dst.a * inv_alpha));
    
    set_pixel(x, y, r, g, b, a);
}

void Surface::plot_aa_pixel(int x, int y, const Color& color, float brightness)
{
    if (brightness <= 0.0f) return;
    brightness = std::min(brightness, 1.0f);
    uint8_t aa_alpha = static_cast<uint8_t>(color.a * brightness);
    blend_pixel(x, y, Color(color.r, color.g, color.b, aa_alpha));
}

Color Surface::get_pixel(int x, int y) const
{
    if (!in_bounds(x, y)) return Color(0, 0, 0, 0);
    
    size_t offset = pixel_offset(x, y);
    return Color(
        pixels_[offset],
        pixels_[offset + 1],
        pixels_[offset + 2],
        pixels_[offset + 3]
    );
}

void Surface::fill(const Color& color)
{
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            set_pixel(x, y, color);
        }
    }
}

void Surface::fill_rect(int x, int y, int w, int h, const Color& color)
{
    int x1 = std::max(0, x);
    int y1 = std::max(0, y);
    int x2 = std::min(width_, x + w);
    int y2 = std::min(height_, y + h);
    
    for (int py = y1; py < y2; ++py) {
        for (int px = x1; px < x2; ++px) {
            set_pixel(px, py, color);
        }
    }
}

void Surface::clear()
{
    std::fill(pixels_.begin(), pixels_.end(), 0);
}

// ============ Drawing with auto-AA dispatch ============

void Surface::draw_line(int x1, int y1, int x2, int y2, const Color& color)
{
    if (AntiAliasingSettings::instance().is_enabled()) {
        draw_line_aa(x1, y1, x2, y2, color);
    } else {
        draw_line_no_aa(x1, y1, x2, y2, color);
    }
}

void Surface::draw_circle(int cx, int cy, int radius, const Color& color)
{
    if (AntiAliasingSettings::instance().is_enabled()) {
        draw_circle_aa(cx, cy, radius, color);
    } else {
        draw_circle_no_aa(cx, cy, radius, color);
    }
}

void Surface::fill_circle(int cx, int cy, int radius, const Color& color)
{
    if (AntiAliasingSettings::instance().is_enabled()) {
        fill_circle_aa(cx, cy, radius, color);
    } else {
        fill_circle_no_aa(cx, cy, radius, color);
    }
}

// ============ Non-AA implementations ============

void Surface::draw_line_no_aa(int x1, int y1, int x2, int y2, const Color& color)
{
    // Bresenham's line algorithm
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx - dy;
    
    while (true) {
        set_pixel(x1, y1, color);
        
        if (x1 == x2 && y1 == y2) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void Surface::draw_circle_no_aa(int cx, int cy, int radius, const Color& color)
{
    // Midpoint circle algorithm
    int x = radius;
    int y = 0;
    int err = 0;
    
    while (x >= y) {
        set_pixel(cx + x, cy + y, color);
        set_pixel(cx + y, cy + x, color);
        set_pixel(cx - y, cy + x, color);
        set_pixel(cx - x, cy + y, color);
        set_pixel(cx - x, cy - y, color);
        set_pixel(cx - y, cy - x, color);
        set_pixel(cx + y, cy - x, color);
        set_pixel(cx + x, cy - y, color);
        
        y++;
        if (err <= 0) {
            err += 2 * y + 1;
        }
        if (err > 0) {
            x--;
            err -= 2 * x + 1;
        }
    }
}

void Surface::fill_circle_no_aa(int cx, int cy, int radius, const Color& color)
{
    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            if (x * x + y * y <= radius * radius) {
                set_pixel(cx + x, cy + y, color);
            }
        }
    }
}

// ============ Anti-aliased implementations ============

void Surface::draw_line_aa(int x1, int y1, int x2, int y2, const Color& color)
{
    // Xiaolin Wu's line algorithm
    auto fpart = [](float x) { return x - std::floor(x); };
    auto rfpart = [&fpart](float x) { return 1.0f - fpart(x); };
    
    bool steep = std::abs(y2 - y1) > std::abs(x2 - x1);
    
    if (steep) {
        std::swap(x1, y1);
        std::swap(x2, y2);
    }
    if (x1 > x2) {
        std::swap(x1, x2);
        std::swap(y1, y2);
    }
    
    float dx = static_cast<float>(x2 - x1);
    float dy = static_cast<float>(y2 - y1);
    float gradient = (dx == 0.0f) ? 1.0f : dy / dx;
    
    // Handle first endpoint
    float xend = std::round(static_cast<float>(x1));
    float yend = y1 + gradient * (xend - x1);
    float xgap = rfpart(x1 + 0.5f);
    int xpxl1 = static_cast<int>(xend);
    int ypxl1 = static_cast<int>(std::floor(yend));
    
    if (steep) {
        plot_aa_pixel(ypxl1, xpxl1, color, rfpart(yend) * xgap);
        plot_aa_pixel(ypxl1 + 1, xpxl1, color, fpart(yend) * xgap);
    } else {
        plot_aa_pixel(xpxl1, ypxl1, color, rfpart(yend) * xgap);
        plot_aa_pixel(xpxl1, ypxl1 + 1, color, fpart(yend) * xgap);
    }
    
    float intery = yend + gradient;
    
    // Handle second endpoint
    xend = std::round(static_cast<float>(x2));
    yend = y2 + gradient * (xend - x2);
    xgap = fpart(x2 + 0.5f);
    int xpxl2 = static_cast<int>(xend);
    int ypxl2 = static_cast<int>(std::floor(yend));
    
    if (steep) {
        plot_aa_pixel(ypxl2, xpxl2, color, rfpart(yend) * xgap);
        plot_aa_pixel(ypxl2 + 1, xpxl2, color, fpart(yend) * xgap);
    } else {
        plot_aa_pixel(xpxl2, ypxl2, color, rfpart(yend) * xgap);
        plot_aa_pixel(xpxl2, ypxl2 + 1, color, fpart(yend) * xgap);
    }
    
    // Main loop
    for (int x = xpxl1 + 1; x < xpxl2; ++x) {
        int ipart = static_cast<int>(std::floor(intery));
        float f = fpart(intery);
        
        if (steep) {
            plot_aa_pixel(ipart, x, color, 1.0f - f);
            plot_aa_pixel(ipart + 1, x, color, f);
        } else {
            plot_aa_pixel(x, ipart, color, 1.0f - f);
            plot_aa_pixel(x, ipart + 1, color, f);
        }
        intery += gradient;
    }
}

void Surface::draw_circle_aa(int cx, int cy, int radius, const Color& color)
{
    // Anti-aliased circle using distance-based alpha
    float r = static_cast<float>(radius);
    
    for (int y = -radius - 1; y <= radius + 1; ++y) {
        for (int x = -radius - 1; x <= radius + 1; ++x) {
            float dist = std::sqrt(static_cast<float>(x * x + y * y));
            float diff = std::abs(dist - r);
            
            if (diff < 1.5f) {
                float alpha = 1.0f - diff;
                alpha = std::clamp(alpha, 0.0f, 1.0f);
                plot_aa_pixel(cx + x, cy + y, color, alpha);
            }
        }
    }
}

void Surface::fill_circle_aa(int cx, int cy, int radius, const Color& color)
{
    // Anti-aliased filled circle with smooth edge
    float r = static_cast<float>(radius);
    
    for (int y = -radius - 1; y <= radius + 1; ++y) {
        for (int x = -radius - 1; x <= radius + 1; ++x) {
            float dist = std::sqrt(static_cast<float>(x * x + y * y));
            
            if (dist <= r - 1.0f) {
                // Inside - full opacity
                blend_pixel(cx + x, cy + y, color);
            } else if (dist <= r + 1.0f) {
                // Edge - anti-aliased
                float alpha = (r + 1.0f - dist) / 2.0f;
                alpha = std::clamp(alpha, 0.0f, 1.0f);
                plot_aa_pixel(cx + x, cy + y, color, alpha);
            }
        }
    }
}

void Surface::draw_rect(int x, int y, int w, int h, const Color& color)
{
    // Use AA lines if enabled
    if (AntiAliasingSettings::instance().is_enabled()) {
        draw_line_aa(x, y, x + w - 1, y, color);           // Top
        draw_line_aa(x, y + h - 1, x + w - 1, y + h - 1, color); // Bottom
        draw_line_aa(x, y, x, y + h - 1, color);           // Left
        draw_line_aa(x + w - 1, y, x + w - 1, y + h - 1, color); // Right
    } else {
        // Top and bottom
        for (int px = x; px < x + w; ++px) {
            set_pixel(px, y, color);
            set_pixel(px, y + h - 1, color);
        }
        // Left and right
        for (int py = y; py < y + h; ++py) {
            set_pixel(x, py, color);
            set_pixel(x + w - 1, py, color);
        }
    }
}

// ============ Blitting ============

void Surface::blit(const Surface& source, int dest_x, int dest_y)
{
    for (int sy = 0; sy < source.height_; ++sy) {
        for (int sx = 0; sx < source.width_; ++sx) {
            int dx = dest_x + sx;
            int dy = dest_y + sy;
            
            if (!in_bounds(dx, dy)) continue;
            
            Color src_color = source.get_pixel(sx, sy);
            
            if (src_color.a == 255) {
                set_pixel(dx, dy, src_color);
            } else if (src_color.a > 0) {
                blend_pixel(dx, dy, src_color);
            }
        }
    }
}

void Surface::blit_scaled(const Surface& source, int dest_x, int dest_y, int dest_w, int dest_h)
{
    float scale_x = static_cast<float>(source.width_) / dest_w;
    float scale_y = static_cast<float>(source.height_) / dest_h;
    
    for (int dy = 0; dy < dest_h; ++dy) {
        for (int dx = 0; dx < dest_w; ++dx) {
            int px = dest_x + dx;
            int py = dest_y + dy;
            
            if (!in_bounds(px, py)) continue;
            
            int sx = static_cast<int>(dx * scale_x);
            int sy = static_cast<int>(dy * scale_y);
            
            Color src_color = source.get_pixel(sx, sy);
            
            if (src_color.a == 255) {
                set_pixel(px, py, src_color);
            } else if (src_color.a > 0) {
                blend_pixel(px, py, src_color);
            }
        }
    }
}

void Surface::blit_alpha(const Surface& source, int dest_x, int dest_y, float alpha)
{
    alpha = std::clamp(alpha, 0.0f, 1.0f);
    
    for (int sy = 0; sy < source.height_; ++sy) {
        for (int sx = 0; sx < source.width_; ++sx) {
            int dx = dest_x + sx;
            int dy = dest_y + sy;
            
            if (!in_bounds(dx, dy)) continue;
            
            Color src_color = source.get_pixel(sx, sy);
            uint8_t effective_alpha = static_cast<uint8_t>(src_color.a * alpha);
            blend_pixel(dx, dy, Color(src_color.r, src_color.g, src_color.b, effective_alpha));
        }
    }
}

std::shared_ptr<Surface> Surface::copy() const
{
    return std::make_shared<Surface>(*this);
}

std::shared_ptr<Surface> Surface::subsurface(int x, int y, int w, int h) const
{
    auto result = std::make_shared<Surface>(w, h);
    
    for (int py = 0; py < h; ++py) {
        for (int px = 0; px < w; ++px) {
            Color color = get_pixel(x + px, y + py);
            result->set_pixel(px, py, color);
        }
    }
    
    return result;
}


// Advanced Shapes

void Surface::draw_round_rect(int x, int y, int w, int h, int radius, const Color& color)
{
    // Clamp radius
    int max_radius = std::min(w, h) / 2;
    radius = std::clamp(radius, 0, max_radius);
    
    if (radius <= 0) {
        draw_rect(x, y, w, h, color);
        return;
    }
    
    // Draw 4 lines
    draw_line(x + radius, y, x + w - radius, y, color);         // Top
    draw_line(x + radius, y + h - 1, x + w - radius, y + h - 1, color); // Bottom
    draw_line(x, y + radius, x, y + h - radius, color);         // Left
    draw_line(x + w - 1, y + radius, x + w - 1, y + h - radius, color); // Right
    
    // Fallback for corners: simple circles for now (imperfect outline overlap but valid)
    // Draw 4 corners using draw_circle_aa logic helper?
    // Let's implement quadrants properly by masking?
    // For simplicity, we use draw_circle but clip? No.
    // Let's implement a manual quadrant drawer.
    
    auto draw_quadrant = [&](int cx, int cy, int r, int q) {
        // q: 0=TR, 1=BR, 2=BL, 3=TL
        int x0 = 0;
        int y0 = r;
        int d = 3 - 2 * r;
        
        while (y0 >= x0) {
            // Plot points based on quadrant
            auto plot = [&](int px, int py) {
                 if (q == 0) blend_pixel(cx + px, cy - py, color);
                 if (q == 1) blend_pixel(cx + px, cy + py, color);
                 if (q == 2) blend_pixel(cx - px, cy + py, color);
                 if (q == 3) blend_pixel(cx - px, cy - py, color);
            };
            
            plot(x0, y0);
            plot(y0, x0); // Symmetry
            
            if (d < 0) d = d + 4 * x0 + 6;
            else {
                d = d + 4 * (x0 - y0) + 10;
                y0--;
            }
            x0++;
        }
    };
    
    // Top-Right
    draw_quadrant(x + w - radius - 1, y + radius, radius, 0);
    // Bottom-Right
    draw_quadrant(x + w - radius - 1, y + h - radius - 1, radius, 1);
    // Bottom-Left
    draw_quadrant(x + radius, y + h - radius - 1, radius, 2);
    // Top-Left
    draw_quadrant(x + radius, y + radius, radius, 3);
}

void Surface::fill_round_rect(int x, int y, int w, int h, int radius, const Color& color)
{
    int max_radius = std::min(w, h) / 2;
    radius = std::clamp(radius, 0, max_radius);
    
    if (radius <= 0) {
        fill_rect(x, y, w, h, color);
        return;
    }
    
    // SDF Implementation for Rounded Rect (prevents overdraw)
    // d = length(max(abs(p) - b, 0.0)) - r
    float rx = static_cast<float>(radius);
    float half_w = w * 0.5f;
    float half_h = h * 0.5f;
    float cx = x + half_w;
    float cy = y + half_h;
    float box_w = half_w - rx;
    float box_h = half_h - rx;
    
    int min_x = std::max(0, x - 1);
    int max_x = std::min(width_, x + w + 1);
    int min_y = std::max(0, y - 1);
    int max_y = std::min(height_, y + h + 1);
    
    bool aa = AntiAliasingSettings::instance().is_enabled();
    
    for (int py = min_y; py < max_y; ++py) {
        for (int px = min_x; px < max_x; ++px) {
            // Point relative to center
            float px_rel = std::abs(px - cx + 0.5f);
            float py_rel = std::abs(py - cy + 0.5f);
            
            // sdRoundedBox
            float qx = px_rel - box_w;
            float qy = py_rel - box_h;
            
            // Distance to surface (negative = inside)
            float d = std::min(std::max(qx, qy), 0.0f) + 
                      std::sqrt(std::max(qx, 0.0f)*std::max(qx, 0.0f) + std::max(qy, 0.0f)*std::max(qy, 0.0f)) - 
                      rx;
            
            if (d <= -0.5f) {
                // Inside
                blend_pixel(px, py, color);
            } else if (aa && d < 0.5f) {
                // Edge (AA)
                float alpha_f = 0.5f - d; // 0.0 to 1.0
                alpha_f = std::clamp(alpha_f, 0.0f, 1.0f);
                Color c = color;
                c.a = static_cast<uint8_t>(c.a * alpha_f);
                blend_pixel(px, py, c);
            }
        }
    }
}

void Surface::fill_pill(int x, int y, int w, int h, const Color& color)
{
    int radius = std::min(w, h) / 2;
    fill_round_rect(x, y, w, h, radius, color);
}

void Surface::draw_pill(int x, int y, int w, int h, const Color& color)
{
    int radius = std::min(w, h) / 2;
    draw_round_rect(x, y, w, h, radius, color);
}

void Surface::fill_squircle(int x, int y, int w, int h, const Color& color)
{
    if (AntiAliasingSettings::instance().is_enabled()) {
        // Anti-aliased implementation using SDF
        float a = w * 0.5f;
        float b = h * 0.5f;
        float cx = x + a;
        float cy = y + b;
        
        int min_x = x; int max_x = x + w;
        int min_y = y; int max_y = y + h;
        
        // Expand bounds slightly for AA
        min_x = std::max(0, min_x - 1);
        max_x = std::min(width_, max_x + 1);
        min_y = std::max(0, min_y - 1);
        max_y = std::min(height_, max_y + 1);
        
        for (int py = min_y; py < max_y; ++py) {
            for (int px = min_x; px < max_x; ++px) {
                float dx = std::abs(px - cx + 0.5f);
                float dy = std::abs(py - cy + 0.5f);
                
                // Implicit function F = (dx/a)^4 + (dy/b)^4
                // We want F <= 1 for inside.
                
                float x_term = dx/a;
                float y_term = dy/b;
                float P = x_term*x_term*x_term*x_term + y_term*y_term*y_term*y_term;
                
                // Calculate signed distance approx
                // Gradient of F: <4x^3/a^4, 4y^3/b^4>
                // We normalized terms already as dx/a, need to be careful with chain rule.
                // dP/dx = 4 * (dx/a)^3 * (1/a)
                // dP/dy = 4 * (dy/b)^3 * (1/b)
                
                float gx = 4.0f * (x_term*x_term*x_term) / a;
                float gy = 4.0f * (y_term*y_term*y_term) / b;
                float grad_len = std::sqrt(gx*gx + gy*gy);
                
                // Signed distance ~ (P - 1) / |Grad|
                float signed_dist = (P - 1.0f) / (grad_len + 1e-6f);
                
                if (signed_dist <= -0.5f) {
                    // Fully inside
                    blend_pixel(px, py, color);
                } else if (signed_dist < 0.5f) {
                    // Boundary - Anti-alias
                    float alpha_factor = 0.5f - signed_dist; // Map ranges [-0.5, 0.5] to [1.0, 0.0]
                    alpha_factor = std::clamp(alpha_factor, 0.0f, 1.0f);
                    
                    if (alpha_factor > 0.0f) {
                        uint8_t final_alpha = static_cast<uint8_t>(color.a * alpha_factor);
                        blend_pixel(px, py, Color(color.r, color.g, color.b, final_alpha));
                    }
                }
            }
        }
    } else {
        // Aliased scanline implementation
        // Superellipse: |x/a|^4 + |y/b|^4 <= 1
        float a = w * 0.5f;
        float b = h * 0.5f;
        float cx = x + a;
        float cy = y + b;
        
        if (a <= 0 || b <= 0) return;
        
        int min_y = std::max(0, y);
        int max_y = std::min(height_, y + h);
        int min_x = std::max(0, x);
        int max_x = std::min(width_, x + w);
        
        for (int py = min_y; py < max_y; ++py) {
            float dy = std::abs(py - cy + 0.5f);
            if (dy >= b) continue;
            
            float y_term = dy / b;
            float y_term_4 = y_term * y_term * y_term * y_term;
            if (y_term_4 > 1.0f) y_term_4 = 1.0f;
            
            float x_term = std::pow(1.0f - y_term_4, 0.25f);
            float dx = a * x_term;
            
            int start_x = static_cast<int>(std::floor(cx - dx));
            int end_x = static_cast<int>(std::ceil(cx + dx));
            start_x = std::clamp(start_x, min_x, max_x);
            end_x = std::clamp(end_x, min_x, max_x);
            
            for (int px = start_x; px < end_x; ++px) {
                 set_pixel(px, py, color);
            }
        }
    }
}

void Surface::draw_squircle(int x, int y, int w, int h, const Color& color)
{
    float a = w * 0.5f;
    float b = h * 0.5f;
    float cx = x + a;
    float cy = y + b;
    
    int min_x = x-1; int max_x = x+w+1;
    int min_y = y-1; int max_y = y+h+1;
    
    for (int py = min_y; py < max_y; ++py) {
        for (int px = min_x; px < max_x; ++px) {
             float dx = std::abs(px - cx + 0.5f);
             float dy = std::abs(py - cy + 0.5f);
             
             // Implicit function F = (dx/a)^4 + (dy/b)^4
             float P = std::pow(dx/a, 4.0f) + std::pow(dy/b, 4.0f);
             
             // Gradient mag approx
             float grad_x = 4.0f * std::pow(dx/a, 3.0f) / a;
             float grad_y = 4.0f * std::pow(dy/b, 3.0f) / b;
             float grad_len = std::sqrt(grad_x*grad_x + grad_y*grad_y);
             
             // Dist approx
             float dist_approx = std::abs(P - 1.0f) / (grad_len + 1e-6f);
             
             if (dist_approx < 1.0f) {
                 float alpha = 1.0f - dist_approx;
                 blend_pixel(px, py, color.with_alpha(static_cast<uint8_t>(color.a * alpha)));
             }
        }
    }
}

} // namespace nativeui

