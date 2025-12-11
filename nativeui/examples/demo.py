#!/usr/bin/env python3
"""
NativeUI Comprehensive Demo

This demonstration showcases all the features of the NativeUI library:
- Window creation and event handling
- Pixel-level drawing operations
- Multiple layers with blend modes
- Spring and easing animations
- Visual effects (blur, frosted glass, gradients)
"""

import Palladium as ui
import math
import random

ui.device('gpu')

# Window dimensions
WIDTH = 800
HEIGHT = 600


def create_gradient_background(surface):
    """Create a beautiful gradient background"""
    ui.Effects.linear_gradient(
        surface, 0, 0, WIDTH, HEIGHT,
        ui.Color(25, 25, 112, 255),   # Midnight blue
        ui.Color(138, 43, 226, 255)   # Blue violet
    )


def create_animated_circle(surface, x, y, radius, color):
    """Draw a filled circle with a glow effect"""
    # Outer glow (larger, semi-transparent)
    glow_color = ui.Color(color.r, color.g, color.b, 50)
    surface.fill_circle(int(x), int(y), radius + 10, glow_color)
    
    # Main circle
    surface.fill_circle(int(x), int(y), radius, color)
    
    # Highlight
    highlight_color = ui.Color(255, 255, 255, 100)
    surface.fill_circle(int(x - radius // 3), int(y - radius // 3), radius // 3, highlight_color)


def create_frosted_panel(width, height):
    """Create a frosted glass panel"""
    panel = ui.Surface(width, height)
    panel.fill(ui.Color(255, 255, 255, 180))
    
    # Add some texture
    ui.Effects.noise(panel, 0.03)
    
    return panel


def main():
    print("NativeUI Demo - Press ESC to exit")
    print("="*40)
    
    # Create window
    window = ui.Window("NativeUI Demo", WIDTH, HEIGHT, vsync=True)
    
    # Create layer stack
    layers = ui.LayerStack(WIDTH, HEIGHT)
    layers.background = ui.Color(0, 0, 0, 255)
    
    # Layer 1: Background gradient
    bg_layer = layers.create_layer("background")
    create_gradient_background(bg_layer.surface)
    
    # Layer 2: Animated shapes
    shapes_layer = layers.create_layer("shapes")
    shapes_layer.set_position(0, 0)
    
    # Layer 3: Frosted glass panel
    panel_width, panel_height = 300, 200
    panel_surface = create_frosted_panel(panel_width, panel_height)
    frosted_layer = layers.create_layer_from_surface(panel_surface.copy(), "frosted_panel")
    frosted_layer.set_position((WIDTH - panel_width) // 2, HEIGHT - panel_height - 50)
    frosted_layer.opacity = 0.85
    frosted_layer.blend_mode = ui.BlendMode.Normal
    
    # Layer 4: UI overlay
    ui_layer = layers.create_layer("ui")
    
    # Create spring animations for bouncing circles
    circles = []
    num_circles = 5
    for i in range(num_circles):
        circle = {
            'x': random.randint(100, WIDTH - 100),
            'spring_y': ui.SpringAnimation.wobbly(random.randint(100, HEIGHT - 200)),
            'target_y': random.randint(150, HEIGHT - 250),
            'radius': random.randint(20, 50),
            'color': ui.Color(
                random.randint(100, 255),
                random.randint(100, 255),
                random.randint(100, 255),
                200
            )
        }
        circle['spring_y'].set_value(100)  # Start from top
        circles.append(circle)
    
    # Easing animation for panel slide-in
    panel_anim = ui.Animation(
        start_value=HEIGHT + 50,
        end_value=HEIGHT - panel_height - 50,
        duration=1.5,
        easing=ui.EasingType.EaseOutElastic
    )
    
    # Wave effect animation
    wave_phase = 0.0
    
    # Ripple effect state
    ripple_active = False
    ripple_x, ripple_y = 0, 0
    ripple_phase = 0.0
    
    # FPS counter
    frame_count = 0
    
    # Main loop
    running = True
    while running and window.is_open:
        dt = window.delta_time
        if dt <= 0:
            dt = 1.0 / 60.0  # Default to 60 FPS
        
        # Handle events
        while True:
            event = window.poll_event()
            if event is None:
                break
            
            if event.type == ui.EventType.Quit:
                running = False
            elif event.type == ui.EventType.KeyDown:
                if event.key == 27:  # ESC
                    running = False
                elif event.key == ord('r') or event.key == ord('R'):
                    # Reset animations
                    for circle in circles:
                        circle['spring_y'].set_value(100)
                        circle['target_y'] = random.randint(150, HEIGHT - 250)
                    panel_anim.restart()
            elif event.type == ui.EventType.MouseButtonDown:
                ripple_active = True
                ripple_x = event.mouse_x
                ripple_y = event.mouse_y
                ripple_phase = 0.0
        
        # Update animations
        for circle in circles:
            circle['spring_y'].set_target(circle['target_y'])
            circle['spring_y'].update(dt)
            
            # Randomly change target occasionally
            if random.random() < 0.01:
                circle['target_y'] = random.randint(150, HEIGHT - 250)
        
        panel_y = panel_anim.update(dt)
        wave_phase += dt * 2.0
        
        if ripple_active:
            ripple_phase += dt * 5.0
            if ripple_phase > 10.0:
                ripple_active = False
        
        # Clear shapes layer
        shapes_layer.surface.clear()
        
        # Draw animated circles
        for circle in circles:
            y = circle['spring_y'].value
            create_animated_circle(
                shapes_layer.surface,
                circle['x'],
                y,
                circle['radius'],
                circle['color']
            )
        
        # Update frosted panel position
        frosted_layer.set_position((WIDTH - panel_width) // 2, int(panel_y))
        
        # Clear and redraw UI layer
        ui_layer.surface.clear()
        
        # Draw text info on UI layer (simple pixel text)
        fps_text = f"FPS: {window.fps:.0f}"
        info_color = ui.Color(255, 255, 255, 200)
        
        # Draw simple pixel "FPS" indicator (top-left corner block)
        ui_layer.surface.fill_rect(10, 10, 80, 20, ui.Color(0, 0, 0, 150))
        ui_layer.surface.draw_rect(10, 10, 80, 20, info_color)
        
        # Composite all layers
        result = layers.composite()
        
        # Apply wave distortion effect on the background only (create a copy)
        if frame_count % 3 == 0:  # Apply less frequently for performance
            # Apply subtle wave to background
            pass  # Skipping for performance in demo
        
        # Apply ripple effect if active
        if ripple_active:
            ui.Effects.ripple(result, ripple_x, ripple_y, 5.0 * (1.0 - ripple_phase / 10.0), 30.0, ripple_phase)
        
        # Present to window
        window.present(result)
        
        frame_count += 1
    
    print("Demo ended. Thank you for trying NativeUI!")


if __name__ == "__main__":
    main()
