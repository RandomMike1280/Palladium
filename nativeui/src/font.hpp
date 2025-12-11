#pragma once

#include <string>
#include <memory>
#include <map>
#include "surface.hpp"

namespace nativeui {

/**
 * Font - Wrapper around TTF_Font
 */
class Font {
public:
    static void init();
    static void quit();

    // Load a font from file
    Font(const std::string& path, int size);
    ~Font();

    // Prevent copying, allow moving
    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;
    Font(Font&&) = default;
    Font& operator=(Font&&) = default;

    // Render text
    std::shared_ptr<Surface> render(const std::string& text, const Color& color);
    
    // Render text wrapped to a specific width (pixels)
    std::shared_ptr<Surface> render_wrapped(const std::string& text, const Color& color, int wrap_width);
    
    // Metrics
    int get_height() const;
    void get_size(const std::string& text, int& w, int& h);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * FontCache - Manages loaded fonts to avoid duplicates
 */
class FontCache {
public:
    static std::shared_ptr<Font> get(const std::string& name, int size);
    static void clear();

    // Helper to find system fonts or bundled fonts
    static std::string resolve_path(const std::string& name);

private:
    static std::map<std::pair<std::string, int>, std::shared_ptr<Font>> cache_;
};

} // namespace nativeui
