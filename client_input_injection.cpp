#include "client_input_injection.h"
#include "constants.h"
#include "encryption_manager.h" // For EncryptionManager class definition
#include "utils.h" // For utility functions
#include <iostream>
#include <string>
#include <vector>
#include <cstring> // For memset
#include <cstdlib> // For std::stoi

#ifdef _WIN32
    // Windows-specific includes
    #include <winsock2.h>
    #include <windows.h>

    bool controlling_local_screen = true; // Flag for client's local screen control

    void inject_input_windows(const std::string& message, EncryptionManager& encryptionManager, bool& isEncrypted) {
        INPUT input;
        memset(&input, 0, sizeof(INPUT));

        if (Utils::startsWith(message, Constants::InputMessages::SCREEN_EXIT)) {
            controlling_local_screen = false; // Server exited, client takes control
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            Utils::logMessage(Constants::LogTypes::INFO, "Client: Taking control, showing cursor");
        } else if (Utils::startsWith(message, Constants::InputMessages::SCREEN_ENTER)) {
            controlling_local_screen = true; // Server returned, client releases control
            SetCursor(NULL);
            Utils::logMessage(Constants::LogTypes::INFO, "Client: Releasing control, hiding cursor");
        } else if (controlling_local_screen) {
            if (message.rfind(Constants::InputMessages::KEY_PRESS, 0) == 0) {
                input.type = INPUT_KEYBOARD;
                input.ki.wVk = std::stoi(message.substr(Constants::InputMessages::KEY_PRESS.length()));
                input.ki.dwFlags = 0;
                SendInput(1, &input, sizeof(INPUT));
            } else if (message.rfind(Constants::InputMessages::KEY_RELEASE, 0) == 0) {
                input.type = INPUT_KEYBOARD;
                input.ki.wVk = std::stoi(message.substr(Constants::InputMessages::KEY_RELEASE.length()));
                input.ki.dwFlags = KEYEVENTF_KEYUP;
                SendInput(1, &input, sizeof(INPUT));
            } else if (message.rfind(Constants::InputMessages::MOUSE_MOVE, 0) == 0) {
                std::string coords_str = message.substr(Constants::InputMessages::MOUSE_MOVE.length());
                size_t comma_pos = coords_str.find(',');
                if (comma_pos != std::string::npos) {
                    int x = std::stoi(coords_str.substr(0, comma_pos));
                    int y = std::stoi(coords_str.substr(comma_pos + 1));
                    SetCursorPos(x, y);
                }
            } else if (message.rfind(Constants::InputMessages::MOUSE_PRESS, 0) == 0) {
                int button = std::stoi(message.substr(Constants::InputMessages::MOUSE_PRESS.length()));
                input.type = INPUT_MOUSE;
                if (button == 1) input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
                else if (button == 2) input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
                else if (button == 3) input.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
                SendInput(1, &input, sizeof(INPUT));
            } else if (message.rfind(Constants::InputMessages::MOUSE_RELEASE, 0) == 0) {
                int button = std::stoi(message.substr(Constants::InputMessages::MOUSE_RELEASE.length()));
                input.type = INPUT_MOUSE;
                if (button == 1) input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
                else if (button == 2) input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
                else if (button == 3) input.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
                SendInput(1, &input, sizeof(INPUT));
            } else if (message.rfind(Constants::InputMessages::MOUSE_SCROLL, 0) == 0) {
                std::string scroll_dir = message.substr(Constants::InputMessages::MOUSE_SCROLL.length());
                input.type = INPUT_MOUSE;
                input.mi.dwFlags = MOUSEEVENTF_WHEEL;
                input.mi.mouseData = (scroll_dir == Constants::InputMessages::MOUSE_SCROLL_UP) ? WHEEL_DELTA : -WHEEL_DELTA;
                SendInput(1, &input, sizeof(INPUT));
            }
        }
    }

#else // Linux X11 specific
    // Linux-specific includes
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>
    #include <X11/extensions/XTest.h>
    #include <X11/cursorfont.h>

    bool controlling_local_screen = true; // Flag for client's local screen control

    void inject_input_linux(const std::string& message, EncryptionManager& encryptionManager, bool& isEncrypted) {
        Display *display = XOpenDisplay(nullptr);
        if (!display) {
            std::cerr << "Error: Could not open X display." << std::endl;
            return;
        }

        Window root = DefaultRootWindow(display);

        if (Utils::startsWith(message, Constants::InputMessages::SCREEN_EXIT)) {
            controlling_local_screen = false; // Server exited, client takes control
            XDefineCursor(display, root, XCreateFontCursor(display, XC_left_ptr));
            XFlush(display);
            Utils::logMessage(Constants::LogTypes::INFO, "Client: Taking control, showing cursor");
        } else if (Utils::startsWith(message, Constants::InputMessages::SCREEN_ENTER)) {
            controlling_local_screen = true; // Server returned, client releases control
            XDefineCursor(display, root, XCreateFontCursor(display, 0));
            XFlush(display);
            Utils::logMessage(Constants::LogTypes::INFO, "Client: Releasing control, hiding cursor");
        } else if (controlling_local_screen) {
            if (message.rfind(Constants::InputMessages::KEY_PRESS, 0) == 0) {
                std::string key_str = message.substr(Constants::InputMessages::KEY_PRESS.length());
                KeySym key_sym = XStringToKeysym(key_str.c_str());
                if (key_sym != NoSymbol) {
                    KeyCode key_code = XKeysymToKeycode(display, key_sym);
                    XTestFakeKeyEvent(display, key_code, True, CurrentTime);
                    XFlush(display);
                }
            } else if (message.rfind(Constants::InputMessages::KEY_RELEASE, 0) == 0) {
                std::string key_str = message.substr(Constants::InputMessages::KEY_RELEASE.length());
                KeySym key_sym = XStringToKeysym(key_str.c_str());
                if (key_sym != NoSymbol) {
                    KeyCode key_code = XKeysymToKeycode(display, key_sym);
                    XTestFakeKeyEvent(display, key_code, False, CurrentTime);
                    XFlush(display);
                }
            } else if (message.rfind(Constants::InputMessages::MOUSE_MOVE, 0) == 0) {
                std::string coords_str = message.substr(Constants::InputMessages::MOUSE_MOVE.length());
                size_t comma_pos = coords_str.find(',');
                if (comma_pos != std::string::npos) {
                    int x = std::stoi(coords_str.substr(0, comma_pos));
                    int y = std::stoi(coords_str.substr(comma_pos + 1));
                    XTestFakeMotionEvent(display, -1, x, y, CurrentTime);
                    XFlush(display);
                }
            } else if (message.rfind(Constants::InputMessages::MOUSE_PRESS, 0) == 0) {
                int button = std::stoi(message.substr(Constants::InputMessages::MOUSE_PRESS.length()));
                XTestFakeButtonEvent(display, button, True, CurrentTime);
                XFlush(display);
            } else if (message.rfind(Constants::InputMessages::MOUSE_RELEASE, 0) == 0) {
                int button = std::stoi(message.substr(Constants::InputMessages::MOUSE_RELEASE.length()));
                XTestFakeButtonEvent(display, button, False, CurrentTime);
                XFlush(display);
            }
        }

        XCloseDisplay(display);
    }
#endif
