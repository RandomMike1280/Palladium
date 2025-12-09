#include "material.hpp"

namespace nativeui {

std::shared_ptr<Material> Material::solid()
{
    return std::shared_ptr<Material>(new Material(MaterialType::Solid));
}

std::shared_ptr<Material> Material::frosted_glass(float blur_radius)
{
    return std::shared_ptr<Material>(new Material(MaterialType::FrostedGlass, blur_radius));
}

} // namespace nativeui
