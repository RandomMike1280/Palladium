/**
 * NativeUI - Python bindings using pybind11
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>

#include "surface.hpp"
#include "window.hpp"
#include "animation.hpp"
#include "effects.hpp"
#include "layer.hpp"
#include "layer.hpp"
#include "material.hpp"
#include "input.hpp"
#include "button.hpp"
#include "textfield.hpp"

namespace py = pybind11;
using namespace nativeui;

// Helper to parse ButtonStyle from dict
ButtonStyle parse_style(const py::dict& d) {
    ButtonStyle s;
    if (d.contains("color")) s.color = d["color"].cast<Color>();
    if (d.contains("opacity")) s.opacity = d["opacity"].cast<float>();
    if (d.contains("scale")) s.scale = d["scale"].cast<float>();
    if (d.contains("blur_radius")) s.blur_radius = d["blur_radius"].cast<float>();
    return s;
}

// Helper to parse ButtonTextStyle from dict
ButtonTextStyle parse_text_style(const py::dict& d) {
    ButtonTextStyle s;
    if (d.contains("text")) s.text = d["text"].cast<std::string>();
    if (d.contains("font")) s.font_name = d["font"].cast<std::string>();
    if (d.contains("font_size")) s.font_size = d["font_size"].cast<int>();
    if (d.contains("color")) {
        // Handle tuple or Color object
        try {
            s.color = d["color"].cast<Color>();
        } catch (...) {
            auto t = d["color"].cast<py::tuple>();
            if (t.size() >= 3) {
                 s.color.r = t[0].cast<uint8_t>();
                 s.color.g = t[1].cast<uint8_t>();
                 s.color.b = t[2].cast<uint8_t>();
                 if (t.size() > 3) s.color.a = t[3].cast<uint8_t>();
            }
        }
    }
    if (d.contains("position")) s.position = d["position"].cast<std::string>();
    
    if (d.contains("padding")) {
        auto p = d["padding"];
        if (py::isinstance<py::int_>(p)) {
            int val = p.cast<int>();
            s.padding_v = val;
            s.padding_h = val;
        } else if (py::isinstance<py::tuple>(p) || py::isinstance<py::list>(p)) {
            auto seq = p.cast<py::sequence>();
            if (seq.size() >= 2) {
                s.padding_v = seq[0].cast<int>();
                s.padding_h = seq[1].cast<int>();
            }
        }
    }
    
    if (d.contains("rotation")) s.rotation = d["rotation"].cast<float>();
    
    return s;
}

PYBIND11_MODULE(Palladium, m) {
    m.doc() = "Palladium - Low-level Python UI library with pixel manipulation, animations, and effects";
    
    // === Color ===
    py::class_<Color>(m, "Color")
        .def(py::init<>())
        .def(py::init<uint8_t, uint8_t, uint8_t, uint8_t>(),
             py::arg("r"), py::arg("g"), py::arg("b"), py::arg("a") = 255)
        .def_readwrite("r", &Color::r)
        .def_readwrite("g", &Color::g)
        .def_readwrite("b", &Color::b)
        .def_readwrite("a", &Color::a)
        .def("to_uint32", &Color::to_uint32)
        .def_static("from_uint32", &Color::from_uint32)
        .def("__repr__", [](const Color& c) {
            return "Color(" + std::to_string(c.r) + ", " + std::to_string(c.g) + 
                   ", " + std::to_string(c.b) + ", " + std::to_string(c.a) + ")";
        });
    
    // === Surface ===
    py::class_<Surface, std::shared_ptr<Surface>>(m, "Surface")
        .def(py::init<int, int>(), py::arg("width"), py::arg("height"))
        .def_property_readonly("width", &Surface::get_width)
        .def_property_readonly("height", &Surface::get_height)
        .def("set_pixel", py::overload_cast<int, int, uint8_t, uint8_t, uint8_t, uint8_t>(&Surface::set_pixel),
             py::arg("x"), py::arg("y"), py::arg("r"), py::arg("g"), py::arg("b"), py::arg("a") = 255)
        .def("set_pixel", py::overload_cast<int, int, const Color&>(&Surface::set_pixel))
        .def("get_pixel", &Surface::get_pixel)
        .def("fill", &Surface::fill)
        .def("fill_rect", &Surface::fill_rect)
        .def("clear", &Surface::clear)
        .def("draw_line", &Surface::draw_line)
        .def("draw_rect", &Surface::draw_rect)
        .def("draw_circle", &Surface::draw_circle)
        .def("fill_circle", &Surface::fill_circle)
        .def("blit", &Surface::blit)
        .def("blit_scaled", &Surface::blit_scaled)
        .def("blit_alpha", &Surface::blit_alpha, py::arg("source"), py::arg("dest_x"), py::arg("dest_y"), py::arg("alpha") = 1.0f)
        .def("copy", &Surface::copy)
        .def("subsurface", &Surface::subsurface)
        // Advanced Shapes
        .def("draw_round_rect", &Surface::draw_round_rect,
             py::arg("x"), py::arg("y"), py::arg("w"), py::arg("h"), py::arg("radius"), py::arg("color"))
        .def("fill_round_rect", &Surface::fill_round_rect,
             py::arg("x"), py::arg("y"), py::arg("w"), py::arg("h"), py::arg("radius"), py::arg("color"))
        .def("draw_pill", &Surface::draw_pill,
             py::arg("x"), py::arg("y"), py::arg("w"), py::arg("h"), py::arg("color"))
        .def("fill_pill", &Surface::fill_pill,
             py::arg("x"), py::arg("y"), py::arg("w"), py::arg("h"), py::arg("color"))
        .def("draw_squircle", &Surface::draw_squircle,
             py::arg("x"), py::arg("y"), py::arg("w"), py::arg("h"), py::arg("color"))
        .def("fill_squircle", &Surface::fill_squircle,
             py::arg("x"), py::arg("y"), py::arg("w"), py::arg("h"), py::arg("color"));
    
    // === Event Types ===
    py::enum_<EventType>(m, "EventType")
        .value("None", EventType::None)
        .value("Quit", EventType::Quit)
        .value("KeyDown", EventType::KeyDown)
        .value("KeyUp", EventType::KeyUp)
        .value("MouseButtonDown", EventType::MouseButtonDown)
        .value("MouseButtonUp", EventType::MouseButtonUp)
        .value("MouseMotion", EventType::MouseMotion)
        .value("MouseWheel", EventType::MouseWheel)
        .value("TextInput", EventType::TextInput);
    
    // === Event ===
    py::class_<Event>(m, "Event")
        .def(py::init<>())
        .def_readwrite("type", &Event::type)
        .def_readwrite("key", &Event::key)
        .def_readwrite("ctrl", &Event::ctrl)
        .def_readwrite("shift", &Event::shift)
        .def_readwrite("alt", &Event::alt)
        .def_readwrite("text", &Event::text)
        .def_readwrite("mouse_x", &Event::mouse_x)
        .def_readwrite("mouse_y", &Event::mouse_y)
        .def_readwrite("mouse_button", &Event::mouse_button)
        .def_readwrite("wheel_x", &Event::wheel_x)
        .def_readwrite("wheel_y", &Event::wheel_y);
    
    // === Window ===
    py::class_<Window>(m, "Window")
        .def(py::init<const std::string&, int, int, bool>(),
             py::arg("title"), py::arg("width"), py::arg("height"), py::arg("vsync") = true)
        .def_property_readonly("width", &Window::get_width)
        .def_property_readonly("height", &Window::get_height)
        .def_property_readonly("title", &Window::get_title)
        .def_property_readonly("is_open", &Window::is_open)
        .def_property_readonly("is_fullscreen", &Window::is_fullscreen)
        .def_property_readonly("delta_time", &Window::get_delta_time)
        .def_property_readonly("fps", &Window::get_fps)
        .def("set_title", &Window::set_title)
        .def("poll_event", [](Window& w) -> py::object {
            Event e;
            if (w.poll_event(e)) {
                return py::cast(e);
            }
            return py::none();
        })
        .def("wait_event", [](Window& w) {
            Event e;
            w.wait_event(e);
            return e;
        })
        .def("present", &Window::present)
        .def("clear", &Window::clear, py::arg("color") = Color(0, 0, 0, 255))
        .def("set_target_fps", &Window::set_target_fps)
        .def("set_cursor_visible", &Window::set_cursor_visible)
        .def("set_cursor_position", &Window::set_cursor_position)
        .def("set_fullscreen", &Window::set_fullscreen)
        .def("close", &Window::close);
    
    // === Easing Types ===
    py::enum_<EasingType>(m, "EasingType")
        .value("Linear", EasingType::Linear)
        .value("EaseInQuad", EasingType::EaseInQuad)
        .value("EaseOutQuad", EasingType::EaseOutQuad)
        .value("EaseInOutQuad", EasingType::EaseInOutQuad)
        .value("EaseInCubic", EasingType::EaseInCubic)
        .value("EaseOutCubic", EasingType::EaseOutCubic)
        .value("EaseInOutCubic", EasingType::EaseInOutCubic)
        .value("EaseInExpo", EasingType::EaseInExpo)
        .value("EaseOutExpo", EasingType::EaseOutExpo)
        .value("EaseInOutExpo", EasingType::EaseInOutExpo)
        .value("EaseInElastic", EasingType::EaseInElastic)
        .value("EaseOutElastic", EasingType::EaseOutElastic)
        .value("EaseInOutElastic", EasingType::EaseInOutElastic)
        .value("EaseInBack", EasingType::EaseInBack)
        .value("EaseOutBack", EasingType::EaseOutBack)
        .value("EaseInOutBack", EasingType::EaseInOutBack)
        .value("EaseInBounce", EasingType::EaseInBounce)
        .value("EaseOutBounce", EasingType::EaseOutBounce)
        .value("EaseInOutBounce", EasingType::EaseInOutBounce);
    
    // === Easing ===
    py::class_<Easing>(m, "Easing")
        .def_static("apply", &Easing::apply)
        .def_static("linear", &Easing::linear)
        .def_static("ease_in_quad", &Easing::ease_in_quad)
        .def_static("ease_out_quad", &Easing::ease_out_quad)
        .def_static("ease_in_out_quad", &Easing::ease_in_out_quad)
        .def_static("ease_in_cubic", &Easing::ease_in_cubic)
        .def_static("ease_out_cubic", &Easing::ease_out_cubic)
        .def_static("ease_in_out_cubic", &Easing::ease_in_out_cubic)
        .def_static("ease_in_expo", &Easing::ease_in_expo)
        .def_static("ease_out_expo", &Easing::ease_out_expo)
        .def_static("ease_in_out_expo", &Easing::ease_in_out_expo);
    
    // === Animation ===
    py::class_<Animation>(m, "Animation")
        .def(py::init<float, float, float, EasingType>(),
             py::arg("start_value"), py::arg("end_value"), py::arg("duration"),
             py::arg("easing") = EasingType::Linear)
        .def("update", &Animation::update)
        .def_property_readonly("value", &Animation::get_value)
        .def_property_readonly("finished", &Animation::is_finished)
        .def_property_readonly("running", &Animation::is_running)
        .def_property_readonly("progress", &Animation::get_progress)
        .def("reset", &Animation::reset)
        .def("restart", &Animation::restart)
        .def("set_start_value", &Animation::set_start_value)
        .def("set_end_value", &Animation::set_end_value)
        .def("set_duration", &Animation::set_duration)
        .def("set_easing", &Animation::set_easing)
        .def("set_loop", &Animation::set_loop)
        .def("set_reverse", &Animation::set_reverse)
        .def("set_yoyo", &Animation::set_yoyo);
    
    // === SpringAnimation ===
    py::class_<SpringAnimation>(m, "SpringAnimation")
        .def(py::init<float, float, float, float>(),
             py::arg("target"), py::arg("stiffness") = 100.0f,
             py::arg("damping") = 10.0f, py::arg("mass") = 1.0f)
        .def("update", &SpringAnimation::update)
        .def_property_readonly("value", &SpringAnimation::get_value)
        .def_property_readonly("velocity", &SpringAnimation::get_velocity)
        .def_property_readonly("at_rest", &SpringAnimation::is_at_rest)
        .def_property_readonly("finished", &SpringAnimation::is_finished)
        .def("set_target", &SpringAnimation::set_target)
        .def("set_value", &SpringAnimation::set_value)
        .def("set_stiffness", &SpringAnimation::set_stiffness)
        .def("set_damping", &SpringAnimation::set_damping)
        .def("set_mass", &SpringAnimation::set_mass)
        .def_static("gentle", &SpringAnimation::gentle)
        .def_static("wobbly", &SpringAnimation::wobbly)
        .def_static("stiff", &SpringAnimation::stiff)
        .def_static("slow", &SpringAnimation::slow);
    
    // === Effects ===
    py::class_<Effects>(m, "Effects")
        .def_static("box_blur", &Effects::box_blur)
        .def_static("gaussian_blur", &Effects::gaussian_blur)
        .def_static("blur_region", &Effects::blur_region)
        .def_static("frosted_glass", &Effects::frosted_glass,
                    py::arg("surface"), py::arg("blur_radius") = 10,
                    py::arg("noise_amount") = 0.05f, py::arg("saturation") = 0.8f)
        .def_static("frosted_glass_region", &Effects::frosted_glass_region,
                    py::arg("surface"), py::arg("x"), py::arg("y"),
                    py::arg("w"), py::arg("h"), py::arg("blur_radius") = 10)
        .def_static("displace", &Effects::displace,
                    py::arg("surface"), py::arg("displacement_map"), py::arg("strength") = 10.0f)
        .def_static("wave_distort", &Effects::wave_distort,
                    py::arg("surface"), py::arg("amplitude"), py::arg("frequency"), py::arg("phase") = 0.0f)
        .def_static("ripple", &Effects::ripple,
                    py::arg("surface"), py::arg("center_x"), py::arg("center_y"),
                    py::arg("amplitude"), py::arg("wavelength"), py::arg("phase") = 0.0f)
        .def_static("brightness", &Effects::brightness)
        .def_static("contrast", &Effects::contrast)
        .def_static("saturation", &Effects::saturation)
        .def_static("hue_shift", &Effects::hue_shift)
        .def_static("invert", &Effects::invert)
        .def_static("grayscale", &Effects::grayscale)
        .def_static("sepia", &Effects::sepia, py::arg("surface"), py::arg("strength") = 1.0f)
        .def_static("blend", &Effects::blend)
        .def_static("linear_gradient", &Effects::linear_gradient)
        .def_static("radial_gradient", &Effects::radial_gradient)
        .def_static("noise", &Effects::noise)
        .def_static("perlin_noise", &Effects::perlin_noise,
                    py::arg("surface"), py::arg("scale"), py::arg("octaves") = 4)
        .def_static("drop_shadow", &Effects::drop_shadow,
                    py::arg("source"), py::arg("offset_x"), py::arg("offset_y"),
                    py::arg("blur_radius"), py::arg("shadow_color"));
    
    // === BlurredSurface ===
    py::class_<BlurredSurface, std::shared_ptr<BlurredSurface>>(m, "BlurredSurface")
        .def(py::init<int, int>(), py::arg("width"), py::arg("height"))
        .def(py::init<std::shared_ptr<Surface>>(), py::arg("surface"))
        .def_property_readonly("surface", py::overload_cast<>(&BlurredSurface::get_surface),
                               py::return_value_policy::reference_internal)
        .def("get_surface_ptr", &BlurredSurface::get_surface_ptr)
        .def_property("blur_radius", 
                      &BlurredSurface::get_blur_radius, 
                      &BlurredSurface::set_blur_radius)
        .def("set_blur_radius", &BlurredSurface::set_blur_radius,
             py::arg("radius"), "Set blur radius immediately")
        .def("animate_blur_radius", &BlurredSurface::animate_blur_radius,
             py::arg("target_radius"), py::arg("duration"), py::arg("easing") = 0,
             "Animate blur radius with easing (0=Linear, 1=EaseInQuad, 2=EaseOutQuad, "
             "3=EaseInOutQuad, 4=EaseInCubic, 5=EaseOutCubic, 6=EaseInOutCubic, "
             "7=EaseInExpo, 8=EaseOutExpo, 9=EaseInOutExpo)")
        .def("update", &BlurredSurface::update, py::arg("dt"),
             "Update blur animation")
        .def_property_readonly("animating", &BlurredSurface::is_animating)
        .def("render", &BlurredSurface::render,
             "Return a new blurred surface")
        .def("render_to", &BlurredSurface::render_to,
             py::arg("dest"), py::arg("x"), py::arg("y"),
             "Render blurred content to destination surface")
        .def_property_readonly("width", &BlurredSurface::get_width)
        .def_property_readonly("height", &BlurredSurface::get_height);
    
    // === Blend Modes ===
    py::enum_<BlendMode>(m, "BlendMode")
        .value("Normal", BlendMode::Normal)
        .value("Multiply", BlendMode::Multiply)
        .value("Screen", BlendMode::Screen)
        .value("Overlay", BlendMode::Overlay)
        .value("Add", BlendMode::Add)
        .value("Subtract", BlendMode::Subtract)
        .value("Difference", BlendMode::Difference)
        .value("ColorDodge", BlendMode::ColorDodge)
        .value("ColorBurn", BlendMode::ColorBurn);
    
    // === Material Types ===
    py::enum_<MaterialType>(m, "MaterialType")
        .value("Solid", MaterialType::Solid)
        .value("FrostedGlass", MaterialType::FrostedGlass);
    
    // === Material ===
    py::class_<Material, std::shared_ptr<Material>>(m, "Material")
        .def_static("solid", &Material::solid, "Create a solid material")
        .def_static("frosted_glass", &Material::frosted_glass, 
                    py::arg("blur_radius") = 10.0f,
                    "Create frosted glass material that blurs background")
        .def_property_readonly("type", &Material::get_type)
        .def_property("blur_radius", &Material::get_blur_radius, &Material::set_blur_radius)
        .def("is_solid", &Material::is_solid)
        .def("is_frosted_glass", &Material::is_frosted_glass);
    
    // === Layer ===
    py::class_<Layer, std::shared_ptr<Layer>>(m, "Layer")
        .def(py::init<int, int>())
        .def(py::init<std::shared_ptr<Surface>>())
        .def_property_readonly("surface", py::overload_cast<>(&Layer::get_surface))
        .def("get_surface_ptr", &Layer::get_surface_ptr)
        .def_property_readonly("x", &Layer::get_x)
        .def_property_readonly("y", &Layer::get_y)
        .def("set_position", &Layer::set_position)
        .def("move", &Layer::move)
        .def_property("scale_x", &Layer::get_scale_x, [](Layer& l, float s) { l.set_scale(s, l.get_scale_y()); })
        .def_property("scale_y", &Layer::get_scale_y, [](Layer& l, float s) { l.set_scale(l.get_scale_x(), s); })
        .def("set_scale", py::overload_cast<float, float>(&Layer::set_scale))
        .def("set_scale", py::overload_cast<float>(&Layer::set_scale))
        .def_property("rotation", &Layer::get_rotation, &Layer::set_rotation)
        .def_property("opacity", &Layer::get_opacity, &Layer::set_opacity)
        .def_property("visible", &Layer::is_visible, &Layer::set_visible)
        .def_property("blend_mode", &Layer::get_blend_mode, &Layer::set_blend_mode)
        .def_property("material", &Layer::get_material, &Layer::set_material)
        .def_property("name", &Layer::get_name, &Layer::set_name);
    
    // === LayerStack ===
    py::class_<LayerStack>(m, "LayerStack")
        .def(py::init<int, int>())
        .def("create_layer", &LayerStack::create_layer, py::arg("name") = "")
        .def("create_layer_from_surface", &LayerStack::create_layer_from_surface,
             py::arg("surface"), py::arg("name") = "")
        .def("add_layer", &LayerStack::add_layer)
        .def("remove_layer", py::overload_cast<std::shared_ptr<Layer>>(&LayerStack::remove_layer))
        .def("remove_layer", py::overload_cast<size_t>(&LayerStack::remove_layer))
        .def("clear_layers", &LayerStack::clear_layers)
        .def("get_layer", &LayerStack::get_layer)
        .def("get_layer_by_name", &LayerStack::get_layer_by_name)
        .def_property_readonly("layer_count", &LayerStack::get_layer_count)
        .def("move_layer_up", &LayerStack::move_layer_up)
        .def("move_layer_down", &LayerStack::move_layer_down)
        .def("move_layer_to_top", &LayerStack::move_layer_to_top)
        .def("move_layer_to_bottom", &LayerStack::move_layer_to_bottom)
        .def("set_layer_index", &LayerStack::set_layer_index)
        .def("composite", &LayerStack::composite)
        .def("composite_to", &LayerStack::composite_to)
        .def("set_background", &LayerStack::set_background)
        .def("get_background", &LayerStack::get_background)
        .def_property("background", &LayerStack::get_background, &LayerStack::set_background)
        .def_property_readonly("width", &LayerStack::get_width)
        .def_property_readonly("height", &LayerStack::get_height);
    
    // === Anti-Aliasing Types ===
    py::enum_<AAType>(m, "AAType")
        .value("Off", AAType::Off)
        .value("Basic", AAType::Basic)
        .value("MSAA4", AAType::MSAA4)
        .value("MSAA8", AAType::MSAA8);
    
    // === Anti-Aliasing Settings (singleton exposed as module attribute) ===
    py::class_<AntiAliasingSettings>(m, "AntiAliasingSettings")
        .def("on", &AntiAliasingSettings::on, "Enable anti-aliasing")
        .def("off", &AntiAliasingSettings::off, "Disable anti-aliasing")
        .def_property_readonly("enabled", &AntiAliasingSettings::is_enabled, "Check if AA is enabled")
        .def("get_type", &AntiAliasingSettings::get_type)
        .def("get_samples", &AntiAliasingSettings::get_samples)
        .def("type", py::overload_cast<AAType>(&AntiAliasingSettings::set_type))
        .def("type", py::overload_cast<int>(&AntiAliasingSettings::set_type))
        .def("type", py::overload_cast<const std::string&>(&AntiAliasingSettings::set_type));
    
    // Expose singleton as module attribute 'anti_aliasing'
    m.attr("anti_aliasing") = py::cast(&AntiAliasingSettings::instance(), py::return_value_policy::reference);

    // === Key Enum ===
    py::enum_<Key>(m, "Key")
        .value("Unknown", Key::Unknown)
        .value("Space", Key::Space)
        .value("Quote", Key::Quote)
        .value("Comma", Key::Comma)
        .value("Minus", Key::Minus)
        .value("Period", Key::Period)
        .value("Slash", Key::Slash)
        .value("n0", Key::n0).value("n1", Key::n1).value("n2", Key::n2).value("n3", Key::n3).value("n4", Key::n4)
        .value("n5", Key::n5).value("n6", Key::n6).value("n7", Key::n7).value("n8", Key::n8).value("n9", Key::n9)
        .value("Semicolon", Key::Semicolon)
        .value("Equals", Key::Equals)
        .value("LeftBracket", Key::LeftBracket)
        .value("Backslash", Key::Backslash)
        .value("RightBracket", Key::RightBracket)
        .value("Backquote", Key::Backquote)
        .value("a", Key::a).value("b", Key::b).value("c", Key::c).value("d", Key::d).value("e", Key::e)
        .value("f", Key::f).value("g", Key::g).value("h", Key::h).value("i", Key::i).value("j", Key::j)
        .value("k", Key::k).value("l", Key::l).value("m", Key::m).value("n", Key::n).value("o", Key::o)
        .value("p", Key::p).value("q", Key::q).value("r", Key::r).value("s", Key::s).value("t", Key::t)
        .value("u", Key::u).value("v", Key::v).value("w", Key::w).value("x", Key::x).value("y", Key::y).value("z", Key::z)
        .value("F1", Key::F1).value("F2", Key::F2).value("F3", Key::F3).value("F4", Key::F4)
        .value("F5", Key::F5).value("F6", Key::F6).value("F7", Key::F7).value("F8", Key::F8)
        .value("F9", Key::F9).value("F10", Key::F10).value("F11", Key::F11).value("F12", Key::F12)
        .value("Escape", Key::Escape)
        .value("Return", Key::Return)
        .value("Tab", Key::Tab)
        .value("Backspace", Key::Backspace)
        .value("Insert", Key::Insert)
        .value("Home", Key::Home)
        .value("PageUp", Key::PageUp)
        .value("Delete", Key::Delete)
        .value("End", Key::End)
        .value("PageDown", Key::PageDown)
        .value("Right", Key::Right)
        .value("Left", Key::Left)
        .value("Down", Key::Down)
        .value("Up", Key::Up)
        .value("LCtrl", Key::LCtrl)
        .value("LShift", Key::LShift)
        .value("LAlt", Key::LAlt)
        .value("RCtrl", Key::RCtrl)
        .value("RShift", Key::RShift)
        .value("RAlt", Key::RAlt);

    // === Input Class ===
    py::class_<Input>(m, "Input")
        .def(py::init<>())
        .def("process", &Input::process, "Update input state from event")
        .def("is_key_down", &Input::is_key_down, "Check if key is currently held")
        .def("get_pressed_keys", &Input::get_pressed_keys, "Get list of currently pressed keys in order")
        .def("check_hotkey", &Input::check_hotkey, 
             py::arg("combo"), py::arg("ordered") = false,
             "Check if a key combination is pressed. ordered=True checks for specific press order.");
    
    // === Button Enums ===
    py::enum_<ButtonShape>(m, "ButtonShape")
        .value("Rectangle", ButtonShape::Rectangle)
        .value("RoundedRect", ButtonShape::RoundedRect)
        .value("Circle", ButtonShape::Circle)
        .value("Pill", ButtonShape::Pill)
        .value("Squircle", ButtonShape::Squircle);

    py::enum_<ButtonAnimType>(m, "ButtonAnimType")
        .value("Instant", ButtonAnimType::Instant)
        .value("Linear", ButtonAnimType::Linear)
        .value("Exponential", ButtonAnimType::Exponential);
        



    py::class_<Button, Layer, std::shared_ptr<Button>>(m, "Button")
        .def(py::init<int, int, ButtonShape, int>(),
             py::arg("width"), py::arg("height"), 
             py::arg("shape") = ButtonShape::RoundedRect, 
             py::arg("radius") = 10)
        // Custom Constructor wrapping
        .def(py::init([](int width, int height, ButtonShape shape, int radius,
                         py::dict normal, py::dict hover, py::dict pressed,
                         py::dict text, // NEW argument
                         std::string hover_anim, float duration) {
            auto btn = std::make_shared<Button>(width, height, shape, radius);
            
            if (normal.size() > 0) btn->set_normal_style(parse_style(normal));
            if (hover.size() > 0) btn->set_hover_style(parse_style(hover));
            if (pressed.size() > 0) btn->set_pressed_style(parse_style(pressed));
            if (text.size() > 0) btn->set_text_style(parse_text_style(text));
            
            ButtonAnimType anim = ButtonAnimType::Linear;
            if (hover_anim == "instant") anim = ButtonAnimType::Instant;
            else if (hover_anim == "linear") anim = ButtonAnimType::Linear;
            else if (hover_anim == "exponential") anim = ButtonAnimType::Exponential;
            
            btn->set_hover_animation(anim, duration);
            return btn;
        }), 
        py::arg("width"), py::arg("height"), 
        py::arg("shape") = ButtonShape::RoundedRect, py::arg("radius") = 10,
        py::arg("normal") = py::dict(), py::arg("hover") = py::dict(), py::arg("pressed") = py::dict(),
        py::arg("text") = py::dict(), // NEW argument default
        py::arg("hover_anim") = "linear", py::arg("duration") = 0.1f)
        
        .def("process_event", &Button::process_event)
        .def("update", &Button::update)
        .def_readwrite("on_click", &Button::on_click)
        // Setters for direct style manipulation if needed
        .def("set_normal_style", [](Button& b, py::dict d) { b.set_normal_style(parse_style(d)); })
        .def("set_hover_style", [](Button& b, py::dict d) { b.set_hover_style(parse_style(d)); })
        .def("set_pressed_style", [](Button& b, py::dict d) { b.set_pressed_style(parse_style(d)); })
        .def("set_text_style", [](Button& b, py::dict d) { b.set_text_style(parse_text_style(d)); }) // Exposed setter
        .def("set_hover_animation", &Button::set_hover_animation);

    // === Module-level convenience functions ===
    m.def("init", &init_sdl, "Initialize SDL (called automatically when creating a window)");
    m.def("quit", &quit_sdl, "Quit SDL");
    
    // === Text Input & Clipboard ===
    m.def("start_text_input", [](){ SDL_StartTextInput(); }, "Start accepting text input events");
    m.def("stop_text_input", [](){ SDL_StopTextInput(); }, "Stop accepting text input events");
    m.def("set_clipboard_text", [](const std::string& text){ SDL_SetClipboardText(text.c_str()); }, "Set clipboard text");
    m.def("get_clipboard_text", []() -> std::string {
        char* text = SDL_GetClipboardText();
        std::string result = text ? text : "";
        if (text) SDL_free(text);
        return result;
    }, "Get clipboard text");
    
    // === TextField Enums ===
    py::enum_<TextFieldShape>(m, "TextFieldShape")
        .value("Rectangle", TextFieldShape::Rectangle)
        .value("RoundedRect", TextFieldShape::RoundedRect);
    
    py::enum_<ExpandDirection>(m, "ExpandDirection")
        .value("Up", ExpandDirection::Up)
        .value("Down", ExpandDirection::Down);
    
    // === TextField ===
    py::class_<TextField, Layer, std::shared_ptr<TextField>>(m, "TextField")
        .def(py::init([](int width, int height, TextFieldShape shape, int radius,
                         py::dict placeholder, py::dict text_style,
                         py::dict normal, py::dict hover, py::dict focused,
                         int max_chars, int max_words, bool multiline, bool end_line,
                         std::string expand_dir) {
            auto tf = std::make_shared<TextField>(width, height, shape, radius);
            
            // Parse placeholder
            if (placeholder.size() > 0) {
                PlaceholderStyle ps;
                if (placeholder.contains("text")) ps.text = placeholder["text"].cast<std::string>();
                if (placeholder.contains("font")) ps.font = placeholder["font"].cast<std::string>();
                if (placeholder.contains("font_size")) ps.font_size = placeholder["font_size"].cast<int>();
                if (placeholder.contains("color")) ps.color = placeholder["color"].cast<Color>();
                tf->set_placeholder(ps);
            }
            
            // Parse text style
            if (text_style.size() > 0) {
                TypedTextStyle ts;
                if (text_style.contains("font")) ts.font = text_style["font"].cast<std::string>();
                if (text_style.contains("font_size")) ts.font_size = text_style["font_size"].cast<int>();
                if (text_style.contains("color")) ts.color = text_style["color"].cast<Color>();
                tf->set_text_style(ts);
            }
            
            // Parse state styles
            auto parse_tf_style = [](const py::dict& d) -> TextFieldStyle {
                TextFieldStyle s;
                if (d.contains("color")) s.background_color = d["color"].cast<Color>();
                if (d.contains("opacity")) s.opacity = d["opacity"].cast<float>();
                if (d.contains("blur_radius")) s.blur_radius = d["blur_radius"].cast<float>();
                if (d.contains("scale")) s.scale = d["scale"].cast<float>();
                return s;
            };
            
            if (normal.size() > 0) tf->set_style("normal", parse_tf_style(normal));
            if (hover.size() > 0) tf->set_style("hover", parse_tf_style(hover));
            if (focused.size() > 0) tf->set_style("focused", parse_tf_style(focused));
            
            tf->set_max_chars(max_chars);
            tf->set_max_words(max_words);
            tf->set_multiline(multiline);
            tf->set_end_line(end_line);
            
            if (expand_dir == "up") tf->set_expand_direction(ExpandDirection::Up);
            else tf->set_expand_direction(ExpandDirection::Down);
            
            return tf;
        }),
        py::arg("width"), py::arg("height"),
        py::arg("shape") = TextFieldShape::RoundedRect, py::arg("radius") = 8,
        py::arg("placeholder") = py::dict(), py::arg("text_style") = py::dict(),
        py::arg("normal") = py::dict(), py::arg("hover") = py::dict(), py::arg("focused") = py::dict(),
        py::arg("max_chars") = 0, py::arg("max_words") = 0,
        py::arg("multiline") = false, py::arg("end_line") = true,
        py::arg("expand_direction") = "down")
        
        .def("process_event", &TextField::process_event)
        .def("update", &TextField::update)
        .def("focus", &TextField::focus)
        .def("blur", &TextField::blur)
        .def_property("text", &TextField::get_text, &TextField::set_text)
        .def_property_readonly("is_focused", &TextField::is_focused)
        .def_readwrite("on_change", &TextField::on_change)
        .def_readwrite("on_submit", &TextField::on_submit);
}
