#!/usr/bin/env python3
"""
Simple NativeUI Test

A minimal example to verify the library works correctly.
This creates a window, draws some shapes, and handles events.
"""

import Palladium as ui

def main():
    print("Simple NativeUI Test")
    print("Press ESC to exit, click to draw circles")
    
    # Create a window
    window = ui.Window("NativeUI Simple Test", 640, 480)
    
    # Create a surface
    surface = ui.Surface(640, 480)
    
    # Fill with a gradient
    ui.Effects.radial_gradient(
        surface, 320, 240, 400,
        ui.Color(70, 130, 180, 255),   # Steel blue center
        ui.Color(25, 25, 50, 255)       # Dark blue edge
    )
    
    # Draw some shapes
    # Red rectangle
    surface.fill_rect(50, 50, 100, 80, ui.Color(255, 100, 100, 200))
    surface.draw_rect(50, 50, 100, 80, ui.Color(255, 255, 255, 255))
    
    # Green circle
    surface.fill_circle(320, 240, 60, ui.Color(100, 255, 100, 200))
    surface.draw_circle(320, 240, 60, ui.Color(255, 255, 255, 255))
    
    # Blue line
    surface.draw_line(400, 50, 600, 150, ui.Color(100, 100, 255, 255))
    
    # Yellow filled circle
    surface.fill_circle(550, 350, 40, ui.Color(255, 255, 100, 200))
    
    print("Window created successfully!")
    print(f"Window size: {window.width}x{window.height}")
    
    # Simple animation
    bounce = ui.SpringAnimation.wobbly(240)
    bounce.set_value(100)
    
    # Main loop
    running = True
    while running and window.is_open:
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
                elif event.key == ord(' '):
                    # Space: reset animation
                    bounce.set_value(100)
            elif event.type == ui.EventType.MouseButtonDown:
                # Draw a circle where clicked
                color = ui.Color(
                    200 + (event.mouse_x % 55),
                    100 + (event.mouse_y % 155),
                    150,
                    200
                )
                surface.fill_circle(event.mouse_x, event.mouse_y, 20, color)
        
        # Update animation
        dt = window.delta_time if window.delta_time > 0 else 1/60
        bounce.set_target(240)
        y = bounce.update(dt)
        
        # Create a copy and add animated element
        display = surface.copy()
        display.fill_circle(100, int(y), 30, ui.Color(255, 200, 100, 230))
        
        # Present to window
        window.present(display)
    
    print("Test completed successfully!")


if __name__ == "__main__":
    main()
