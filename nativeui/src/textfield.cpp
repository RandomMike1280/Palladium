#include "textfield.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <SDL2/SDL.h>

namespace nativeui {

TextField* TextField::active_field_ = nullptr;

TextFieldStyle TextFieldStyle::lerp(const TextFieldStyle& a, const TextFieldStyle& b, float t) {
    TextFieldStyle result;
    result.background_color = Color(
        static_cast<uint8_t>(a.background_color.r + (b.background_color.r - a.background_color.r) * t),
        static_cast<uint8_t>(a.background_color.g + (b.background_color.g - a.background_color.g) * t),
        static_cast<uint8_t>(a.background_color.b + (b.background_color.b - a.background_color.b) * t),
        static_cast<uint8_t>(a.background_color.a + (b.background_color.a - a.background_color.a) * t)
    );
    result.opacity = a.opacity + (b.opacity - a.opacity) * t;
    result.blur_radius = a.blur_radius + (b.blur_radius - a.blur_radius) * t;
    result.scale = a.scale + (b.scale - a.scale) * t;
    return result;
}

TextField::TextField(int width, int height, TextFieldShape shape, int radius)
    : Layer(width, height)
    , shape_(shape)
    , radius_(radius)
    , base_width_(width)
    , base_height_(height)
    , transition_anim_(0.0f, 1.0f, 0.1f)
{
    // Default styles
    styles_["normal"] = TextFieldStyle();
    styles_["hover"] = TextFieldStyle();
    styles_["hover"].background_color = Color(60, 60, 70, 255);
    styles_["focused"] = TextFieldStyle();
    styles_["focused"].background_color = Color(70, 70, 80, 255);
    
    current_style_ = styles_["normal"];
    target_style_ = styles_["normal"];
    
    redraw();
}

void TextField::set_placeholder(const PlaceholderStyle& style) {
    placeholder_ = style;
    redraw();
}

void TextField::set_text_style(const TypedTextStyle& style) {
    text_style_ = style;
    redraw();
}

void TextField::set_style(const std::string& state, const TextFieldStyle& style) {
    styles_[state] = style;
    if (state == "normal" && !is_focused_ && !is_hovered_) {
        current_style_ = style;
        update_target_style();
        redraw();
    }
}

void TextField::set_text(const std::string& text) {
    if (check_limits(text)) {
        text_ = text;
        cursor_pos_ = static_cast<int>(text_.length());
        sel_start_ = cursor_pos_;
        sel_end_ = cursor_pos_;
        update_dimensions();
        update_scroll();
        redraw();
    }
}

void TextField::focus() {
    if (is_focused_) return;

    if (active_field_ && active_field_ != this) {
        active_field_->blur(); // Deactivates previous, calls StopTextInput
    }
    
    active_field_ = this;
    is_focused_ = true;
    cursor_visible_ = true;
    cursor_blink_timer_ = 0.0f;
    update_target_style();
    
    SDL_StartTextInput();
    redraw();
}

void TextField::blur() {
    if (!is_focused_) return;
    
    is_focused_ = false;
    
    if (active_field_ == this) {
        active_field_ = nullptr;
        SDL_StopTextInput();
    }
    
    update_target_style();
    redraw();
}

void TextField::set_hover_animation(float duration) {
    transition_anim_.set_duration(duration);
}

void TextField::process_event(const Event& event) {
    if (event.type == EventType::MouseMotion) {
        bool hit = hit_test(event.mouse_x, event.mouse_y);
        if (hit != is_hovered_) {
            is_hovered_ = hit;
            update_target_style();
        }
    } else if (event.type == EventType::MouseButtonDown) {
        bool hit = hit_test(event.mouse_x, event.mouse_y);
        if (hit) {
            focus();
            // TODO: Calculate cursor position from click (requires click-point to text-index mapping)
            // For now, move cursor to end if clicked
            cursor_pos_ = static_cast<int>(text_.length());
            // Double click check: SDL mouse clicks have a 'clicks' count but our wrapper currently simplifies it
            // Reset selection
            sel_start_ = cursor_pos_;
            sel_end_ = cursor_pos_;
            redraw();
        } else {
            blur();
        }
    } else if (event.type == EventType::TextInput && is_focused_) {
        // Handle Unicode text insertion
        if (has_selection()) {
            delete_selection();
        }
        std::string input = event.text;
        
        // Filter newlines if not multiline (SDL might send them?) usually Enter is KeyDown
        if (!multiline_) {
            input.erase(std::remove(input.begin(), input.end(), '\n'), input.end());
            input.erase(std::remove(input.begin(), input.end(), '\r'), input.end());
        }
        
        insert_text(input);
        
    } else if (event.type == EventType::KeyDown && is_focused_) {
        handle_key_input(event);
    }
}

bool TextField::has_selection() const {
    return sel_start_ != sel_end_;
}

void TextField::normalize_selection() {
    // Determine min/max but keep sel_start/sel_end as anchor/focus
    // For operations, calculate range
}

void TextField::handle_key_input(const Event& event) {
    int key = event.key;
    bool ctrl = event.ctrl;
    bool shift = event.shift;
    bool changed = false;
    
    // Shortcuts
    if (ctrl) {
        if (key == SDLK_a) {
            select_all();
            redraw();
            return;
        } else if (key == SDLK_c) {
            copy_to_clipboard();
            return;
        } else if (key == SDLK_v) {
            paste_from_clipboard();
            return;
        } else if (key == SDLK_x) {
            cut_to_clipboard();
            return;
        }
    }

    // Navigation and Editing
    if (key == 8) { // Backspace
        if (has_selection()) {
            delete_selection();
            changed = true;
        } else if (ctrl) {
            backspace_word();
            changed = true;
        } else {
            backspace_char();
            changed = true;
        }
    } else if (key == 127 || key == 0x7F) { // Delete
        if (has_selection()) {
            delete_selection();
            changed = true;
        } else {
            delete_char();
            changed = true;
        }
    } else if (key == 13) { // Enter/Return
        if (multiline_) { // Shift+Enter logic removed, just Enter for multiline
             if (has_selection()) delete_selection();
             insert_text("\n");
             changed = true;
        } else if (on_submit) {
            on_submit(text_);
        }
    } 
    // Navigation
    else if (key == 1073741903) { // Right arrow
        if (ctrl) {
            // Move by word
            // Simplified: move until next space
            int len = static_cast<int>(text_.length());
            int next = cursor_pos_ + 1;
            while(next < len && text_[next] != ' ') next++;
            cursor_pos_ = std::min(len, next);
        } else {
            if (cursor_pos_ < static_cast<int>(text_.length())) cursor_pos_++;
        }
        
        if (shift) sel_end_ = cursor_pos_;
        else { sel_start_ = cursor_pos_; sel_end_ = cursor_pos_; }
        
        cursor_visible_ = true;
        cursor_blink_timer_ = 0.0f;
        changed = true;
        
    } else if (key == 1073741904) { // Left arrow
        if (ctrl) {
            // Move by word left
            int prev = cursor_pos_ - 1;
            while(prev > 0 && text_[prev-1] != ' ') prev--;
            cursor_pos_ = std::max(0, prev);
        } else {
            if (cursor_pos_ > 0) cursor_pos_--;
        }
        
        if (shift) sel_end_ = cursor_pos_;
        else { sel_start_ = cursor_pos_; sel_end_ = cursor_pos_; }
        
        cursor_visible_ = true;
        cursor_blink_timer_ = 0.0f;
        changed = true;
        
    } else if (key == 1073741898) { // Home
        cursor_pos_ = 0;
        if (shift) sel_end_ = cursor_pos_;
        else { sel_start_ = cursor_pos_; sel_end_ = cursor_pos_; }
        changed = true;
        
    } else if (key == 1073741901) { // End
        cursor_pos_ = static_cast<int>(text_.length());
        if (shift) sel_end_ = cursor_pos_;
        else { sel_start_ = cursor_pos_; sel_end_ = cursor_pos_; }
        changed = true;
    }
    
    // NOTE: Regular character input is handled by TextInput event now, 
    // EXCEPT that we still want to catch control keys that might not trigger TextInput
    // or if TextInput is not supported.
    // However, mixing the two can duplicate input.
    // SDL guidelines say: Process KeyDown for commands, Process TextInput for text.
    // So we DON'T process ASCII chars here anymore.
    
    if (changed) {
        update_dimensions();
        update_scroll();
        redraw();
        if (on_change) on_change(text_);
    }
}

void TextField::insert_text(const std::string& str) {
    if (str.empty()) return;
    
    std::string new_text = text_;
    // Insertion point is min of selection if any, else cursor
    int start = std::min(sel_start_, sel_end_);
    // But if we just deleted selection, cursor_pos points to insertion point
    // We assume delete_selection was called if a range existed
    // Actually, simple insert at cursor_pos
    new_text.insert(cursor_pos_, str);
    
    if (check_limits(new_text)) {
        text_ = new_text;
        cursor_pos_ += static_cast<int>(str.length());
        sel_start_ = cursor_pos_;
        sel_end_ = cursor_pos_;
        cursor_visible_ = true;
        cursor_blink_timer_ = 0.0f;
        update_dimensions();
        update_scroll();
        redraw();
        if (on_change) on_change(text_);
    }
}

void TextField::delete_selection() {
    if (!has_selection()) return;
    
    int start = std::min(sel_start_, sel_end_);
    int end = std::max(sel_start_, sel_end_);
    int count = end - start;
    
    text_.erase(start, count);
    cursor_pos_ = start;
    sel_start_ = start;
    sel_end_ = start;
    update_dimensions();
    update_scroll();
}

std::string TextField::get_selected_text() const {
    if (!has_selection()) return "";
    int start = std::min(sel_start_, sel_end_);
    int end = std::max(sel_start_, sel_end_);
    return text_.substr(start, end - start);
}

void TextField::select_all() {
    cursor_pos_ = static_cast<int>(text_.length());
    sel_start_ = 0;
    sel_end_ = cursor_pos_;
    redraw();
}

void TextField::copy_to_clipboard() {
    if (has_selection()) {
        SDL_SetClipboardText(get_selected_text().c_str());
    }
}

void TextField::cut_to_clipboard() {
    if (has_selection()) {
        copy_to_clipboard();
        delete_selection();
        redraw();
        if (on_change) on_change(text_);
    }
}

void TextField::paste_from_clipboard() {
    if (SDL_HasClipboardText()) {
        char* text = SDL_GetClipboardText();
        if (text) {
            std::string s(text);
            SDL_free(text);
            if (has_selection()) delete_selection();
            insert_text(s);
        }
    }
}

void TextField::backspace_word() {
    if (cursor_pos_ == 0 || text_.empty()) return;

    int new_pos = cursor_pos_;
    bool start_is_space = (new_pos > 0 && text_[new_pos - 1] == ' ');
    
    if (start_is_space) {
        while (new_pos > 0 && text_[new_pos - 1] == ' ') new_pos--;
    } else {
        while (new_pos > 0 && text_[new_pos - 1] != ' ') new_pos--;
    }
    
    int count = cursor_pos_ - new_pos;
    text_.erase(new_pos, count);
    cursor_pos_ = new_pos;
    sel_start_ = cursor_pos_;
    sel_end_ = cursor_pos_;
    cursor_visible_ = true;
    cursor_blink_timer_ = 0.0f;
    update_dimensions();
    update_scroll();
}

void TextField::backspace_char() {
    if (cursor_pos_ > 0 && !text_.empty()) {
        // Need to handle UTF-8 backspacing? 
        // Simple logic assumes 1 byte. For UTF-8, we need to find previous char boundary.
        // Quick crude check: while byte is continuation byte (0x80-0xBF), go back
        int delete_len = 1;
        while (cursor_pos_ - delete_len > 0 && 
               (text_[cursor_pos_ - delete_len] & 0xC0) == 0x80) {
            delete_len++;
        }
        
        text_.erase(cursor_pos_ - delete_len, delete_len);
        cursor_pos_ -= delete_len;
        sel_start_ = cursor_pos_;
        sel_end_ = cursor_pos_;
        cursor_visible_ = true;
        cursor_blink_timer_ = 0.0f;
    }
}

void TextField::delete_char() {
    if (cursor_pos_ < static_cast<int>(text_.length())) {
        // Move forward to find next char boundary
        int delete_len = 1;
        while (cursor_pos_ + delete_len < static_cast<int>(text_.length()) && 
               (text_[cursor_pos_ + delete_len] & 0xC0) == 0x80) {
            delete_len++;
        }
        text_.erase(cursor_pos_, delete_len);
        cursor_visible_ = true;
        cursor_blink_timer_ = 0.0f;
    }
}

bool TextField::check_limits(const std::string& new_text) {
    // Note: max_chars usually refers to unicode chars, but bytes is easier. 
    // We'll stick to bytes for simplicity or count unicode codepoints if needed.
    // For now: bytes.
    if (max_chars_ > 0 && static_cast<int>(new_text.length()) > max_chars_) {
        return false;
    }
    if (max_words_ > 0 && count_words(new_text) > max_words_) {
        return false;
    }
    return true;
}

int TextField::count_words(const std::string& str) {
    std::istringstream iss(str);
    std::string word;
    int count = 0;
    while (iss >> word) count++;
    return count;
}

void TextField::update_scroll() {
    auto font = FontCache::get(text_style_.font, text_style_.font_size);
    if (!font) return;
    
    // Calculate text width up to cursor
    std::string text_to_cursor = text_.substr(0, cursor_pos_);
    int cursor_x, h;
    font->get_size(text_to_cursor, cursor_x, h);
    
    int visible_width = base_width_ - 16;
    
    if (!end_line_) {
        if (cursor_x - scroll_offset_x_ > visible_width) {
            scroll_offset_x_ = cursor_x - visible_width + 20;
        } else if (cursor_x - scroll_offset_x_ < 0) {
            scroll_offset_x_ = cursor_x - 20;
            if (scroll_offset_x_ < 0) scroll_offset_x_ = 0;
        }
    }
}

void TextField::update_dimensions() {
    if (!end_line_) return;
    
    auto font = FontCache::get(text_style_.font, text_style_.font_size);
    if (!font) return;
    
    int line_height = font->get_height();
    int padding = 16;
    int usable_width = base_width_ - padding;
    
    // Calculate wrapped lines
    auto lines = wrap_text(text_, usable_width);
    int num_lines = std::max(1, static_cast<int>(lines.size()));
    
    int new_height = num_lines * line_height + padding;
    new_height = std::max(new_height, base_height_);
    
    // TODO: Resize logic if Layer supports it
}

std::vector<std::string> TextField::wrap_text(const std::string& text, int max_width) {
    std::vector<std::string> lines;
    auto font = FontCache::get(text_style_.font, text_style_.font_size);
    if (!font) {
        lines.push_back(text);
        return lines;
    }
    
    std::string current_line;
    std::string current_word;
    
    for (size_t i = 0; i <= text.length(); ++i) {
        char c = (i < text.length()) ? text[i] : ' ';
        
        if (c == '\n') {
            current_line += current_word;
            lines.push_back(current_line);
            current_line.clear();
            current_word.clear();
        } else if (c == ' ') {
            int w, h;
            font->get_size(current_line + current_word + " ", w, h);
            if (w > max_width && !current_line.empty()) {
                lines.push_back(current_line);
                current_line = current_word + " ";
            } else {
                current_line += current_word + " ";
            }
            current_word.clear();
        } else {
            current_word += c;
        }
    }
    
    if (!current_line.empty() || !current_word.empty()) {
        lines.push_back(current_line + current_word);
    }
    
    if (lines.empty()) lines.push_back("");
    
    return lines;
}

void TextField::update_target_style() {
    if (is_focused_) {
        target_style_ = styles_.count("focused") ? styles_["focused"] : styles_["normal"];
    } else if (is_hovered_) {
        target_style_ = styles_.count("hover") ? styles_["hover"] : styles_["normal"];
    } else {
        target_style_ = styles_["normal"];
    }
    
    transition_anim_.reset();
    transition_anim_.restart();
}

void TextField::update(float dt) {
    // Style animation
    if (transition_anim_.is_running()) {
        transition_anim_.update(dt);
        float alpha = 1.0f - std::exp(-10.0f * dt);
        current_style_ = TextFieldStyle::lerp(current_style_, target_style_, alpha);
        redraw();
    }
    
    // Cursor blink
    if (is_focused_) {
        cursor_blink_timer_ += dt;
        if (cursor_blink_timer_ >= 0.5f) {
            cursor_blink_timer_ = 0.0f;
            cursor_visible_ = !cursor_visible_;
            redraw();
        }
    }
}

void TextField::redraw() {
    Surface& s = get_surface();
    s.clear();
    
    set_opacity(current_style_.opacity);
    set_scale(current_style_.scale);
    
    // For frosted glass, ensure high enough alpha for mask (handled by draw_background logic fix potentially
    // but here we just set the material. The fix was in draw_background's manual pixel drawing?
    // No, I previous fixed draw_background to be SDF based. 
    // And frosted glass logic in LayerStack to respect transparency.
    // So here just setting material is fine.
    
    if (current_style_.blur_radius > 0.0f) {
        set_material(Material::frosted_glass(current_style_.blur_radius));
    } else {
        set_material(Material::solid());
    }
    
    draw_background(s);
    draw_selection(s); // NEW: Draw selection highlight
    draw_text_content(s);
    draw_cursor(s);
}

void TextField::draw_background(Surface& s) {
    int w = s.get_width();
    int h = s.get_height();
    Color c = current_style_.background_color;
    
    // Using SDF implementation for clean AA and no overdraw
    // d = length(max(abs(p) - b, 0.0)) - r
    
    // Respect shape setting
    float rx = (shape_ == TextFieldShape::RoundedRect) ? static_cast<float>(radius_) : 0.0f;
    
    float half_w = w * 0.5f;
    float half_h = h * 0.5f;
    float cx = half_w; // relative to surface 0,0
    float cy = half_h;
    float box_w = half_w - rx;
    float box_h = half_h - rx;
    
    bool aa = true; // Use AA always for high quality UI
    
    for (int py = 0; py < h; ++py) {
        for (int px = 0; px < w; ++px) {
            // Point relative to center
            float px_rel = std::abs(px - cx + 0.5f);
            float py_rel = std::abs(py - cy + 0.5f);
            
            float qx = px_rel - box_w;
            float qy = py_rel - box_h;
            
            float d = std::min(std::max(qx, qy), 0.0f) + 
                      std::sqrt(std::max(qx, 0.0f)*std::max(qx, 0.0f) + std::max(qy, 0.0f)*std::max(qy, 0.0f)) - 
                      rx;
            
            if (d <= -0.5f) {
                s.set_pixel(px, py, c);
            } else if (aa && d < 0.5f) {
                float alpha_f = 0.5f - d;
                alpha_f = std::clamp(alpha_f, 0.0f, 1.0f);
                Color dest = c;
                dest.a = static_cast<uint8_t>(dest.a * alpha_f);
                s.set_pixel(px, py, dest);
            }
        }
    }
}

void TextField::draw_selection(Surface& s) {
    if (!has_selection()) return;
    
    auto font = FontCache::get(text_style_.font, text_style_.font_size);
    if (!font) return;
    
    int start = std::min(sel_start_, sel_end_);
    int end = std::max(sel_start_, sel_end_);
    
    std::string before = text_.substr(0, start);
    std::string selected = text_.substr(start, end - start);
    
    int x1, h1;
    font->get_size(before, x1, h1);
    
    int sel_w, h2;
    font->get_size(selected, sel_w, h2);
    
    int padding = 8;
    int h = s.get_height();
    int line_height = font->get_height();
    int y = (h - line_height) / 2;
    
    int draw_x = padding + x1 - scroll_offset_x_;
    
    // Highlight color (blue-ish)
    Color highlight(50, 100, 200, 128);
    
    s.fill_rect(draw_x, y, sel_w, line_height, highlight);
}

void TextField::draw_text_content(Surface& s) {
    int padding = 8;
    int w = s.get_width();
    int h = s.get_height();
    
    bool show_placeholder = text_.empty() && !placeholder_.text.empty();
    
    std::shared_ptr<Font> font;
    std::string render_text;
    Color text_color;
    
    if (show_placeholder) {
        font = FontCache::get(placeholder_.font, placeholder_.font_size);
        render_text = placeholder_.text;
        text_color = placeholder_.color;
    } else {
        font = FontCache::get(text_style_.font, text_style_.font_size);
        render_text = text_;
        text_color = text_style_.color;
    }
    
    if (!font || render_text.empty()) return;
    
    auto text_surf = font->render(render_text, text_color);
    if (!text_surf) return;
    
    int txt_w = text_surf->get_width();
    int txt_h = text_surf->get_height();
    
    // Calculate position (vertically centered, left-aligned with scroll)
    int x = padding - scroll_offset_x_;
    int y = (h - txt_h) / 2;
    
    // Clip and blit
    int clip_start = std::max(0, -x);
    int clip_width = std::min(txt_w - clip_start, w - padding * 2);
    
    if (clip_width > 0) {
        // Blit
         for (int ty = 0; ty < txt_h; ++ty) {
            for (int tx = clip_start; tx < clip_start + clip_width; ++tx) {
                int dest_x = x + tx;
                int dest_y = y + ty;
                
                if (dest_x >= padding && dest_x < w - padding && dest_y >= 0 && dest_y < h) {
                    Color c = text_surf->get_pixel(tx, ty);
                    if (c.a > 0) {
                        s.blend_pixel(dest_x, dest_y, c);
                    }
                }
            }
        }
    }
}

void TextField::draw_cursor(Surface& s) {
    if (!is_focused_ || !cursor_visible_) return;
    
    auto font = FontCache::get(text_style_.font, text_style_.font_size);
    if (!font) return;
    
    int padding = 8;
    int h = s.get_height();
    
    // Calculate cursor X position
    std::string text_to_cursor = text_.substr(0, cursor_pos_);
    int cursor_x, txt_h;
    font->get_size(text_to_cursor, cursor_x, txt_h);
    
    int x = padding + cursor_x - scroll_offset_x_;
    int line_height = font->get_height();
    int y = (h - line_height) / 2;
    
    // Draw cursor line
    Color cursor_color = text_style_.color;
    if (x >= padding && x < s.get_width() - padding) {
        for (int dy = 0; dy < line_height; ++dy) {
            s.set_pixel(x, y + dy, cursor_color);
            if (x + 1 < s.get_width() - padding) {
                s.set_pixel(x + 1, y + dy, cursor_color);
            }
        }
    }
}

} // namespace nativeui
