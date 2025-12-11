"""
GPU Acceleration Demo
Demonstrates the new GPU-accelerated rendering in Palladium
"""

import Palladium as ui
import time
import math

def main():
    ui.init()
    
    # Check GPU availability
    if ui.is_gpu_available():
        print("GPU acceleration is available!")
    else:
        print("GPU acceleration not available, using CPU fallback")
        return
    
    # Create GPU window
    window = ui.GPUWindow("GPU Acceleration Demo", 800, 600)
    
    # Create GPU surfaces
    background = ui.GPUSurface(800, 600)
    foreground = ui.GPUSurface(300, 200)
    
    # Draw colorful background
    background.begin_draw()
    background.fill(ui.Color(20, 25, 35))
    
    # Draw some circles
    colors = [
        ui.Color(255, 100, 100),
        ui.Color(100, 255, 100),
        ui.Color(100, 100, 255),
        ui.Color(255, 255, 100),
        ui.Color(255, 100, 255),
    ]
    for i, color in enumerate(colors):
        x = 150 + i * 130
        y = 300 + int(math.sin(i * 0.8) * 100)
        background.fill_circle(x, y, 60, color)
    background.end_draw()
    
    # Draw foreground with pattern that shows blur effect
    foreground.begin_draw()
    foreground.fill(ui.Color(0, 0, 0, 0))  # Start transparent
    
    # Draw sharp shapes that will become blurry
    foreground.fill_rounded_rect(20, 20, 260, 160, 20, ui.Color(255, 255, 255, 200))
    foreground.fill_circle(80, 100, 40, ui.Color(255, 50, 50))
    foreground.fill_circle(150, 100, 40, ui.Color(50, 255, 50))
    foreground.fill_circle(220, 100, 40, ui.Color(50, 50, 255))
    foreground.end_draw()
    
    # Apply GPU blur to foreground (this is FAST!)
    print("Applying GPU blur...")
    start = time.perf_counter()
    ui.GPUEffects.gaussian_blur(foreground, 15.0)
    blur_time = (time.perf_counter() - start) * 1000
    print(f"GPU blur took: {blur_time:.2f}ms")
    
    # Main loop
    event = ui.Event()
    frame_count = 0
    
    while window.is_open:
        while window.poll_event(event):
            if event.type == ui.EventType.Quit:
                window.close()
            elif event.type == ui.EventType.KeyDown:
                if event.key == 27:  # ESC
                    window.close()
        
        # Render
        window.begin_draw()
        window.clear(ui.Color(20, 25, 35))
        window.draw(background)
        
        # Animate foreground position
        t = frame_count * 0.02
        fx = 250 + int(math.sin(t) * 100)
        fy = 200 + int(math.cos(t * 0.7) * 50)
        window.draw(foreground, fx, fy, 0.9)
        
        window.present()
        frame_count += 1
        
        # Print FPS occasionally
        if frame_count % 60 == 0:
            print(f"FPS: {window.fps:.1f}")
    
    print("Done!")

if __name__ == "__main__":
    main()
