# Python bindings for fheroes2

This module exposes a minimal interface for running and interacting with the game engine from Python code. The interface is useful for prototyping reinforcement learning experiments.

## Building

The bindings are built with CMake along with the rest of the project:

```bash
cmake -S . -B build -DENABLE_IMAGE=ON
cmake --build build --target pyfheroes2
```

This will create a `pyfheroes2` Python extension inside the build directory.

## Usage example

```python
import pyfheroes2

# start the game (blocking call)
pyfheroes2.start_game(["fheroes2"])
```

Additional helper functions allow sending input events and capturing the current screen buffer:

```python
pyfheroes2.send_key( pyfheroes2.KEY_LEFT, 1 )        # press key
pyfheroes2.send_key( pyfheroes2.KEY_LEFT, 0 )        # release key
pyfheroes2.move_mouse(100, 100)
pyfheroes2.mouse_button(1, 1, 100, 100)              # left button down
pyfheroes2.mouse_button(1, 0, 100, 100)              # left button up
image, width, height = pyfheroes2.capture()
```

Captured image data can then be converted to a NumPy array for further processing.

## Testing the bindings

For a quick manual test you can run the `test_bindings.py` script located in
the same directory:

```bash
python3 test_bindings.py
```

The script launches the game in a background thread, sends a right-arrow key
press, captures a single frame and then quits the game.
