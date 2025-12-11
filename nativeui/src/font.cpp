#include "font.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <stdexcept>
#include <filesystem>
#include <algorithm>

// Define common font locations for Windows (simplistic)
#ifdef _WIN32
const std::string SYSTEM_FONT_DIR = "C:\\Windows\\Fonts\\";
#else
const std::string SYSTEM_FONT_DIR = "/usr/share/fonts/"; // Fallback
#endif

namespace nativeui {

struct Font::Impl {
    TTF_Font* font = nullptr;
};

void Font::init() {
    if (TTF_Init() == -1) {
        throw std::runtime_error("TTF_Init: " + std::string(TTF_GetError()));
    }
}

void Font::quit() {
    TTF_Quit();
}

Font::Font(const std::string& path, int size) : impl_(std::make_unique<Impl>()) {
    impl_->font = TTF_OpenFont(path.c_str(), size);
    if (!impl_->font) {
        throw std::runtime_error("TTF_OpenFont: " + std::string(TTF_GetError()) + " (Path: " + path + ")");
    }
}

Font::~Font() {
    if (impl_->font) {
        TTF_CloseFont(impl_->font);
    }
}

std::shared_ptr<Surface> Font::render(const std::string& text, const Color& color) {
    if (!impl_->font || text.empty()) return nullptr;

    SDL_Color sdl_color = { color.r, color.g, color.b, color.a };
    
    // Use blended (high quality, alpha) rendering
    SDL_Surface* surface = TTF_RenderUTF8_Blended(impl_->font, text.c_str(), sdl_color);
    
    if (!surface) {
        // If empty text or error, return null or throw?
        // TTF_RenderText returns null on error.
        return nullptr;
    }

    // Convert SDL_Surface to our Surface
    // SDL_ttf blended mode returns ARGB8888 usually (32-bit)
    if (surface->format->BytesPerPixel != 4) {
         SDL_FreeSurface(surface);
         return nullptr; // Only support 32-bit for now simplicity
    }

    auto result = std::make_shared<Surface>(surface->w, surface->h);
    
    // Copy pixels
    // We assume SDL surface is locked or doesn't need locking for software surfaces usually
    // But safe to lock.
    SDL_LockSurface(surface);
    
    uint8_t* src_pixels = static_cast<uint8_t*>(surface->pixels);
    int pitch = surface->pitch;
    
    for (int y = 0; y < surface->h; ++y) {
        for (int x = 0; x < surface->w; ++x) {
            // Extract from SDL surface (BGRA or RGBA depending on endian)
            // TTF blended is typically ARGB (on big endian) or BGRA (little endian)
            // Actually it depends. Let's rely on GetRGBA
            uint32_t pixel = *reinterpret_cast<uint32_t*>(src_pixels + y * pitch + x * 4);
            uint8_t r, g, b, a;
            SDL_GetRGBA(pixel, surface->format, &r, &g, &b, &a);
            
            result->set_pixel(x, y, Color(r, g, b, a));
        }
    }
    
    SDL_UnlockSurface(surface);
    SDL_FreeSurface(surface);
    
    return result;
}

std::shared_ptr<Surface> Font::render_wrapped(const std::string& text, const Color& color, int wrap_width) {
    if (!impl_->font || text.empty()) return nullptr;

    SDL_Color sdl_color = { color.r, color.g, color.b, color.a };
    
    // Use wrapped rendering
    SDL_Surface* surface = TTF_RenderUTF8_Blended_Wrapped(impl_->font, text.c_str(), sdl_color, wrap_width);
    
    if (!surface) {
        return nullptr;
    }

    // Convert SDL_Surface to our Surface (duplicate logic for now to avoid refactor overhead)
    // SDL_ttf blended mode returns 32-bit usually
    if (surface->format->BytesPerPixel != 4) {
         SDL_FreeSurface(surface);
         return nullptr; 
    }

    auto result = std::make_shared<Surface>(surface->w, surface->h);
    
    SDL_LockSurface(surface);
    
    uint8_t* src_pixels = static_cast<uint8_t*>(surface->pixels);
    int pitch = surface->pitch;
    
    for (int y = 0; y < surface->h; ++y) {
        for (int x = 0; x < surface->w; ++x) {
            uint32_t pixel = *reinterpret_cast<uint32_t*>(src_pixels + y * pitch + x * 4);
            uint8_t r, g, b, a;
            SDL_GetRGBA(pixel, surface->format, &r, &g, &b, &a);
            result->set_pixel(x, y, Color(r, g, b, a));
        }
    }
    
    SDL_UnlockSurface(surface);
    SDL_FreeSurface(surface);
    
    return result;
}

int Font::get_height() const {
    if (!impl_->font) return 0;
    return TTF_FontHeight(impl_->font);
}

void Font::get_size(const std::string& text, int& w, int& h) {
    if (!impl_->font) { w=0; h=0; return; }
    TTF_SizeUTF8(impl_->font, text.c_str(), &w, &h);
}

// ============ FontCache ============

std::map<std::pair<std::string, int>, std::shared_ptr<Font>> FontCache::cache_;

std::string FontCache::resolve_path(const std::string& name) {
    if (std::filesystem::exists(name)) return name;
    
    // Try system font dir
    std::string sys_path = SYSTEM_FONT_DIR + name + ".ttf";
    if (std::filesystem::exists(sys_path)) return sys_path;
    
    sys_path = SYSTEM_FONT_DIR + name;
    if (std::filesystem::exists(sys_path)) return sys_path;
    
    // Fallback: simple mapping (very basic)
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
    
    std::map<std::string, std::string> font_map = {
        {"arial", "arial.ttf"},
        {"roboto", "arial.ttf"}, // Mapped to Arial
        {"roboto bold", "arialbd.ttf"}, // Map to Arial Bold
        {"segoe ui", "segoeui.ttf"},
        {"times new roman", "times.ttf"},
        {"verdana", "verdana.ttf"},
        {"consolas", "consolas.ttf"}
    };
    
    if (font_map.find(lower_name) != font_map.end()) {
        sys_path = SYSTEM_FONT_DIR + font_map[lower_name];
        if (std::filesystem::exists(sys_path)) return sys_path;
    }
    
    return name; // Return original if nothing found, let TTF fail
}

std::shared_ptr<Font> FontCache::get(const std::string& name, int size) {
    std::string path = resolve_path(name);
    auto key = std::make_pair(path, size);
    
    if (cache_.find(key) == cache_.end()) {
        try {
            cache_[key] = std::make_shared<Font>(path, size);
        } catch (...) {
            return nullptr;
        }
    }
    
    return cache_[key];
}

void FontCache::clear() {
    cache_.clear();
}

} // namespace nativeui
