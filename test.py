import Palladium as ui

class GameState:
    def __init__(self):
        self.board = [["" for _ in range(3)] for _ in range(3)]
        self.current = "X"
        self.winner = None

    def play(self, r, c):
        if self.winner is not None:
            return False
        if self.board[r][c] != "":
            return False
        
        self.board[r][c] = self.current

        if self.check_win(self.current):
            self.winner = self.current
        elif self.check_draw():
            self.winner = "Draw"
        else:
            self.current = "O" if self.current == "X" else "X"

        return True  # Move accepted

    def check_draw(self):
        return all(cell != "" for row in self.board for cell in row)

    def check_win(self, p):
        B = self.board

        # rows / cols
        for i in range(3):
            if B[i][0] == B[i][1] == B[i][2] == p:
                return True
            if B[0][i] == B[1][i] == B[2][i] == p:
                return True

        # diagonals
        if B[0][0] == B[1][1] == B[2][2] == p:
            return True
        if B[0][2] == B[1][1] == B[2][0] == p:
            return True

        return False


def create_cell_button(x, y, r, c, size, game, stack, meta):
    btn = ui.Button(
        size, size,
        ui.ButtonShape.RoundedRect,
        12,
        normal={'color': ui.Color(60, 60, 80)},
        hover={'color': ui.Color(80, 80, 110), 'scale': 1.08},
        pressed={'color': ui.Color(40, 40, 60), 'scale': 0.97},
        text={'text': "", 'font_size': 42, 'position': "center"}
    )

    btn.set_position(x, y)
    stack.add_layer(btn)

    # store (r, c) metadata externally
    meta[btn] = (r, c)

    # define click behavior (Palladium style)
    def handle_click():
        rr, cc = meta[btn]
        if game.play(rr, cc):
            # update button text
            btn.text['text'] = game.board[rr][cc]

    btn.on_click = handle_click

    return btn


def main():
    ui.init()
    window = ui.Window("Tic Tac Toe – Palladium", 500, 600, vsync=True)

    stack = ui.LayerStack(500, 600)
    stack.background = ui.Color(30, 30, 40)

    game = GameState()
    buttons = []
    button_meta = {}

    size = 120
    start_x = 40
    start_y = 120

    # Create TIC TAC TOE grid
    for r in range(3):
        for c in range(3):
            x = start_x + c * (size + 10)
            y = start_y + r * (size + 10)
            btn = create_cell_button(x, y, r, c, size, game, stack, button_meta)
            buttons.append(btn)

    # Reset button
    reset = ui.Button(
        200, 55,
        ui.ButtonShape.RoundedRect,
        12,
        normal={'color': ui.Color(120, 80, 200)},
        hover={'color': ui.Color(140, 100, 220), 'scale': 1.05},
        pressed={'color': ui.Color(90, 60, 170)},
        text={'text': "Reset Game", 'font_size': 22}
    )
    reset.set_position(150, 500)

    def reset_game():
        game.__init__()
        for b in buttons:
            b.text['text'] = ""

    reset.on_click = reset_game
    stack.add_layer(reset)

    # Fake title bar as a visual element
    title = ui.Button(
        300, 60,
        ui.ButtonShape.RoundedRect, 15,
        normal={'color': ui.Color(50, 50, 70)},
        text={'text': "Tic Tac Toe", 'font_size': 27}
    )
    title.set_position(100, 30)
    stack.add_layer(title)

    running = True
    while running:
        while True:
            event = window.poll_event()
            if event is None:
                break

            if event.type == ui.EventType.Quit:
                running = False
            elif event.type == ui.EventType.KeyDown and event.key == 27:
                running = False

            # feed events to buttons
            for b in buttons:
                b.process_event(event)
            reset.process_event(event)

        # update animations
        dt = window.delta_time
        for b in buttons:
            b.update(dt)
        reset.update(dt)

        final = stack.composite()
        window.present(final)

    window.close()
    ui.quit()


if __name__ == "__main__":
    main()
