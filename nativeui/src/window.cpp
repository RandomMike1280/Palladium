#include "window.hpp"
#include "font.hpp"
#include <stdexcept>

namespace nativeui {

// SDL initialization count
static int sdl_init_count = 0;

void init_sdl()
{
    if (sdl_init_count == 0) {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
            throw std::runtime_error(std::string("SDL initialization failed: ") + SDL_GetError());
        }
        // Initialize Fonts
        try {
            Font::init();
        } catch (const std::exception& e) {
            SDL_Quit();
            throw;
        }
    }
    sdl_init_count++;
}

void quit_sdl()
{
    sdl_init_count--;
    if (sdl_init_count <= 0) {
        Font::quit();
        SDL_Quit();
        sdl_init_count = 0;
    }
}

Window::Window(const std::string& title, int width, int height, bool vsync)
    : title_(title)
    , width_(width)
    , height_(height)
    , is_open_(true)
    , is_fullscreen_(false)
    , vsync_(vsync)
    , window_(nullptr)
    , renderer_(nullptr)
    , texture_(nullptr)
    , last_frame_time_(0)
    , delta_time_(0.0f)
    , fps_(0.0f)

    , target_fps_(0)
    , unfocused_fps_(0)
{
    init_sdl();
    
    window_ = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_SHOWN
    );
    
    if (!window_) {
        throw std::runtime_error(std::string("Failed to create window: ") + SDL_GetError());
    }
    
    Uint32 renderer_flags = SDL_RENDERER_ACCELERATED;
    if (vsync) {
        renderer_flags |= SDL_RENDERER_PRESENTVSYNC;
    }
    
    renderer_ = SDL_CreateRenderer(window_, -1, renderer_flags);
    if (!renderer_) {
        SDL_DestroyWindow(window_);
        throw std::runtime_error(std::string("Failed to create renderer: ") + SDL_GetError());
    }
    
    texture_ = SDL_CreateTexture(
        renderer_,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING,
        width,
        height
    );
    
    if (!texture_) {
        SDL_DestroyRenderer(renderer_);
        SDL_DestroyWindow(window_);
        throw std::runtime_error(std::string("Failed to create texture: ") + SDL_GetError());
    }
    
    last_frame_time_ = SDL_GetPerformanceCounter();
}

Window::~Window()
{
    if (texture_) SDL_DestroyTexture(texture_);
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_) SDL_DestroyWindow(window_);
    quit_sdl();
}

void Window::set_title(const std::string& title)
{
    title_ = title;
    SDL_SetWindowTitle(window_, title.c_str());
}

bool Window::poll_event(Event& event)
{
    SDL_Event sdl_event;
    if (SDL_PollEvent(&sdl_event)) {
        event = translate_event(sdl_event);
        return true;
    }
    return false;
}

void Window::wait_event(Event& event)
{
    SDL_Event sdl_event;
    if (SDL_WaitEvent(&sdl_event)) {
        event = translate_event(sdl_event);
    }
}

Event Window::translate_event(const SDL_Event& sdl_event)
{
    Event event;
    
    switch (sdl_event.type) {
        case SDL_QUIT:
            event.type = EventType::Quit;
            is_open_ = false;
            break;
            
        case SDL_KEYDOWN:
            event.type = EventType::KeyDown;
            event.key = sdl_event.key.keysym.sym;
            event.ctrl = (sdl_event.key.keysym.mod & KMOD_CTRL) != 0;
            event.shift = (sdl_event.key.keysym.mod & KMOD_SHIFT) != 0;
            event.alt = (sdl_event.key.keysym.mod & KMOD_ALT) != 0;
            break;

        case SDL_TEXTINPUT:
            event.type = EventType::TextInput;
            event.text = sdl_event.text.text;
            break;
            
        case SDL_KEYUP:
            event.type = EventType::KeyUp;
            event.key = sdl_event.key.keysym.sym;
            event.ctrl = (sdl_event.key.keysym.mod & KMOD_CTRL) != 0;
            event.shift = (sdl_event.key.keysym.mod & KMOD_SHIFT) != 0;
            event.alt = (sdl_event.key.keysym.mod & KMOD_ALT) != 0;
            break;
            
        case SDL_MOUSEBUTTONDOWN:
            event.type = EventType::MouseButtonDown;
            event.mouse_x = sdl_event.button.x;
            event.mouse_y = sdl_event.button.y;
            event.mouse_button = sdl_event.button.button;
            break;
            
        case SDL_MOUSEBUTTONUP:
            event.type = EventType::MouseButtonUp;
            event.mouse_x = sdl_event.button.x;
            event.mouse_y = sdl_event.button.y;
            event.mouse_button = sdl_event.button.button;
            break;
            
        case SDL_MOUSEMOTION:
            event.type = EventType::MouseMotion;
            event.mouse_x = sdl_event.motion.x;
            event.mouse_y = sdl_event.motion.y;
            break;
            
        case SDL_MOUSEWHEEL:
            event.type = EventType::MouseWheel;
            event.wheel_x = sdl_event.wheel.x;
            event.wheel_y = sdl_event.wheel.y;
            break;
            
        default:
            event.type = EventType::None;
            break;
    }
    
    return event;
}

void Window::present(const Surface& surface)
{
    // Update texture with surface data
    void* pixels;
    int pitch;
    
    if (SDL_LockTexture(texture_, nullptr, &pixels, &pitch) == 0) {
        const uint8_t* src = surface.get_data();
        uint8_t* dst = static_cast<uint8_t*>(pixels);
        
        int min_width = std::min(width_, surface.get_width());
        int min_height = std::min(height_, surface.get_height());
        size_t src_pitch = surface.get_pitch();
        
        for (int y = 0; y < min_height; ++y) {
            std::memcpy(dst + y * pitch, src + y * src_pitch, min_width * 4);
        }
        
        SDL_UnlockTexture(texture_);
    }
    
    SDL_RenderClear(renderer_);
    SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
    SDL_RenderPresent(renderer_);
    
    update_timing();
}

void Window::draw(std::shared_ptr<Surface> surface) {
    pending_surface_ = surface;
}

void Window::present() {
    if (pending_surface_) {
        present(*pending_surface_);
        pending_surface_ = nullptr;
    } else {
        SDL_RenderPresent(renderer_);
        update_timing();
    }
}

void Window::clear(const Color& color)
{
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderClear(renderer_);
}



void Window::update_timing()
{
    uint64_t current_time = SDL_GetPerformanceCounter();
    uint64_t frequency = SDL_GetPerformanceFrequency();
    
    // Check window state for FPS throttling
    int effective_target_fps = target_fps_;
    
    if (is_minimized()) {
        // When minimized, heavily throttle to save resources
        // User asked to stop drawing, effectively this loop handles timing
        // We'll cap at 5 FPS to keep the loop moving but very slowly
        effective_target_fps = 5;
    } else if (unfocused_fps_ > 0 && !is_focused()) {
        effective_target_fps = unfocused_fps_;
    }
    
    delta_time_ = static_cast<float>(current_time - last_frame_time_) / frequency;
    if (delta_time_ > 0.0f) {
        fps_ = 1.0f / delta_time_;
    }
    
    // Frame rate limiting
    if (effective_target_fps > 0) {
        float target_frame_time = 1.0f / effective_target_fps;
        if (delta_time_ < target_frame_time) {
            SDL_Delay(static_cast<Uint32>((target_frame_time - delta_time_) * 1000.0f));
            current_time = SDL_GetPerformanceCounter();
            delta_time_ = static_cast<float>(current_time - last_frame_time_) / frequency;
        }
    }
    
    last_frame_time_ = current_time;
}

void Window::set_target_fps(int fps)
{
    target_fps_ = fps;
}

void Window::set_unfocused_fps(int fps)
{
    unfocused_fps_ = fps;
}

bool Window::is_focused() const
{
    Uint32 flags = SDL_GetWindowFlags(window_);
    return (flags & SDL_WINDOW_INPUT_FOCUS) != 0;
}

bool Window::is_minimized() const
{
    Uint32 flags = SDL_GetWindowFlags(window_);
    return (flags & SDL_WINDOW_MINIMIZED) != 0;
}

void Window::set_cursor_visible(bool visible)
{
    SDL_ShowCursor(visible ? SDL_ENABLE : SDL_DISABLE);
}

void Window::set_cursor_position(int x, int y)
{
    SDL_WarpMouseInWindow(window_, x, y);
}

void Window::set_fullscreen(bool fullscreen)
{
    if (fullscreen != is_fullscreen_) {
        SDL_SetWindowFullscreen(window_, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        is_fullscreen_ = fullscreen;
    }
}

} // namespace nativeui
