
import Palladium as ui

def main():
    ui.init()
    window = ui.Window("Simple Text Test", 400, 300, vsync=True)
    stack = ui.LayerStack(400, 300)
    stack.background = ui.Color(30, 30, 40)
    
    # Just one simple button with text
    btn = ui.Button(200, 60, ui.ButtonShape.RoundedRect, 10,
        normal={'color': ui.Color(50, 100, 200)},
        text={'text': "Click Me", 'font_size': 20}
    )
    btn.set_position(100, 120)
    stack.add_layer(btn)
    
    print("Entering loop...")
    running = True
    frame = 0
    while running and frame < 60:  # Only run 60 frames for testing
        while True:
            event = window.poll_event()
            if event is None: break
            if event.type == ui.EventType.Quit: running = False
            btn.process_event(event)
            
        btn.update(window.delta_time)
        final = stack.composite()
        window.present(final)
        frame += 1
    
    print(f"Finished {frame} frames")
    window.close()
    ui.quit()

if __name__ == "__main__":
    import traceback
    try:
        main()
    except Exception as e:
        print(f"Error: {e}")
        traceback.print_exc()
