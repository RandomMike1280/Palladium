"""
NativeUI Shapes Demo
Demonstrates Rounded Rectangles, Pills, and Squircles
"""

import Palladium as ui
import math

ui.device('gpu')

def main():
    ui.init()
    window = ui.Window("NativeUI Shapes Demo", 1000, 600)
    
    # Enable AA
    ui.anti_aliasing.on()
    ui.anti_aliasing.type(ui.AAType.MSAA4)
    
    surface = ui.Surface(1000, 600)
    
    bg_color = ui.Color(30, 30, 40)
    running = True
    
    time = 0.0
    
    while running:
        event = window.poll_event()
        if event and event.type == ui.EventType.Quit:
            running = False
            
        time += 0.05
        
        surface.fill(bg_color)
        
        # 1. Rounded Rectangles
        # Row 1
        y = 50
        x = 50
        
        # Various radii
        for i in range(5):
            radius = (i + 1) * 10
            # Filled
            color = ui.Color(200, 100, 100)
            surface.fill_round_rect(x, y, 100, 80, radius, color)
            
            # Outline
            outline_color = ui.Color(255, 255, 255)
            # Offset slightly to show outlining
            surface.draw_round_rect(x, y + 100, 100, 80, radius, outline_color)
            
            x += 150
            
        # 2. Pills
        y = 250
        x = 50
        
        # Horizontal Pills
        for i in range(3):
            w = 120 + i * 40
            h = 60
            color = ui.Color(100, 200, 100)
            surface.fill_pill(x, y, w, h, color)
            
            # Outline Pill
            surface.draw_pill(x, y + 80, w, h, ui.Color(255, 255, 255))
            
            x += w + 20
            
        # 3. Squircles
        y = 450
        x = 50
        
        for i in range(4):
            sz = 80 + i * 20
            color = ui.Color(100, 100, 200)
            
            # Animated color
            r = int((math.sin(time + i) + 1) * 127)
            g = int((math.cos(time + i) + 1) * 127)
            b = 200
            
            surface.fill_squircle(x, y, sz, sz, ui.Color(r, g, b))
            surface.draw_squircle(x, y, sz, sz, ui.Color(255, 255, 255))
            
            x += sz + 30
            
        window.present(surface)
        
    window.close()
    ui.quit()

if __name__ == "__main__":
    main()
