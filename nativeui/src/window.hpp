#pragma once

#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <SDL2/SDL.h>
#include "surface.hpp"

namespace nativeui {

/**
 * Event types
 */
enum class EventType {
    None,
    Quit,
    KeyDown,
    KeyUp,
    MouseButtonDown,
    MouseButtonUp,
    MouseMotion,
    MouseWheel,
    TextInput
};

/**
 * Event structure
 */
struct Event {
    EventType type = EventType::None;
    
    // Key event data
    int key = 0;
    bool ctrl = false;
    bool shift = false;
    bool alt = false;
    
    // Text input data
    std::string text;
    
    // Mouse event data
    int mouse_x = 0;
    int mouse_y = 0;
    int mouse_button = 0;
    int wheel_x = 0;
    int wheel_y = 0;
};

/**
 * Window - SDL2-based window management
 */
class Window {
public:
    Window(const std::string& title, int width, int height, bool vsync = true);
    ~Window();
    
    // Non-copyable
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    
    // Window properties
    int get_width() const { return width_; }
    int get_height() const { return height_; }
    const std::string& get_title() const { return title_; }
    void set_title(const std::string& title);
    bool is_open() const { return is_open_; }
    
    // Event handling
    bool poll_event(Event& event);
    void wait_event(Event& event);
    
    // Rendering
    void draw(std::shared_ptr<Surface> surface);
    void present(); // New parameterless present
    void present(const Surface& surface); // Existing one for compat
    void clear(const Color& color = Color(0, 0, 0, 255));
    
    // Frame timing
    float get_delta_time() const { return delta_time_; }
    float get_fps() const { return fps_; }
    void set_target_fps(int fps);
    void set_unfocused_fps(int fps);
    
    // Window state
    bool is_focused() const;
    bool is_minimized() const;
    void set_cursor_visible(bool visible);
    void set_cursor_position(int x, int y);
    
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
    bool vsync_;
    
    SDL_Window* window_;
    SDL_Renderer* renderer_;
    SDL_Texture* texture_;
    std::shared_ptr<Surface> pending_surface_;
    
    // Timing
    uint64_t last_frame_time_;
    float delta_time_;
    float fps_;
    int target_fps_;
    int unfocused_fps_;
    
    void update_timing();
    Event translate_event(const SDL_Event& sdl_event);
};

// SDL initialization/cleanup (called automatically)
void init_sdl();
void quit_sdl();

} // namespace nativeui
