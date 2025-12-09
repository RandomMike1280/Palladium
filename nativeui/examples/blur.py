"""
NativeUI Blur Demo
Demonstrates the BlurredSurface class with animated blur radius
"""

import Palladium as ui
import math

def main():
    # Initialize and create window
    ui.init()
    window = ui.Window("Blur Demo", 800, 600, vsync=True)
    
    # Create multiple blurred circles with different colors
    circles = []
    colors = [
        ui.Color(255, 100, 100),   # Red
        ui.Color(100, 255, 100),   # Green
        ui.Color(100, 100, 255),   # Blue
        ui.Color(255, 200, 50),    # Yellow
        ui.Color(200, 100, 255),   # Purple
    ]
    
    for i, color in enumerate(colors):
        blurred = ui.BlurredSurface(120, 120)
        blurred.surface.fill_circle(60, 60, 50, color)
        circles.append({
            'blur': blurred,
            'x': 100 + i * 140,
            'y': 200,
            'phase': i * 0.5,  # Different phase for wave effect
            'target_blur': 0.0,
            'last_blur': 0.0,
        })
    
    # Animation state
    time_elapsed = 0.0
    
    running = True
    while running:
        dt = window.delta_time
        time_elapsed += dt
        
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
                    # Space: trigger blur animation on all circles
                    for i, circle in enumerate(circles):
                        # Toggle between no blur and heavy blur
                        new_target = 0.0 if circle['blur'].blur_radius > 5 else 20.0
                        # Use different easing for each
                        easing = i % 10  # Cycle through easings
                        circle['blur'].animate_blur_radius(new_target, 0.8, easing)
        
        # Create gradient background
        screen = ui.Surface(800, 600)
        ui.Effects.linear_gradient(screen, 0, 0, 800, 600, 
                                   ui.Color(30, 30, 60), ui.Color(60, 30, 80))
        
        # Animate each circle with wave motion
        for i, circle in enumerate(circles):
            # Update blur animation
            circle['blur'].update(dt)
            
            # Wave motion - blur radius oscillates
            wave = math.sin(time_elapsed * 2.0 + circle['phase'])
            wave_blur = 5.0 + wave * 50.0  # Oscillate between 0 and 10
            
            # Only set wave blur if not animating from user input
            if not circle['blur'].animating:
                circle['blur'].blur_radius = wave_blur
            
            # Vertical wave motion
            y_offset = math.sin(time_elapsed * 1.5 + circle['phase']) * 50
            
            # Render blurred circle
            circle['blur'].render_to(screen, 
                                     circle['x'] - 60, 
                                     int(circle['y'] + y_offset) - 60)
        
        # Draw instructions
        screen.draw_rect(10, 10, 250, 50, ui.Color(0, 0, 0, 150))
        
        # Render to window
        window.present(screen)
    
    window.close()
    ui.quit()

if __name__ == "__main__":
    main()
