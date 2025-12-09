"""
NativeUI Input System Demo
Demonstrates key tracking and ordered/unordered hotkeys
"""

import Palladium as ui

def main():
    ui.init()
    window = ui.Window("Input Demo - Press Keys!", 800, 600)
    
    # Input manager
    input_manager = ui.Input()
    
    # Visuals
    font_color = ui.Color(255, 255, 255)
    bg_color = ui.Color(20, 20, 30)
    
    # Hotkey definitions
    hotkeys = [
        {
            "name": "Save (Ctrl+S)",
            "combo": [ui.Key.LCtrl, ui.Key.s],
            "ordered": False,
            "status": "Waiting..."
        },
        {
            "name": "Copy (Ctrl+C)",
            "combo": [ui.Key.LCtrl, ui.Key.c],
            "ordered": False,
            "status": "Waiting..."
        },
        {
            "name": "Paste (Ctrl+V)",
            "combo": [ui.Key.LCtrl, ui.Key.v],
            "ordered": False,
            "status": "Waiting..."
        },
        {
            "name": "Ordered (A then B)",
            "combo": [ui.Key.a, ui.Key.b],
            "ordered": True,
            "status": "Waiting..."
        }
    ]
    
    surface = ui.Surface(800, 600)
    
    # Basic text rendering helper (simulated with rectangles/shapes for now as we lack text engine)
    # We will just print to console for confirmation as well
    
    running = True
    while running:
        # Event loop
        while True:
            event = window.poll_event()
            if event is None:
                break
            
            # Feed event to input manager
            input_manager.process(event)
            
            if event.type == ui.EventType.Quit:
                running = False
            elif event.type == ui.EventType.KeyDown:
                print(f"Key Pressed: {event.key}")
                
        # Update hotkey status
        for hk in hotkeys:
            if input_manager.check_hotkey(hk["combo"], hk["ordered"]):
                hk["status"] = "ACTIVE!"
            else:
                hk["status"] = "Waiting..."
                
        # Render info
        surface.fill(bg_color)
        
        # Draw status indicators (red=waiting, green=active)
        y = 50
        for hk in hotkeys:
            color = ui.Color(0, 255, 0) if hk["status"] == "ACTIVE!" else ui.Color(255, 50, 50)
            surface.draw_rect(50, y, 20, 20, color)
            surface.fill_rect(50, y, 20, 20, ui.Color(color.r, color.g, color.b, 100))
            y += 40
            
        # Draw pressed keys (simulated visualization)
        pressed = input_manager.get_pressed_keys()
        
        # Visualize pressed keys count
        surface.draw_rect(50, 300, len(pressed) * 20, 20, ui.Color(0, 200, 255))
        
        window.present(surface)
        
        # Console output for verification
        if any(hk["status"] == "ACTIVE!" for hk in hotkeys):
             active_names = [hk["name"] for hk in hotkeys if hk["status"] == "ACTIVE!"]
             # print(f"Active Hotkeys: {active_names}") # Uncomment to flood console
             pass
             
    window.close()
    ui.quit()

if __name__ == "__main__":
    main()
