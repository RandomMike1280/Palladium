# NativeUI

A low-level Python UI library written in C++, offering pixel-level manipulation, hardware-accelerated rendering, native animations, and visual effects.

## Features

- **Pixel-Level Control**: Direct access to pixels with `set_pixel`, `get_pixel`
- **Drawing Primitives**: Lines, rectangles, circles with anti-aliasing
- **Layer Compositing**: Multiple layers with blend modes (Normal, Multiply, Screen, Overlay, etc.)
- **Animations**:
  - Easing functions: Linear, Quad, Cubic, Expo, Elastic, Back, Bounce
  - Spring physics: Realistic bounce with configurable stiffness/damping
- **Visual Effects**:
  - Blur (box and gaussian)
  - Frosted glass effect
  - Pixel displacement and wave distortion
  - Color adjustments (brightness, contrast, saturation, hue shift)
  - Gradients (linear and radial)
  - Noise generation (random and perlin)
- **Hardware Acceleration**: SDL2 backend with VSync support

## Requirements

- Python 3.8+
- SDL2 development libraries
- pybind11
- C++17 compatible compiler

## Installation

### 1. Install SDL2

**Windows (vcpkg):**
```powershell
vcpkg install sdl2:x64-windows
```

**Windows (manual):**
1. Download SDL2 development libraries from https://libsdl.org
2. Extract to a folder (e.g., `C:\SDL2`)
3. Set environment variable: `set SDL2_DIR=C:\SDL2`

### 2. Install Python dependencies

```bash
pip install pybind11 numpy
```

### 3. Build and install

```bash
cd nativeui
pip install -e .
```

## Quick Example

```python
import Palladium as ui

# Create window
window = ui.Window("Hello NativeUI", 800, 600)

# Create surface
surface = ui.Surface(800, 600)

# Draw a gradient background
ui.Effects.linear_gradient(
    surface, 0, 0, 800, 600,
    ui.Color(50, 50, 100),
    ui.Color(20, 20, 40)
)

# Draw a circle
surface.fill_circle(400, 300, 50, ui.Color(255, 100, 100, 200))

# Create a spring animation
spring = ui.SpringAnimation.wobbly(300)
spring.set_value(100)

# Main loop
while window.is_open:
    event = window.poll_event()
    if event and event.type == ui.EventType.Quit:
        break
    
    # Update animation
    spring.set_target(300)
    y = spring.update(window.delta_time or 0.016)
    
    # Draw
    display = surface.copy()
    display.fill_circle(400, int(y), 30, ui.Color(100, 255, 100))
    
    window.present(display)
```

## API Reference

### Core Classes

| Class | Description |
|-------|-------------|
| `Window` | SDL2 window with event handling |
| `Surface` | RGBA pixel buffer with drawing methods |
| `Color` | RGBA color (0-255) |
| `Layer` | Single layer with position, opacity, blend mode |
| `LayerStack` | Multiple layers with compositing |

### Animation Classes

| Class | Description |
|-------|-------------|
| `Animation` | Time-based animation with easing |
| `SpringAnimation` | Physics-based spring animation |
| `Easing` | Easing function utilities |

### Effects (Static Methods)

| Method | Description |
|--------|-------------|
| `Effects.box_blur(surface, radius)` | Fast blur |
| `Effects.gaussian_blur(surface, sigma)` | Quality blur |
| `Effects.frosted_glass(surface, blur_radius, noise, saturation)` | Glass effect |
| `Effects.brightness/contrast/saturation(surface, amount)` | Color adjustments |
| `Effects.linear_gradient/radial_gradient(...)` | Gradient fills |
| `Effects.wave_distort/ripple(...)` | Pixel displacement |

## License

MIT License
