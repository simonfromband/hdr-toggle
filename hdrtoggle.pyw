import ctypes
import time

# Define key codes
VK_LWIN = 0x5B
VK_MENU = 0x12  # Alt
VK_B = 0x42

# Key event constants
KEYEVENTF_KEYUP = 0x0002

# Press a key
def press_key(vk):
    ctypes.windll.user32.keybd_event(vk, 0, 0, 0)

# Release a key
def release_key(vk):
    ctypes.windll.user32.keybd_event(vk, 0, KEYEVENTF_KEYUP, 0)

# Optional tiny delay to make sure Windows registers it
time.sleep(0.05)

# Press Win + Alt + B
press_key(VK_LWIN)
press_key(VK_MENU)
press_key(VK_B)

# Release in reverse order
release_key(VK_B)
release_key(VK_MENU)
release_key(VK_LWIN)
