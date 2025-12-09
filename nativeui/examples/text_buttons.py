
import Palladium as ui

def main():
    ui.init()
    window = ui.Window("Button Text Verification", 1000, 600, vsync=True)
    stack = ui.LayerStack(1000, 600)
    stack.background = ui.Color(30, 30, 40)
    
    # 1. Default (Center)
    b1 = ui.Button(200, 60, ui.ButtonShape.RoundedRect, 10,
        normal={'color': ui.Color(50, 100, 200)},
        hover={'color': ui.Color(70, 120, 220), 'scale': 1.05},
        pressed={'color': ui.Color(40, 90, 180)},
        text={'text': "Default Center", 'font_size': 20}
    )
    b1.set_position(50, 50)
    stack.add_layer(b1)
    
    # 2. Top-Left with padding
    b2 = ui.Button(200, 60, ui.ButtonShape.RoundedRect, 10,
        normal={'color': ui.Color(200, 100, 50)},
        hover={'color': ui.Color(220, 120, 70), 'scale': 1.05},
        pressed={'color': ui.Color(180, 90, 40)},
        text={'text': "Top-Left (Pad 10)", 'position': "top left", 'padding': 10, 'font_size': 16}
    )
    b2.set_position(300, 50)
    stack.add_layer(b2)
    
    # 3. Left (Should fail V padding, center vertically)
    b3 = ui.Button(200, 60, ui.ButtonShape.RoundedRect, 10,
        normal={'color': ui.Color(50, 200, 100)},
        hover={'color': ui.Color(70, 220, 120), 'scale': 1.05},
        pressed={'color': ui.Color(40, 180, 90)},
        text={'text': "Left (Pad 20)", 'position': "left", 'padding': 20, 'font_size': 16, 'color': ui.Color(0, 0, 0)}
    )
    b3.set_position(550, 50)
    stack.add_layer(b3)
    
    # 4. Top Center (Should fail H padding, center horizontally)
    b4 = ui.Button(200, 60, ui.ButtonShape.RoundedRect, 10,
        normal={'color': ui.Color(150, 50, 200)},
        hover={'color': ui.Color(170, 70, 220), 'scale': 1.05},
        pressed={'color': ui.Color(130, 40, 180)},
        text={'text': "Top Center (Pad 10)", 'position': "top center", 'padding': 10, 'font_size': 16}
    )
    b4.set_position(50, 150)
    stack.add_layer(b4)
    
    # 5. Bottom Right
    b5 = ui.Button(200, 60, ui.ButtonShape.RoundedRect, 10,
        normal={'color': ui.Color(100, 100, 100)},
        hover={'color': ui.Color(120, 120, 120), 'scale': 1.05},
        pressed={'color': ui.Color(80, 80, 80)},
        text={'text': "Bottom Right", 'position': "bottom right", 'padding': (5, 15), 'font_size': 16}
    )
    b5.set_position(300, 150)
    stack.add_layer(b5)
    
    running = True
    while running:
        while True:
            event = window.poll_event()
            if event is None: break
            if event.type == ui.EventType.Quit: running = False
            elif event.type == ui.EventType.KeyDown and event.key == 27: running = False
            
            b1.process_event(event)
            b2.process_event(event)
            b3.process_event(event)
            b4.process_event(event)
            b5.process_event(event)
            
        b1.update(window.delta_time)
        b2.update(window.delta_time)
        b3.update(window.delta_time)
        b4.update(window.delta_time)
        b5.update(window.delta_time)
        
        final = stack.composite()
        window.present(final)
    
    window.close()
    ui.quit()

if __name__ == "__main__":
    import traceback
    try:
        main()
    except Exception as e:
        print(f"Error: {e}")
        traceback.print_exc()
