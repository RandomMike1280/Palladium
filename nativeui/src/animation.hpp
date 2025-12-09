#pragma once

#include <cmath>
#include <functional>
#include <algorithm>

namespace nativeui {

/**
 * Easing function types
 */
enum class EasingType {
    Linear,
    EaseInQuad,
    EaseOutQuad,
    EaseInOutQuad,
    EaseInCubic,
    EaseOutCubic,
    EaseInOutCubic,
    EaseInExpo,
    EaseOutExpo,
    EaseInOutExpo,
    EaseInElastic,
    EaseOutElastic,
    EaseInOutElastic,
    EaseInBack,
    EaseOutBack,
    EaseInOutBack,
    EaseInBounce,
    EaseOutBounce,
    EaseInOutBounce
};

/**
 * Easing functions implementation
 */
class Easing {
public:
    static float apply(EasingType type, float t);
    
    // Individual easing functions
    static float linear(float t) { return t; }
    
    static float ease_in_quad(float t) { return t * t; }
    static float ease_out_quad(float t) { return t * (2.0f - t); }
    static float ease_in_out_quad(float t) {
        return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
    }
    
    static float ease_in_cubic(float t) { return t * t * t; }
    static float ease_out_cubic(float t) { 
        float f = t - 1.0f; 
        return f * f * f + 1.0f; 
    }
    static float ease_in_out_cubic(float t) {
        return t < 0.5f ? 4.0f * t * t * t : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;
    }
    
    static float ease_in_expo(float t) { 
        return t == 0.0f ? 0.0f : std::pow(2.0f, 10.0f * (t - 1.0f)); 
    }
    static float ease_out_expo(float t) { 
        return t == 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t); 
    }
    static float ease_in_out_expo(float t) {
        if (t == 0.0f) return 0.0f;
        if (t == 1.0f) return 1.0f;
        if (t < 0.5f) return 0.5f * std::pow(2.0f, 20.0f * t - 10.0f);
        return 1.0f - 0.5f * std::pow(2.0f, -20.0f * t + 10.0f);
    }
    
    static float ease_in_elastic(float t);
    static float ease_out_elastic(float t);
    static float ease_in_out_elastic(float t);
    
    static float ease_in_back(float t);
    static float ease_out_back(float t);
    static float ease_in_out_back(float t);
    
    static float ease_in_bounce(float t);
    static float ease_out_bounce(float t);
    static float ease_in_out_bounce(float t);
};

/**
 * Animation - Tweens a value from start to end over a duration
 */
class Animation {
public:
    Animation(float start_value, float end_value, float duration, EasingType easing = EasingType::Linear);
    
    // Update and get current value
    float update(float dt);
    float get_value() const { return current_value_; }
    
    // State
    bool is_finished() const { return elapsed_ >= duration_; }
    bool is_running() const { return !is_finished(); }
    float get_progress() const { return std::min(elapsed_ / duration_, 1.0f); }
    
    // Control
    void reset();
    void restart();
    void set_start_value(float value) { start_value_ = value; }
    void set_end_value(float value) { end_value_ = value; }
    void set_duration(float duration) { duration_ = duration; }
    void set_easing(EasingType easing) { easing_ = easing; }
    
    // Looping
    void set_loop(bool loop) { loop_ = loop; }
    void set_reverse(bool reverse) { reverse_ = reverse; }
    void set_yoyo(bool yoyo) { yoyo_ = yoyo; }  // Go back and forth

private:
    float start_value_;
    float end_value_;
    float current_value_;
    float duration_;
    float elapsed_;
    EasingType easing_;
    
    bool loop_;
    bool reverse_;
    bool yoyo_;
    bool going_forward_;
};

/**
 * SpringAnimation - Physics-based spring animation
 */
class SpringAnimation {
public:
    SpringAnimation(float target, float stiffness = 100.0f, float damping = 10.0f, float mass = 1.0f);
    
    // Update and get current value
    float update(float dt);
    float get_value() const { return position_; }
    float get_velocity() const { return velocity_; }
    
    // State
    bool is_at_rest() const;
    bool is_finished() const { return is_at_rest(); }
    
    // Control
    void set_target(float target) { target_ = target; }
    void set_value(float value) { position_ = value; velocity_ = 0.0f; }
    void set_stiffness(float stiffness) { stiffness_ = stiffness; }
    void set_damping(float damping) { damping_ = damping; }
    void set_mass(float mass) { mass_ = mass; }
    
    // Presets
    static SpringAnimation gentle(float target);
    static SpringAnimation wobbly(float target);
    static SpringAnimation stiff(float target);
    static SpringAnimation slow(float target);

private:
    float position_;
    float velocity_;
    float target_;
    float stiffness_;
    float damping_;
    float mass_;
    
    static constexpr float REST_THRESHOLD = 0.001f;
    static constexpr float VELOCITY_THRESHOLD = 0.001f;
};

} // namespace nativeui
