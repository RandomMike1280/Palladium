#pragma once

#include <memory>

namespace nativeui {

/**
 * Material types for objects
 */
enum class MaterialType {
    Solid = 0,        // Opaque, no background interaction
    FrostedGlass = 1  // Blurs background behind the object
};

/**
 * Material - Defines how an object interacts with its background
 */
class Material {
public:
    // Create solid material (default)
    static std::shared_ptr<Material> solid();
    
    // Create frosted glass material
    static std::shared_ptr<Material> frosted_glass(float blur_radius = 10.0f);
    
    // Getters
    MaterialType get_type() const { return type_; }
    float get_blur_radius() const { return blur_radius_; }
    
    // Setters
    void set_blur_radius(float radius) { blur_radius_ = std::max(0.0f, radius); }
    
    // Check type
    bool is_solid() const { return type_ == MaterialType::Solid; }
    bool is_frosted_glass() const { return type_ == MaterialType::FrostedGlass; }

private:
    Material(MaterialType type, float blur_radius = 0.0f)
        : type_(type), blur_radius_(blur_radius) {}
    
    MaterialType type_;
    float blur_radius_;
};

} // namespace nativeui
