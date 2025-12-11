"""
Unified GPU/CPU Demo
Demonstrates the new simplified API: just call ui.device('gpu') once!
"""

import Palladium as ui
import math

def main():
    ui.init()
    
    # ===== THIS IS THE MAGIC LINE =====
    # Switch to GPU mode with a single line - everything else stays the same!
    if ui.is_gpu_available():
        ui.device('gpu')
        print(f"Using: {ui.get_device().upper()} mode")
    else:
        ui.device('cpu')
        print(f"Using: {ui.get_device().upper()} mode (GPU not available)")
    
    # Create window and surfaces
    window = ui.create_window("Unified API Demo", 800, 600)
    background = ui.create_surface(800, 600)
    
    # Increase overlay size to accommodate the large blur radius (40px)
    # Content is ~280x180. We need padding of at least 40px on all sides.
    # 280 + 80 = 360, 180 + 80 = 260. Let's use 400x300 for plenty of room.
    overlay = ui.create_surface(400, 300)
    
    # Draw on surfaces
    background.begin_draw()
    background.fill(ui.Color(25, 30, 40))
    
    # Draw colorful circles
    colors = [
        ui.Color(255, 80, 80),
        ui.Color(80, 255, 80),
        ui.Color(80, 80, 255),
        ui.Color(255, 255, 80),
        ui.Color(255, 80, 255),
    ]
    for i, color in enumerate(colors):
        x = 130 + i * 140
        background.fill_circle(x, 350, 70, color)
    background.end_draw()
    
    # Draw overlay with sharp shapes (will be blurred)
    # Content is centered in the 400x300 surface
    # Rect size 280x180. Center at 200, 150.
    # Top-left of rect: 200 - 140 = 60, 150 - 90 = 60.
    overlay.begin_draw()
    overlay.clear(ui.Color(0, 0, 0, 0))
    overlay.fill_rounded_rect(60, 60, 280, 180, 20, ui.Color(255, 255, 255, 220))
    # Adjust circle positions to be relative to the new rect position
    # old rect was at 10,10. offset is +50,+50.
    overlay.fill_circle(110, 150, 35, ui.Color(255, 50, 50))
    overlay.fill_circle(200, 150, 35, ui.Color(50, 255, 50))
    overlay.fill_circle(290, 150, 35, ui.Color(50, 50, 255))
    overlay.end_draw()
    
    # Apply blur - automatically uses GPU if in GPU mode!
    print("Applying blur...")
    import time
    start = time.perf_counter()
    ui.blur(overlay, 40.0)
    blur_time = (time.perf_counter() - start) * 1000
    print(f"Blur took: {blur_time:.2f}ms")
    
    # Main loop
    event = ui.Event()
    frame_count = 0
    
    while window.is_open:
        while True:
            evt = window.poll_event()
            if evt is None:
                break
            if evt.type == ui.EventType.Quit:
                window.close()
            elif evt.type == ui.EventType.KeyDown:
                if evt.key == 27:  # ESC
                    window.close()
        
        # Render - same API for GPU and CPU!
        window.begin_draw()
        window.clear(ui.Color(25, 30, 40))
        window.draw(background)
        
        # Animate overlay
        t = frame_count * 0.02
        ox = 250 + int(math.sin(t) * 100)
        oy = 180 + int(math.cos(t * 0.7) * 50)
        window.draw(overlay, ox, oy)
        
        window.present()
        frame_count += 1
        
        if frame_count % 60 == 0:
            print(f"FPS: {window.fps:.1f}")
    
    print("Done!")

if __name__ == "__main__":
    main()
