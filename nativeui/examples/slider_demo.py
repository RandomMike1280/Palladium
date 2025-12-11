
import Palladium as ui
import math

def main():
    ui.init()
    if ui.is_gpu_available():
        ui.device('gpu')
    else:
        ui.device('cpu')

    if False:
        window = ui.GPUWindow("Slider Demo (GPU)", 800, 600)
        surface = ui.GPUSurface(800, 600)
    else:
        window = ui.Window("Slider Demo (CPU)", 800, 600)
        surface = ui.Surface(800, 600)
        print("Running in CPU Mode")

    # Create Sliders
    rect_slider = ui.Slider(ui.SliderShape.Rectangle)
    rect_slider.set_range(0, 100)
    rect_slider.value = 50
    rect_slider.set_position(50, 50)
    rect_slider.set_dimensions(300, 20)
    rect_slider.set_colors(ui.Color(60, 60, 60), ui.Color(0, 150, 255), ui.Color(255, 255, 255))
    
    pill_slider = ui.Slider(ui.SliderShape.Pill)
    pill_slider.set_range(0, 100)
    pill_slider.value = 75
    pill_slider.set_position(50, 150)
    pill_slider.set_dimensions(300, 20)
    pill_slider.set_colors(ui.Color(60, 60, 60), ui.Color(255, 100, 100), ui.Color(255, 255, 255))
    
    arc_slider = ui.Slider(ui.SliderShape.Arc)
    arc_slider.set_range(0, 100)
    arc_slider.value = 25
    arc_slider.set_position(600, 300) # Center
    arc_slider.set_dimensions(100, 20) # Radius, Thickness
    arc_slider.set_arc_angles(135, 270) # Start 135, sweep 270 (part of circle)
    arc_slider.set_colors(ui.Color(60, 60, 60), ui.Color(100, 255, 150), ui.Color(255, 255, 255))

    full_arc_slider = ui.Slider(ui.SliderShape.Arc)
    full_arc_slider.set_range(0, 100)
    full_arc_slider.value = 50
    full_arc_slider.set_position(200, 400) # Center
    full_arc_slider.set_dimensions(80, 15) # Radius, Thickness
    full_arc_slider.set_arc_angles(0, 360) # Full circle
    full_arc_slider.set_show_value(True)

    sliders = [rect_slider, pill_slider, arc_slider, full_arc_slider]

    running = True
    while running:
        event = window.poll_event()
        if event:
            if event.type == ui.EventType.Quit:
                running = False
            
            # Pass events to sliders
            for s in sliders:
                s.handle_event(event)
        
        # Update
        dt = 0.016 # Fixed dt for simplicity
        for s in sliders:
            s.update(dt)
            
        # Draw
        surface.clear(ui.Color(30, 30, 30))
        
        for s in sliders:
            s.draw(surface)
            
        if hasattr(surface, 'end_draw'):
            surface.end_draw() # Flush GPU
        
        window.draw(surface)
        
        window.present()

if __name__ == "__main__":
    main()
