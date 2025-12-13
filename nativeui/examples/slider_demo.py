import Palladium as ui
import math

def main():
    ui.init()
    if ui.is_gpu_available():
        ui.device('gpu')
    else:
        ui.device('cpu')

    if ui.get_device() == 'gpu':
        window = ui.GPUWindow("Slider Demo (GPU)", 800, 600)
        surface = ui.GPUSurface(800, 600)
        print("Running in GPU Mode")
    else:
        window = ui.Window("Slider Demo (CPU)", 800, 600)
        surface = ui.Surface(800, 600)
        print("Running in CPU Mode")

    # Create Sliders

    # --- API REFERENCE ---
    # Common Properties:
    #   slider.set_range(min_val: float, max_val: float)   - Set the value range.
    #   slider.value = float                               - Set/Get current value (0.0 to range).
    #   slider.set_position(x: float, y: float)            - Top-left position.
    #   slider.set_dimensions(width: float, height: float) - Size. 
    #       - For Linear (Rect/Pill): Width = length, Height = thickness/height.
    #       - For Arc: Width = visible width (radius*2 approx), Height = thickness.
    #       - For Selector: Width = visible tape width, Height = total height.
    #   slider.set_colors(bg_col, fill_col, text_col)      - ui.Color(r, g, b, a=255).
    #   slider.set_show_value(bool)                        - Toggle numerical text display.
    #
    # Shape Specific:
    #   - Arc:
    #       slider.set_arc_angles(start: float, sweep: float) - Angles in degrees (0 = East/Right, Clockwise).
    #   - Selector:
    #       slider.set_exponential_stops([v1, v2, ...])       - Custom scaling stops (e.g. [0, 10, 100]).
    #       slider.set_fine_control_enabled(bool)             - Enable Press-and-hold Zoom feature. Or "Zoom for fine control"

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
    # Customization: Hide value text
    # arc_slider.set_show_value(False)

    full_arc_slider = ui.Slider(ui.SliderShape.Arc)
    full_arc_slider.set_range(0, 100)
    full_arc_slider.value = 50
    full_arc_slider.set_position(200, 400) # Center
    full_arc_slider.set_dimensions(80, 15) # Radius, Thickness
    full_arc_slider.set_arc_angles(0, 360) # Full circle
    full_arc_slider.set_show_value(True)

    # Selector Slider
    selector = ui.Slider(ui.SliderShape.Selector)
    selector.set_range(0, 500)
    selector.value = 32
    # Customization: Exponential Scaling
    # Defines stops that are visually equidistant. [0..10] takes same space as [10..50].
    selector.set_exponential_stops([0.0, 10.0, 50.0, 500.0]) 
    selector.set_position(300, 400)
    selector.set_dimensions(400, 150) 
    selector.set_colors(ui.Color(60, 60, 60), ui.Color(255, 255, 255), ui.Color(200, 200, 200))
    # Customization: Fine Control (Default: True)
    # Press and hold (>0.3s) acts as zoom.
    selector.set_fine_control_enabled(True)

    sliders = [rect_slider, pill_slider, arc_slider, full_arc_slider, selector]

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
        dt = window.delta_time
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
