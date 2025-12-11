
import Palladium as ui
import math
import sys

def main():
    # Check for force CPU
    force_cpu = len(sys.argv) > 1 and sys.argv[1] == 'cpu'
    
    if force_cpu:
        ui.device('cpu')
        print("Forced CPU mode")
    elif ui.is_gpu_available():
        ui.device('gpu')
        print("Running in GPU mode")
    else:
        ui.device('cpu')
        print("GPU not available, falling back to CPU mode")

    window = ui.create_window("Advanced Text Demo", 800, 600)
    surface = ui.create_surface(800, 600)
    
    # 1. Main Title with Shadow
    title = ui.Text("Advanced Text Object", "Arial", 40)
    title.color = ui.Color(255, 255, 255, 255)
    title.set_position(50, 30)
    # White text with black soft shadow
    title.set_shadow(ui.Color(0, 0, 0, 180), 3, 3, 4)
    
    # 2. Subtitle
    subtitle = ui.Text("Features: Wrapping, Alignment, Effects, Animation", "Segoe UI", 20)
    subtitle.color = ui.Color(200, 200, 200, 255)
    subtitle.set_position(50, 80)
    
    # 3. Wrapping Box (Left Aligned)
    lorem = "This text is automatically wrapped to fit within a 250px wide column. It uses the standard Arial font."
    col1 = ui.Text(lorem, "Arial", 16)
    col1.width = 250
    col1.set_align(ui.TextAlign.Left)
    col1.set_position(50, 150)
    
    # 4. Wrapping Box (Justified)
    lorem2 = "This column exemplifies justified text alignment, spreading words evenly across the full width of 250px."
    col2 = ui.Text(lorem2, "Times New Roman", 16)
    col2.width = 250
    col2.set_align(ui.TextAlign.Justified)
    col2.set_position(350, 150)
    
    # 5. Outlined Text (Poor Man's Stroke)
    outline_txt = ui.Text("OUTLINE", "Verdana", 60)
    outline_txt.color = ui.Color(255, 50, 50, 255)
    outline_txt.set_outline(ui.Color(255, 255, 255, 255), 2)
    outline_txt.set_position(50, 300)
    
    # 6. Animated Text
    anim_txt = ui.Text("Bounce & Color", "Consolas", 30)
    anim_txt.set_position(400, 350)
    
    # 7. Font Loading Test (if file exists)
    # custom_font = ui.load_font("assets/CustomFont.ttf")
    
    running = True
    elapsed_time = 0.0
    
    while running:
        # Event Loop
        while True:
            event = window.poll_event()
            if not event:
                break
            if event.type == ui.EventType.Quit:
                running = False
            elif event.type == ui.EventType.KeyDown:
                if event.key.scancode == ui.Scancode.ESCAPE:
                    running = False
        
        # Update Animation
        elapsed_time += window.delta_time
        t = elapsed_time
        
        # Animate color (Rainbow)
        r = int((math.sin(t) + 1) * 127.5)
        g = int((math.sin(t + 2) + 1) * 127.5)
        b = int((math.sin(t + 4) + 1) * 127.5)
        anim_txt.color = ui.Color(r, g, b, 255)
        
        # Animate position
        anim_y = 350 + math.sin(t * 5) * 20
        anim_txt.y = anim_y
        
        # Render
        surface.clear(ui.Color(40, 44, 52, 255)) # Dark blue-grey bg
        
        title.draw(surface)
        subtitle.draw(surface)
        col1.draw(surface)
        col2.draw(surface)
        outline_txt.draw(surface)
        anim_txt.draw(surface)
        
        if hasattr(surface, 'end_draw'):
            surface.end_draw() # Flush drawing commands before using as source
        
        window.draw(surface)
        window.present()
        window.set_target_fps(60)

if __name__ == "__main__":
    main()
