
class UIObject:
    pass

class Button(UIObject):
    def __init__(self, width: int, height: int, shape: 'ButtonShape', corner_radius: int,
                 normal: dict, hover: dict, pressed: dict, text: dict) -> None: ...
    
    def set_position(self, x: int, y: int) -> None: ...