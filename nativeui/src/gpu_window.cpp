#ifdef _WIN32

#include "gpu_window.hpp"

namespace palladium {

GPUWindow::GPUWindow(const std::string& title, int width, int height, bool vsync)
    : title_(title)
    , width_(width)
    , height_(height)
    , is_open_(true)
    , is_fullscreen_(false)
    , sdl_window_(nullptr)
    , hwnd_(nullptr)
    , last_frame_time_(0)
    , delta_time_(0.0f)
    , fps_(0.0f)
    , target_fps_(0)
    , unfocused_fps_(0)
{
    // Initialize SDL if needed
    nativeui::init_sdl();
    
    // Create SDL window
    sdl_window_ = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_SHOWN
    );
    
    if (!sdl_window_) {
        throw std::runtime_error(std::string("Failed to create window: ") + SDL_GetError());
    }
    
    // Get native window handle
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (!SDL_GetWindowWMInfo(sdl_window_, &wmInfo)) {
        SDL_DestroyWindow(sdl_window_);
        throw std::runtime_error("Failed to get window handle");
    }
    hwnd_ = wmInfo.info.win.window;
    
    // Ensure D2D context is initialized
    auto& ctx = D2DContext::instance();
    if (!ctx.is_available()) {
        SDL_DestroyWindow(sdl_window_);
        throw std::runtime_error("GPU acceleration not available");
    }
    
    // Create D2D device context
    context_ = ctx.create_device_context();
    
    // Create swap chain
    create_swap_chain();
    
    // Create render target
    create_render_target();
    
    last_frame_time_ = SDL_GetPerformanceCounter();
}

GPUWindow::~GPUWindow() {
    if (is_drawing_) {
        context_->EndDraw();
    }
    
    target_bitmap_.Reset();
    context_.Reset();
    swap_chain_.Reset();
    
    if (sdl_window_) {
        SDL_DestroyWindow(sdl_window_);
    }
    nativeui::quit_sdl();
}

void GPUWindow::create_swap_chain() {
    auto& ctx = D2DContext::instance();
    
    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Width = width_;
    desc.Height = height_;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.Stereo = FALSE;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    desc.Flags = 0;
    
    HRESULT hr = ctx.get_dxgi_factory()->CreateSwapChainForHwnd(
        ctx.get_d3d_device(),
        hwnd_,
        &desc,
        nullptr,
        nullptr,
        &swap_chain_
    );
    throw_if_failed(hr, "Failed to create swap chain");
}

void GPUWindow::create_render_target() {
    // Get back buffer
    ComPtr<IDXGISurface> back_buffer;
    HRESULT hr = swap_chain_->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
    throw_if_failed(hr, "Failed to get swap chain back buffer");
    
    // Create D2D bitmap from DXGI surface
    D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)
    );
    
    hr = context_->CreateBitmapFromDxgiSurface(back_buffer.Get(), props, &target_bitmap_);
    throw_if_failed(hr, "Failed to create render target bitmap");
    
    context_->SetTarget(target_bitmap_.Get());
    context_->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
}

void GPUWindow::set_title(const std::string& title) {
    title_ = title;
    SDL_SetWindowTitle(sdl_window_, title.c_str());
}

bool GPUWindow::poll_event(nativeui::Event& event) {
    SDL_Event sdl_event;
    if (SDL_PollEvent(&sdl_event)) {
        event = translate_event(sdl_event);
        return true;
    }
    return false;
}

void GPUWindow::wait_event(nativeui::Event& event) {
    SDL_Event sdl_event;
    if (SDL_WaitEvent(&sdl_event)) {
        event = translate_event(sdl_event);
    }
}

nativeui::Event GPUWindow::translate_event(const SDL_Event& sdl_event) {
    nativeui::Event event;
    
    switch (sdl_event.type) {
        case SDL_QUIT:
            event.type = nativeui::EventType::Quit;
            is_open_ = false;
            break;
            
        case SDL_KEYDOWN:
            event.type = nativeui::EventType::KeyDown;
            event.key = sdl_event.key.keysym.sym;
            event.ctrl = (sdl_event.key.keysym.mod & KMOD_CTRL) != 0;
            event.shift = (sdl_event.key.keysym.mod & KMOD_SHIFT) != 0;
            event.alt = (sdl_event.key.keysym.mod & KMOD_ALT) != 0;
            break;

        case SDL_TEXTINPUT:
            event.type = nativeui::EventType::TextInput;
            event.text = sdl_event.text.text;
            break;
            
        case SDL_KEYUP:
            event.type = nativeui::EventType::KeyUp;
            event.key = sdl_event.key.keysym.sym;
            event.ctrl = (sdl_event.key.keysym.mod & KMOD_CTRL) != 0;
            event.shift = (sdl_event.key.keysym.mod & KMOD_SHIFT) != 0;
            event.alt = (sdl_event.key.keysym.mod & KMOD_ALT) != 0;
            break;
            
        case SDL_MOUSEBUTTONDOWN:
            event.type = nativeui::EventType::MouseButtonDown;
            event.mouse_x = sdl_event.button.x;
            event.mouse_y = sdl_event.button.y;
            event.mouse_button = sdl_event.button.button;
            break;
            
        case SDL_MOUSEBUTTONUP:
            event.type = nativeui::EventType::MouseButtonUp;
            event.mouse_x = sdl_event.button.x;
            event.mouse_y = sdl_event.button.y;
            event.mouse_button = sdl_event.button.button;
            break;
            
        case SDL_MOUSEMOTION:
            event.type = nativeui::EventType::MouseMotion;
            event.mouse_x = sdl_event.motion.x;
            event.mouse_y = sdl_event.motion.y;
            break;
            
        case SDL_MOUSEWHEEL:
            event.type = nativeui::EventType::MouseWheel;
            event.wheel_x = sdl_event.wheel.x;
            event.wheel_y = sdl_event.wheel.y;
            break;
            
        default:
            event.type = nativeui::EventType::None;
            break;
    }
    
    return event;
}

void GPUWindow::begin_draw() {
    if (!is_drawing_) {
        context_->BeginDraw();
        is_drawing_ = true;
    }
}

void GPUWindow::end_draw() {
    if (is_drawing_) {
        HRESULT hr = context_->EndDraw();
        is_drawing_ = false;
        
        if (hr == D2DERR_RECREATE_TARGET) {
            // Device lost, recreate resources
            target_bitmap_.Reset();
            create_render_target();
        }
    }
}

void GPUWindow::clear(const nativeui::Color& color) {
    bool was_drawing = is_drawing_;
    if (!was_drawing) begin_draw();
    
    context_->Clear(D2D1::ColorF(
        color.r / 255.0f,
        color.g / 255.0f,
        color.b / 255.0f,
        color.a / 255.0f
    ));
    
    if (!was_drawing) end_draw();
}

void GPUWindow::present() {
    end_draw();
    
    HRESULT hr = swap_chain_->Present(1, 0);  // VSync on
    
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
        // Handle device loss
        target_bitmap_.Reset();
        create_render_target();
    }
    
    update_timing();
}

void GPUWindow::draw(const GPUSurface& surface, int x, int y, float opacity) {
    draw_scaled(surface, x, y, surface.get_width(), surface.get_height(), opacity);
}

void GPUWindow::draw_scaled(const GPUSurface& surface, int x, int y, int w, int h, float opacity) {
    bool was_drawing = is_drawing_;
    if (!was_drawing) begin_draw();
    
    D2D1_RECT_F dest_rect = D2D1::RectF(
        static_cast<float>(x),
        static_cast<float>(y),
        static_cast<float>(x + w),
        static_cast<float>(y + h)
    );
    
    // Create a drawable copy of the source bitmap
    D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_NONE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );
    
    ComPtr<ID2D1Bitmap1> drawable_bitmap;
    HRESULT hr = context_->CreateBitmap(
        D2D1::SizeU(surface.get_width(), surface.get_height()),
        nullptr, 0, props, &drawable_bitmap
    );
    
    if (SUCCEEDED(hr)) {
        D2D1_POINT_2U dest_point = D2D1::Point2U(0, 0);
        hr = drawable_bitmap->CopyFromBitmap(&dest_point, surface.get_bitmap(), nullptr);
        
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

void GPUWindow::update_timing() {
    uint64_t current_time = SDL_GetPerformanceCounter();
    uint64_t frequency = SDL_GetPerformanceFrequency();
    
    // Check window state for FPS throttling
    int effective_target_fps = target_fps_;
    
    if (is_minimized()) {
        effective_target_fps = 5; // Throttle hard when minimized
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

void GPUWindow::set_target_fps(int fps) {
    target_fps_ = fps;
}

void GPUWindow::set_unfocused_fps(int fps) {
    unfocused_fps_ = fps;
}

bool GPUWindow::is_focused() const {
    Uint32 flags = SDL_GetWindowFlags(sdl_window_);
    return (flags & SDL_WINDOW_INPUT_FOCUS) != 0;
}

bool GPUWindow::is_minimized() const {
    Uint32 flags = SDL_GetWindowFlags(sdl_window_);
    return (flags & SDL_WINDOW_MINIMIZED) != 0;
}

void GPUWindow::set_cursor_visible(bool visible) {
    SDL_ShowCursor(visible ? SDL_ENABLE : SDL_DISABLE);
}

void GPUWindow::set_fullscreen(bool fullscreen) {
    if (fullscreen != is_fullscreen_) {
        SDL_SetWindowFullscreen(sdl_window_, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        is_fullscreen_ = fullscreen;
    }
}

} // namespace palladium

#endif // _WIN32
