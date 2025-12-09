#pragma once

#include "layer.hpp"
#include "animation.hpp"
#include "window.hpp"
#include "font.hpp"
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace nativeui {

enum class TextFieldShape {
    Rectangle,
    RoundedRect
};

enum class ExpandDirection {
    Up,
    Down
};

// Placeholder text styling
struct PlaceholderStyle {
    std::string text = "";
    std::string font = "Roboto";
    int font_size = 14;
    Color color = Color(128, 128, 128, 255);
};

// Typed text styling
struct TypedTextStyle {
    std::string font = "Roboto";
    int font_size = 14;
    Color color = Color(255, 255, 255, 255);
};

// Visual style for different states
struct TextFieldStyle {
    Color background_color = Color(50, 50, 60, 255);
    float opacity = 1.0f;
    float blur_radius = 0.0f;
    float scale = 1.0f;
    
    static TextFieldStyle lerp(const TextFieldStyle& a, const TextFieldStyle& b, float t);
};

class TextField : public Layer {
public:
    TextField(int width, int height, TextFieldShape shape = TextFieldShape::RoundedRect, int radius = 8);
    
    // Styling
    void set_placeholder(const PlaceholderStyle& style);
    void set_text_style(const TypedTextStyle& style);
    void set_style(const std::string& state, const TextFieldStyle& style);
    
    // Properties
    void set_max_chars(int limit) { max_chars_ = limit; }
    void set_max_words(int limit) { max_words_ = limit; }
    void set_multiline(bool enabled) { multiline_ = enabled; }
    void set_end_line(bool enabled) { end_line_ = enabled; }
    void set_expand_direction(ExpandDirection dir) { expand_dir_ = dir; }
    
    // Text access
    const std::string& get_text() const { return text_; }
    void set_text(const std::string& text);
    
    // Focus
    void focus();
    void blur();
    bool is_focused() const { return is_focused_; }
    
    // Animation
    void set_hover_animation(float duration = 0.1f);
    
    // Event handling
    void process_event(const Event& event);
    void update(float dt);
    
    // Callbacks
    std::function<void(const std::string&)> on_change;
    std::function<void(const std::string&)> on_submit;
    
    // Redraw
    void redraw();
    
private:
    TextFieldShape shape_;
    int radius_;
    int base_width_;
    int base_height_;
    
    // Limits
    int max_chars_ = 0;
    int max_words_ = 0;
    
    // Behavior
    bool multiline_ = false;
    bool end_line_ = true;
    ExpandDirection expand_dir_ = ExpandDirection::Down;
    
    // Styles
    PlaceholderStyle placeholder_;
    TypedTextStyle text_style_;
    std::map<std::string, TextFieldStyle> styles_;
    
    // State
    std::string text_;
    bool is_focused_ = false;
    bool is_hovered_ = false;
    int cursor_pos_ = 0;
    int sel_start_ = 0;
    int sel_end_ = 0;
    int scroll_offset_x_ = 0;
    int scroll_offset_y_ = 0;
    
    // Animation
    TextFieldStyle current_style_;
    TextFieldStyle target_style_;
    Animation transition_anim_;
    
    // Cursor blink
    float cursor_blink_timer_ = 0.0f;
    bool cursor_visible_ = true;
    
    // Internal
    void update_target_style();
    void handle_key_input(const Event& event);
    void insert_text(const std::string& text);
    void backspace_char();
    void backspace_word();
    void delete_char();
    void delete_selection();
    
    bool has_selection() const;
    void normalize_selection();
    void select_all();
    std::string get_selected_text() const;
    void copy_to_clipboard();
    void cut_to_clipboard();
    void paste_from_clipboard();
    
    static TextField* active_field_;
    
    bool check_limits(const std::string& new_text);
    int count_words(const std::string& str);
    
    void update_scroll();
    void update_dimensions();
    std::vector<std::string> wrap_text(const std::string& text, int max_width);
    
    void draw_background(Surface& s);
    void draw_selection(Surface& s);
    void draw_text_content(Surface& s);
    void draw_cursor(Surface& s);
};

} // namespace nativeui
