#include "animation.hpp"

namespace nativeui {

// Easing helper constants
static constexpr float PI = 3.14159265358979323846f;
static constexpr float BACK_OVERSHOOT = 1.70158f;

float Easing::apply(EasingType type, float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    
    switch (type) {
        case EasingType::Linear:        return linear(t);
        case EasingType::EaseInQuad:    return ease_in_quad(t);
        case EasingType::EaseOutQuad:   return ease_out_quad(t);
        case EasingType::EaseInOutQuad: return ease_in_out_quad(t);
        case EasingType::EaseInCubic:   return ease_in_cubic(t);
        case EasingType::EaseOutCubic:  return ease_out_cubic(t);
        case EasingType::EaseInOutCubic:return ease_in_out_cubic(t);
        case EasingType::EaseInExpo:    return ease_in_expo(t);
        case EasingType::EaseOutExpo:   return ease_out_expo(t);
        case EasingType::EaseInOutExpo: return ease_in_out_expo(t);
        case EasingType::EaseInElastic: return ease_in_elastic(t);
        case EasingType::EaseOutElastic:return ease_out_elastic(t);
        case EasingType::EaseInOutElastic: return ease_in_out_elastic(t);
        case EasingType::EaseInBack:    return ease_in_back(t);
        case EasingType::EaseOutBack:   return ease_out_back(t);
        case EasingType::EaseInOutBack: return ease_in_out_back(t);
        case EasingType::EaseInBounce:  return ease_in_bounce(t);
        case EasingType::EaseOutBounce: return ease_out_bounce(t);
        case EasingType::EaseInOutBounce: return ease_in_out_bounce(t);
        default: return t;
    }
}

float Easing::ease_in_elastic(float t)
{
    if (t == 0.0f) return 0.0f;
    if (t == 1.0f) return 1.0f;
    return -std::pow(2.0f, 10.0f * t - 10.0f) * std::sin((t * 10.0f - 10.75f) * (2.0f * PI) / 3.0f);
}

float Easing::ease_out_elastic(float t)
{
    if (t == 0.0f) return 0.0f;
    if (t == 1.0f) return 1.0f;
    return std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * (2.0f * PI) / 3.0f) + 1.0f;
}

float Easing::ease_in_out_elastic(float t)
{
    if (t == 0.0f) return 0.0f;
    if (t == 1.0f) return 1.0f;
    if (t < 0.5f) {
        return -(std::pow(2.0f, 20.0f * t - 10.0f) * std::sin((20.0f * t - 11.125f) * (2.0f * PI) / 4.5f)) / 2.0f;
    }
    return (std::pow(2.0f, -20.0f * t + 10.0f) * std::sin((20.0f * t - 11.125f) * (2.0f * PI) / 4.5f)) / 2.0f + 1.0f;
}

float Easing::ease_in_back(float t)
{
    const float c3 = BACK_OVERSHOOT + 1.0f;
    return c3 * t * t * t - BACK_OVERSHOOT * t * t;
}

float Easing::ease_out_back(float t)
{
    const float c3 = BACK_OVERSHOOT + 1.0f;
    return 1.0f + c3 * std::pow(t - 1.0f, 3.0f) + BACK_OVERSHOOT * std::pow(t - 1.0f, 2.0f);
}

float Easing::ease_in_out_back(float t)
{
    const float c2 = BACK_OVERSHOOT * 1.525f;
    if (t < 0.5f) {
        return (std::pow(2.0f * t, 2.0f) * ((c2 + 1.0f) * 2.0f * t - c2)) / 2.0f;
    }
    return (std::pow(2.0f * t - 2.0f, 2.0f) * ((c2 + 1.0f) * (t * 2.0f - 2.0f) + c2) + 2.0f) / 2.0f;
}

float Easing::ease_out_bounce(float t)
{
    const float n1 = 7.5625f;
    const float d1 = 2.75f;
    
    if (t < 1.0f / d1) {
        return n1 * t * t;
    } else if (t < 2.0f / d1) {
        t -= 1.5f / d1;
        return n1 * t * t + 0.75f;
    } else if (t < 2.5f / d1) {
        t -= 2.25f / d1;
        return n1 * t * t + 0.9375f;
    } else {
        t -= 2.625f / d1;
        return n1 * t * t + 0.984375f;
    }
}

float Easing::ease_in_bounce(float t)
{
    return 1.0f - ease_out_bounce(1.0f - t);
}

float Easing::ease_in_out_bounce(float t)
{
    if (t < 0.5f) {
        return (1.0f - ease_out_bounce(1.0f - 2.0f * t)) / 2.0f;
    }
    return (1.0f + ease_out_bounce(2.0f * t - 1.0f)) / 2.0f;
}

// Animation class implementation

Animation::Animation(float start_value, float end_value, float duration, EasingType easing)
    : start_value_(start_value)
    , end_value_(end_value)
    , current_value_(start_value)
    , duration_(duration)
    , elapsed_(0.0f)
    , easing_(easing)
    , loop_(false)
    , reverse_(false)
    , yoyo_(false)
    , going_forward_(true)
{
}

float Animation::update(float dt)
{
    if (is_finished() && !loop_ && !yoyo_) {
        return current_value_;
    }
    
    elapsed_ += dt;
    
    if (elapsed_ >= duration_) {
        if (yoyo_) {
            going_forward_ = !going_forward_;
            elapsed_ = 0.0f;
        } else if (loop_) {
            elapsed_ = 0.0f;
        }
    }
    
    float t = std::min(elapsed_ / duration_, 1.0f);
    float eased_t = Easing::apply(easing_, t);
    
    if (reverse_ || (!going_forward_ && yoyo_)) {
        eased_t = 1.0f - eased_t;
    }
    
    current_value_ = start_value_ + (end_value_ - start_value_) * eased_t;
    return current_value_;
}

void Animation::reset()
{
    elapsed_ = 0.0f;
    going_forward_ = true;
    current_value_ = start_value_;
}

void Animation::restart()
{
    reset();
}

// SpringAnimation class implementation

SpringAnimation::SpringAnimation(float target, float stiffness, float damping, float mass)
    : position_(target)
    , velocity_(0.0f)
    , target_(target)
    , stiffness_(stiffness)
    , damping_(damping)
    , mass_(mass)
{
}

float SpringAnimation::update(float dt)
{
    // Spring physics using Hooke's Law + damping
    // F = -kx - cv (spring force + damping force)
    // a = F/m
    
    float displacement = position_ - target_;
    float spring_force = -stiffness_ * displacement;
    float damping_force = -damping_ * velocity_;
    float acceleration = (spring_force + damping_force) / mass_;
    
    // Semi-implicit Euler integration (more stable than explicit Euler)
    velocity_ += acceleration * dt;
    position_ += velocity_ * dt;
    
    return position_;
}

bool SpringAnimation::is_at_rest() const
{
    float displacement = std::abs(position_ - target_);
    float speed = std::abs(velocity_);
    return displacement < REST_THRESHOLD && speed < VELOCITY_THRESHOLD;
}

SpringAnimation SpringAnimation::gentle(float target)
{
    return SpringAnimation(target, 120.0f, 14.0f, 1.0f);
}

SpringAnimation SpringAnimation::wobbly(float target)
{
    return SpringAnimation(target, 180.0f, 12.0f, 1.0f);
}

SpringAnimation SpringAnimation::stiff(float target)
{
    return SpringAnimation(target, 210.0f, 20.0f, 1.0f);
}

SpringAnimation SpringAnimation::slow(float target)
{
    return SpringAnimation(target, 280.0f, 60.0f, 1.0f);
}

} // namespace nativeui
