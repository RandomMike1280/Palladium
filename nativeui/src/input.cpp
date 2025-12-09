#include "input.hpp"
#include "window.hpp"  // For Event definition
#include <algorithm>
#include <iostream>

namespace nativeui {

Input::Input() {
    pressed_keys_.reserve(16);
}

void Input::process(const Event& event) {
    if (event.type == EventType::KeyDown) {
        Key key = static_cast<Key>(event.key);
        // Only add if not already pressed (prevent repeats from filling buffer if we want strict order)
        // Alternatively, we could allow repeats, but hotkey detection usually cares about unique holds.
        if (!is_key_down(key)) {
            pressed_keys_.push_back(key);
        }
    } else if (event.type == EventType::KeyUp) {
        Key key = static_cast<Key>(event.key);
        // Remove from pressed list
        auto it = std::remove(pressed_keys_.begin(), pressed_keys_.end(), key);
        pressed_keys_.erase(it, pressed_keys_.end());
    }
}

bool Input::is_key_down(Key key) const {
    return std::find(pressed_keys_.begin(), pressed_keys_.end(), key) != pressed_keys_.end();
}

std::vector<Key> Input::get_pressed_keys() const {
    return pressed_keys_;
}

int Input::get_key_index(Key key) const {
    auto it = std::find(pressed_keys_.begin(), pressed_keys_.end(), key);
    if (it != pressed_keys_.end()) {
        return static_cast<int>(std::distance(pressed_keys_.begin(), it));
    }
    return -1;
}

bool Input::check_hotkey(const std::vector<Key>& combo, bool ordered) const {
    if (combo.empty()) return false;
    
    if (!ordered) {
        // Unordered: Just check if ALL keys in combo are currently down
        for (Key k : combo) {
            if (!is_key_down(k)) return false;
        }
        return true;
    } else {
        // Ordered: Check if keys in combo appear in pressed_keys_ in that relative order
        // They don't have to be adjacent, just in sequence.
        // E.g. pressed: [Ctrl, Shift, A], combo: [Ctrl, A] -> True
        
        int current_search_idx = 0;
        int found_count = 0;
        
        for (Key k : combo) {
            bool found = false;
            // Search for k in pressed_keys_ starting from current_search_idx
            for (int i = current_search_idx; i < static_cast<int>(pressed_keys_.size()); ++i) {
                if (pressed_keys_[i] == k) {
                    current_search_idx = i + 1; // Next search starts after this
                    found = true;
                    found_count++;
                    break;
                }
            }
            if (!found) return false;
        }
        return found_count == combo.size();
    }
}

} // namespace nativeui
