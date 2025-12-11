import Palladium as ui
import math
import time
import sys

def main():
    ui.init()
    
    # Try using GPU mode
    if ui.device('gpu'):
        print("Using device: GPU")
    else:
        print("Using device: CPU")
        
    window = ui.create_window("FPS Control Demo", 800, 600)
    surface = ui.create_surface(800, 600)
    
    # Configure initial FPS settings
    window.set_target_fps(60)
    window.set_unfocused_fps(15)
    
    # State for input
    target_fps_str = "60"
    target_fps = 60
    
    rotation = 0.0
    
    print("FPS Control Demo Running")
    print("------------------------")
    print("Type numbers (0-9) to set Target FPS. Press ENTER to apply.")
    print("Press ESC to quit.")
    print("Minimize window to test throttling.")
    print(f"Initial Target FPS: {target_fps}")
    
    running = True
    while running:
        # Event handling
        while True:
            event = window.poll_event()
            if event is None: break
            
            if event.type == ui.EventType.Quit:
                running = False
            elif event.type == ui.EventType.KeyDown:
                if event.key == 27: # ESC
                    running = False
                elif event.key == 13: # Enter
                    try:
                        fps = int(target_fps_str)
                        target_fps = fps
                        window.set_target_fps(target_fps)
                        print(f"Target FPS set to: {target_fps}")
                    except ValueError:
                        pass
                    # Reset string after apply or error
                    target_fps_str = str(target_fps) 
                elif event.key == 8: # Backspace
                    if len(target_fps_str) > 0:
                        target_fps_str = target_fps_str[:-1]
                elif 48 <= event.key <= 57: # 0-9
                    char = chr(event.key)
                    target_fps_str += char
                    print(f"Input: {target_fps_str}")
                    
        # Check window state
        is_focused = window.is_focused
        is_minimized = window.is_minimized
        
        if is_minimized:
            # Minimized behavior
            time.sleep(0.1) # Sleep to prevent tight loop, though update_timing handles it too
            continue
            
        # Update animation
        dt = window.delta_time
        rotation += 90.0 * dt
        
        # Draw
        if hasattr(surface, 'begin_draw'):
            surface.begin_draw()
        
        # Dynamic background color to show focus state
        bg_color = ui.Color(30, 30, 40) if is_focused else ui.Color(50, 20, 20)
        surface.clear(bg_color)
        
        # Draw rotating square
        center_x, center_y = 400, 300
        size = 100
        rad = math.radians(rotation)
        cos_a = math.cos(rad)
        sin_a = math.sin(rad)
        
        points = [
            (-size, -size), (size, -size), (size, size), (-size, size)
        ]
        
        transformed_points = []
        for x, y in points:
            tx = x * cos_a - y * sin_a + center_x
            ty = x * sin_a + y * cos_a + center_y
            transformed_points.append((tx, ty))
            
        color = ui.Color(100, 200, 255)
        for i in range(4):
            p1 = transformed_points[i]
            p2 = transformed_points[(i + 1) % 4]
            surface.draw_line(int(p1[0]), int(p1[1]), int(p2[0]), int(p2[1]), color)
            
        if hasattr(surface, 'end_draw'):
            surface.end_draw()
            
        if hasattr(window, 'draw'):
            window.draw(surface)
            window.present()
        else:
            window.present(surface)
        
        # Update Title with Stats
        current_fps = window.fps
        focus_state = "Focused" if is_focused else "Background"
        title = f"FPS Demo | Real: {current_fps:.1f} | Target: {target_fps} (Input: {target_fps_str}) | {focus_state}"
        window.set_title(title)

if __name__ == "__main__":
    main()
