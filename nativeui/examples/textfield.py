"""TextField Demo - Tests the new TextField component"""
import Palladium as ui

def main():
    ui.init()
    window = ui.Window("TextField Demo", 800, 500, vsync=True)
    stack = ui.LayerStack(800, 500)
    stack.background = ui.Color(25, 25, 35)
    
    # Simple text field
    tf1 = ui.TextField(
        width=300, height=40,
        shape=ui.TextFieldShape.RoundedRect,
        radius=8,
        placeholder={'text': "Type something here...", 'color': ui.Color(100, 100, 120)},
        text_style={'color': ui.Color(255, 255, 255), 'font_size': 16},
        normal={'color': ui.Color(45, 45, 55)},
        hover={'color': ui.Color(55, 55, 65)},
        focused={'color': ui.Color(65, 65, 80)}
    )
    tf1.set_position(50, 50)
    stack.add_layer(tf1)
    
    # With character limit
    tf2 = ui.TextField(
        width=300, height=40,
        placeholder={'text': "Max 20 characters...", 'color': ui.Color(100, 100, 120)},
        text_style={'color': ui.Color(200, 255, 200)},
        normal={'color': ui.Color(35, 55, 45)},
        hover={'color': ui.Color(45, 65, 55)},
        focused={'color': ui.Color(55, 85, 65)},
        max_chars=20
    )
    tf2.set_position(50, 120)
    stack.add_layer(tf2)
    
    # With frosted glass - use moderate alpha, blur does the transparency
    tf3 = ui.TextField(
        width=300, height=40,
        placeholder={'text': "Frosted glass style...", 'color': ui.Color(200, 200, 220)},
        text_style={'color': ui.Color(255, 255, 255)},
        normal={'color': ui.Color(60, 60, 80, 180), 'blur_radius': 15},
        hover={'color': ui.Color(70, 70, 90, 190), 'blur_radius': 18},
        focused={'color': ui.Color(80, 80, 100, 200), 'blur_radius': 20}
    )
    tf3.set_position(50, 190)
    stack.add_layer(tf3)
    
    # Rectangle shape
    tf4 = ui.TextField(
        width=300, height=40,
        shape=ui.TextFieldShape.Rectangle,
        placeholder={'text': "Rectangle field...", 'color': ui.Color(100, 100, 120)},
        text_style={'color': ui.Color(255, 200, 100)},
        normal={'color': ui.Color(55, 45, 35)},
        hover={'color': ui.Color(65, 55, 45)},
        focused={'color': ui.Color(80, 65, 55)}
    )
    tf4.set_position(50, 260)
    stack.add_layer(tf4)
    
    # Status label (using a button as label)
    status = ui.Button(300, 30, ui.ButtonShape.RoundedRect, 5,
        normal={'color': ui.Color(30, 30, 40, 200)},
        text={'text': "Click a field to focus", 'font_size': 12, 'color': ui.Color(150, 150, 150)}
    )
    status.set_position(50, 350)
    stack.add_layer(status)
    
    # Callbacks
    def on_tf1_change(text):
        print(f"Field 1: {text}")
    
    def on_tf1_submit(text):
        print(f"Submitted: {text}")
        
    tf1.on_change = on_tf1_change
    tf1.on_submit = on_tf1_submit
    
    fields = [tf1, tf2, tf3, tf4]
    
    running = True
    while running:
        while True:
            event = window.poll_event()
            if event is None: break
            if event.type == ui.EventType.Quit: running = False
            if event.type == ui.EventType.KeyDown and event.key == 27: running = False
            
            for tf in fields:
                tf.process_event(event)
        
        dt = window.delta_time
        for tf in fields:
            tf.update(dt)
        
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
