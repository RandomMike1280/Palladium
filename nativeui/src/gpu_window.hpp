#pragma once

#ifdef _WIN32

#include "d2d_context.hpp"
#include "gpu_surface.hpp"
#include "window.hpp"
#include <SDL2/SDL_syswm.h>

namespace palladium {

/**
 * GPUWindow - Hardware-accelerated window using Direct2D/DXGI
 * 
 * This window uses a DXGI swap chain for efficient GPU rendering.
 * Content is rendered directly on the GPU without CPU-GPU transfers.
 */
class GPUWindow {
public:
    GPUWindow(const std::string& title, int width, int height, bool vsync = true);
    ~GPUWindow();
    
    // Non-copyable
    GPUWindow(const GPUWindow&) = delete;
    GPUWindow& operator=(const GPUWindow&) = delete;
    
    // Window properties
    int get_width() const { return width_; }
    int get_height() const { return height_; }
    const std::string& get_title() const { return title_; }
    void set_title(const std::string& title);
    bool is_open() const { return is_open_; }
    
    // Event handling (delegates to SDL)
    bool poll_event(nativeui::Event& event);
    void wait_event(nativeui::Event& event);
    
    // GPU rendering
    void begin_draw();
    void end_draw();
    void clear(const nativeui::Color& color = nativeui::Color(0, 0, 0, 255));
    void present();
    
    // Draw GPU surfaces
    void draw(const GPUSurface& surface, int x = 0, int y = 0, float opacity = 1.0f);
    void draw_scaled(const GPUSurface& surface, int x, int y, int w, int h, float opacity = 1.0f);
    
    // Direct2D context access for advanced rendering
    ID2D1DeviceContext* get_context() const { return context_.Get(); }
    
    // Frame timing
    float get_delta_time() const { return delta_time_; }
    float get_fps() const { return fps_; }
    void set_target_fps(int fps);
    void set_unfocused_fps(int fps);
    
    // Window state
    bool is_focused() const;
    bool is_minimized() const;
    
    // Cursor
    void set_cursor_visible(bool visible);
    
    // Fullscreen
    void set_fullscreen(bool fullscreen);
    bool is_fullscreen() const { return is_fullscreen_; }
    
    // Close
    void close() { is_open_ = false; }

private:
    std::string title_;
    int width_;
    int height_;
    bool is_open_;
    bool is_fullscreen_;
    bool is_drawing_ = false;
    
    // SDL window (for cross-platform window management)
    SDL_Window* sdl_window_;
    HWND hwnd_;
    
    // DXGI swap chain
    ComPtr<IDXGISwapChain1> swap_chain_;
    
    // D2D rendering
    ComPtr<ID2D1DeviceContext> context_;
    ComPtr<ID2D1Bitmap1> target_bitmap_;
    
    // Timing
    uint64_t last_frame_time_;
    float delta_time_;
    float fps_;
    int target_fps_;
    int unfocused_fps_;
    
    void create_swap_chain();
    void create_render_target();
    void update_timing();
    nativeui::Event translate_event(const SDL_Event& sdl_event);
};

} // namespace palladium

#endif // _WIN32
