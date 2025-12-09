"""
NativeUI Materials Demo
Demonstrates Solid and FrostedGlass materials
"""

import Palladium as ui
import math
import random

def main():
    ui.init()
    window = ui.Window("Materials Demo", 800, 600, vsync=True)
    
    # Create layer stack
    stack = ui.LayerStack(800, 600)
    stack.background = ui.Color(20, 20, 30)
    
    # 1. Background Layer - Animated shapes
    bg_surface = ui.Surface(800, 600)
    bg_layer = stack.create_layer_from_surface(bg_surface, "background")
    
    # Store shapes for animation
    shapes = []
    for _ in range(50):
        shapes.append({
            'x': random.randint(0, 800),
            'y': random.randint(0, 600),
            'radius': random.randint(10, 40),
            'dx': random.uniform(-2, 2),
            'dy': random.uniform(-2, 2),
            'color': ui.Color(
                random.randint(50, 255),
                random.randint(50, 255),
                random.randint(50, 255)
            )
        })
    
    # 2. Frosted Glass Layer - The draggable lens
    glass_surface = ui.Surface(200, 200)
    # Draw a semi-transparent white rect with rounded corners (simulated by drawing circles at corners)
    glass_color = ui.Color(255, 255, 255, 40)
    glass_surface.fill(glass_color)
    glass_surface.draw_rect(0, 0, 200, 200, ui.Color(255, 255, 255, 100)) # Border
    
    glass_layer = stack.create_layer_from_surface(glass_surface, "glass")
    glass_layer.set_position(300, 200)
    
    # Apply Frosted Glass Material
    # Blur radius 15.0 initially
    glass_layer.material = ui.Material.frosted_glass(15.0)
    
    # 3. Solid Layer - Control object
    solid_surface = ui.Surface(150, 150)
    solid_surface.fill(ui.Color(100, 200, 255))
    solid_surface.draw_rect(0, 0, 150, 150, ui.Color(255, 255, 255))
    
    solid_layer = stack.create_layer_from_surface(solid_surface, "solid")
    solid_layer.set_position(550, 250)
    solid_layer.material = ui.Material.solid() # Default, but explicit for demo
    
    running = True
    while running:
        # Handle events
        while True:
            event = window.poll_event()
            if event is None:
                break
            
            if event.type == ui.EventType.Quit:
                running = False
            elif event.type == ui.EventType.KeyDown:
                if event.key == 27: # ESC
                    running = False
            elif event.type == ui.EventType.MouseMotion:
                # Move glass layer with mouse
                glass_layer.set_position(event.mouse_x - 100, event.mouse_y - 100)
            elif event.type == ui.EventType.MouseWheel:
                # Adjust blur radius (Requires CTRL)
                if event.ctrl:
                    mat = glass_layer.material
                    if mat.is_frosted_glass():
                        new_radius = mat.blur_radius + event.wheel_y
                        mat.blur_radius = max(0.0, min(50.0, new_radius))
                        print(f"Blur Radius: {mat.blur_radius}")
        
        # Update background animation
        bg_surface.fill(ui.Color(0, 0, 0, 0)) # Clear
        for s in shapes:
            s['x'] += s['dx']
            s['y'] += s['dy']
            
            # Bounce
            if s['x'] < 0 or s['x'] > 800: s['dx'] *= -1
            if s['y'] < 0 or s['y'] > 600: s['dy'] *= -1
            
            bg_surface.fill_circle(int(s['x']), int(s['y']), s['radius'], s['color'])
        
        # Composite and render
        final_image = stack.composite()
        
        # Draw text info
        final_image.draw_rect(10, 10, 300, 60, ui.Color(0, 0, 0, 180))
        # (Assuming we don't have text rendering yet, just using rect as background for imagined text)
        
        window.present(final_image)
    
    window.close()
    ui.quit()

if __name__ == "__main__":
    main()
