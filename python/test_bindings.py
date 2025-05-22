import threading
import time
import pyfheroes2

# Example function to run the game. This call blocks until the game exits.
def run_game():
    # Replace command line arguments as needed
    pyfheroes2.start_game(["fheroes2"])

# Start the game in a separate thread so we can interact with it
thread = threading.Thread(target=run_game, daemon=True)
thread.start()

# Give the game some time to initialise
time.sleep(3)

# Send a keyboard event (press and release the Right arrow key)
pyfheroes2.send_key(pyfheroes2.KEY_RIGHT, 1)
time.sleep(0.1)
pyfheroes2.send_key(pyfheroes2.KEY_RIGHT, 0)

# Capture the current screen buffer
image, width, height = pyfheroes2.capture()
print(f"Captured frame {width}x{height}, {len(image)} bytes")

# Quit the game by sending an ESC key event
pyfheroes2.send_key(27, 1)
pyfheroes2.send_key(27, 0)

thread.join()
