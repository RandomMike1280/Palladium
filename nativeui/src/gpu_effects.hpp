#pragma once

#ifdef _WIN32

#include "gpu_surface.hpp"
#include <d2d1effects.h>

namespace palladium {

/**
 * GPUEffects - Hardware-accelerated image effects using Direct2D
 * 
 * These effects run entirely on the GPU and are significantly
 * faster than CPU equivalents, especially for large surfaces or radii.
 */
class GPUEffects {
public:
    /**
     * Apply Gaussian blur to a GPU surface
     * @param surface The GPU surface to blur (modified in place)
     * @param radius Blur radius in pixels (0-250)
     */
    static void gaussian_blur(GPUSurface& surface, float radius);
    
    /**
     * Apply Gaussian blur and return new surface (source unchanged)
     * @param source Source GPU surface
     * @param radius Blur radius in pixels
     * @return New blurred GPU surface
     */
    static std::unique_ptr<GPUSurface> gaussian_blur_copy(const GPUSurface& source, float radius);
    
    /**
     * Apply drop shadow effect
     * @param surface Surface to add shadow to
     * @param offset_x Horizontal shadow offset
     * @param offset_y Vertical shadow offset
     * @param blur_radius Shadow blur radius
     * @param color Shadow color
     */
    static void drop_shadow(GPUSurface& surface, float offset_x, float offset_y, 
                            float blur_radius, const nativeui::Color& color);
    
    /**
     * Adjust saturation
     * @param surface Surface to adjust
     * @param saturation 0.0 = grayscale, 1.0 = original, >1.0 = oversaturated
     */
    static void saturation(GPUSurface& surface, float saturation);
    
    /**
     * Adjust brightness
     * @param surface Surface to adjust
     * @param brightness -1.0 to 1.0 (0.0 = no change)
     */
    static void brightness(GPUSurface& surface, float brightness);
    
    /**
     * Apply tint/color overlay
     * @param surface Surface to tint
     * @param color Tint color
     */
    static void tint(GPUSurface& surface, const nativeui::Color& color);
    
private:
    // Helper to apply an effect and render back to the same surface
    static void apply_effect_in_place(GPUSurface& surface, ID2D1Effect* effect);
};

} // namespace palladium

#endif // _WIN32
