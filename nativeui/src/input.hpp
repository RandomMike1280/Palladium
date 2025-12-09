#pragma once

#include <vector>
#include <algorithm>
#include <SDL2/SDL.h>

namespace nativeui {

/**
 * Key codes (Subset of SDL keycodes for convenience)
 */
enum class Key {
    Unknown = 0,
    
    // ASCII
    Space = 32,
    Quote = 39,
    Comma = 44,
    Minus = 45,
    Period = 46,
    Slash = 47,
    n0 = 48, n1 = 49, n2 = 50, n3 = 51, n4 = 52, 
    n5 = 53, n6 = 54, n7 = 55, n8 = 56, n9 = 57,
    Semicolon = 59,
    Equals = 61,
    LeftBracket = 91,
    Backslash = 92,
    RightBracket = 93,
    Backquote = 96,
    a = 97, b = 98, c = 99, d = 100, e = 101, f = 102, g = 103, h = 104, 
    i = 105, j = 106, k = 107, l = 108, m = 109, n = 110, o = 111, p = 112, 
    q = 113, r = 114, s = 115, t = 116, u = 117, v = 118, w = 119, x = 120, 
    y = 121, z = 122,
    
    // Function keys
    F1 = 1073741882, F2 = 1073741883, F3 = 1073741884, F4 = 1073741885,
    F5 = 1073741886, F6 = 1073741887, F7 = 1073741888, F8 = 1073741889,
    F9 = 1073741890, F10 = 1073741891, F11 = 1073741892, F12 = 1073741893,
    
    // Modifiers & Controls
    Escape = 27,
    Return = 13,
    Tab = 9,
    Backspace = 8,
    Insert = 1073741897,
    Home = 1073741898,
    PageUp = 1073741899,
    Delete = 127,
    End = 1073741901,
    PageDown = 1073741902,
    Right = 1073741903,
    Left = 1073741904,
    Down = 1073741905,
    Up = 1073741906,
    
    LCtrl = 1073742048,
    LShift = 1073742049,
    LAlt = 1073742050,
    RCtrl = 1073742052,
    RShift = 1073742053,
    RAlt = 1073742054
};

// Forward decl of Event (assumed existing in window.hpp or similar)
struct Event;

/**
 * Input - Manages key states and hotkeys
 */
class Input {
public:
    Input();
    
    // Update state based on event
    void process(const Event& event);
    
    // Check if a specific key is currently held down
    bool is_key_down(Key key) const;
    
    // Get all currently pressed keys in order of press
    std::vector<Key> get_pressed_keys() const;
    
    /**
     * Check if a combination of keys is pressed.
     * @param combo List of keys required
     * @param ordered If true, keys must have been pressed in the specific order
     * @return True if the combination matches active keys
     */
    bool check_hotkey(const std::vector<Key>& combo, bool ordered = false) const;

private:
    std::vector<Key> pressed_keys_;  // Maintained in order of press
    
    // Helper to find index of key in pressed list
    int get_key_index(Key key) const;
};

} // namespace nativeui
