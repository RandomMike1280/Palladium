"""
NativeUI Component Demo - Buttons
Demonstrates interactive buttons with shapes, custom states, and animations.
"""

import Palladium as ui

def main():
    ui.init()
    window = ui.Window("NativeUI Buttons Demo", 1000, 700)
    
    # Enable AA for smooth shapes
    ui.anti_aliasing.on()
    ui.anti_aliasing.type(ui.AAType.MSAA4)
    
    # Create Layer Stack
    stack = ui.LayerStack(1000, 700)
    stack.set_background(ui.Color(30, 30, 35))
    
    # --- 1. Standard Material Button ---
    btn_simple = ui.Button(160, 50, ui.ButtonShape.RoundedRect, 8,
        normal={'color': ui.Color(60, 60, 70)},
        hover={'color': ui.Color(80, 80, 90), 'scale': 1.05},
        pressed={'color': ui.Color(40, 40, 50), 'scale': 0.95},
        hover_anim="exponential", duration=0.2
    )
    btn_simple.set_position(100, 100)
    btn_simple.on_click = lambda: print("Simple Button Clicked!")
    stack.add_layer(btn_simple)
    
    # --- 2. Pill Button (Vibrant) ---
    btn_primary = ui.Button(200, 60, ui.ButtonShape.Pill, 0,
        normal={'color': ui.Color(0, 120, 255)},
        hover={'color': ui.Color(50, 150, 255), 'scale': 1.1},
        pressed={'color': ui.Color(0, 90, 200), 'scale': 0.9},
        hover_anim="linear", duration=0.1
    )
    btn_primary.set_position(300, 100)
    # Move to center vert
    btn_primary.move(0, 10) # align visually
    btn_primary.on_click = lambda: print("Primary Action!")
    stack.add_layer(btn_primary)
    
    # --- 3. Frosted Glass Button (Squircle) ---
    btn_glass = ui.Button(150, 150, ui.ButtonShape.Squircle, 0,
        normal={'color': ui.Color(255, 255, 255, 80), 'blur_radius': 10.0},
        hover={'color': ui.Color(255, 255, 255, 120), 'blur_radius': 25.0, 'scale': 1.05},
        pressed={'color': ui.Color(255, 255, 255, 160), 'blur_radius': 50.0, 'scale': 0.95},
        hover_anim="exponential", duration=0.3
    )
    btn_glass.set_position(100, 300)
    btn_glass.on_click = lambda: print("Glass Button Pressed!")
    stack.add_layer(btn_glass)
    
    # --- 4. Circle FAB ---
    btn_fab = ui.Button(100, 100, ui.ButtonShape.Circle, 0,
        normal={'color': ui.Color(255, 80, 80)},
        hover={'color': ui.Color(255, 100, 100), 'scale': 1.2},
        pressed={'color': ui.Color(200, 50, 50), 'scale': 0.9},
        hover_anim="exponential", duration=0.4
    )
    btn_fab.set_position(300, 335)
    btn_fab.on_click = lambda: print("FAB Clicked!")
    stack.add_layer(btn_fab)

    # --- Background Content (to show glass effect) ---
    # Create a layer behind glass button
    bg_shape = ui.Layer(ui.Surface(200, 200))
    bg_shape.surface.fill_circle(100, 100, 40, ui.Color(200, 100, 255))
    bg_shape.set_position(80, 280)
    stack.add_layer(bg_shape)
    stack.move_layer_to_bottom(bg_shape)

    running = True
    clock_time = 0.0
    
    while running:
        event = window.poll_event()
        if event:
            if event.type == ui.EventType.Quit:
                running = False
            
            # Dispatch event to buttons manually (in a real UI, a Scene/loop would do this)
            # Or LayerStack could have 'process_event'
            # For now, explicit dispatch
            btn_simple.process_event(event)
            btn_primary.process_event(event)
            btn_glass.process_event(event)
            btn_fab.process_event(event)

            # Optional: Allow dragging the background shape to see blur update
            # (Simple drag logic omitted for brevity, just static demo)

        # Animate
        dt = 0.016 # approx 60fps
        btn_simple.update(dt)
        btn_primary.update(dt)
        btn_glass.update(dt)
        btn_fab.update(dt)
        
        # Render
        # Composite stack
        final_surface = stack.composite()
        window.present(final_surface)
        
    window.close()
    ui.quit()

if __name__ == "__main__":
    main()
