#include "server_input_capture.h"
#include "constants.h"
#include "encryption_manager.h" // For EncryptionManager class definition
#include "utils.h" // For utility functions
#include <iostream>
#include <string>
#include <vector>
#include <cstring> // For memset

#ifdef _WIN32
    // Windows-specific includes
    #include <winsock2.h>
    #include <windows.h>

    // Windows-specific global variables for input capture
    HHOOK keyboardHook;
    HHOOK mouseHook;
    bool onLocalScreen = true; // Flag for server's local screen control

    // Static variables to hold parameters for hook procedures
    static SOCKET s_udpSocket_win;
    static sockaddr_in s_clientAddr_win;
    static EncryptionManager* s_encryptionManager_win = nullptr;
    static bool* s_isEncrypted_win = nullptr;

    // Function to hide the cursor
    void hideCursor() {
        while (ShowCursor(FALSE) >= 0);
    }

    // Function to show the cursor
    void showCursor() {
        while (ShowCursor(TRUE) < 0);
    }

    LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode == HC_ACTION && onLocalScreen) { // Only capture if controlling local screen
            KBDLLHOOKSTRUCT* pKBDLLHookStruct = (KBDLLHOOKSTRUCT*)lParam;
            std::string message;
            if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                message = Constants::InputMessages::KEY_PRESS + std::to_string(pKBDLLHookStruct->vkCode);
            } else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
                message = Constants::InputMessages::KEY_RELEASE + std::to_string(pKBDLLHookStruct->vkCode);
            }

            if (!message.empty() && s_udpSocket_win != INVALID_SOCKET) {
                Utils::sendSecureMessage(s_udpSocket_win, s_clientAddr_win, message, 
                                       *s_encryptionManager_win, *s_isEncrypted_win);
            }
        }
        return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
    }

    LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode == HC_ACTION) {
            MSLLHOOKSTRUCT* pMSLLHookStruct = (MSLLHOOKSTRUCT*)lParam;
            std::string message;
            POINT p = pMSLLHookStruct->pt; // Use point from hook struct for mouse move

            // Get screen dimensions
            int screen_width = GetSystemMetrics(SM_CXSCREEN);

            if (wParam == WM_MOUSEMOVE) {
                if (onLocalScreen) {
                    if (p.x >= screen_width - Constants::ScreenTransition::DEFAULT_EDGE_SENSITIVITY) {
                        onLocalScreen = false;
                        hideCursor();
                        Utils::logMessage(Constants::LogTypes::INFO, "Server: Exit to remote screen, hiding cursor");
                        message = Constants::InputMessages::SCREEN_EXIT + Constants::ScreenTransition::DIRECTION_RIGHT;
                    } else {
                        message = Constants::InputMessages::MOUSE_MOVE + std::to_string(p.x) + "," + std::to_string(p.y);
                    }
                } else { // Mouse is on client screen, waiting to return
                    if (p.x <= Constants::ScreenTransition::DEFAULT_EDGE_SENSITIVITY) {
                        onLocalScreen = true;
                        showCursor();
                        Utils::logMessage(Constants::LogTypes::INFO, "Server: Return to local screen, showing cursor");
                        message = Constants::InputMessages::SCREEN_ENTER + Constants::ScreenTransition::DIRECTION_LEFT;
                    }
                    // Do not send mouse move events if not onLocalScreen
                }
            } else if (onLocalScreen) { // Only send other mouse events if controlling local screen
                if (wParam == WM_LBUTTONDOWN) {
                    message = Constants::InputMessages::MOUSE_PRESS + "1";
                } else if (wParam == WM_LBUTTONUP) {
                    message = Constants::InputMessages::MOUSE_RELEASE + "1";
                } else if (wParam == WM_RBUTTONDOWN) {
                    message = Constants::InputMessages::MOUSE_PRESS + "2";
                } else if (wParam == WM_RBUTTONUP) {
                    message = Constants::InputMessages::MOUSE_RELEASE + "2";
                } else if (wParam == WM_MBUTTONDOWN) {
                    message = Constants::InputMessages::MOUSE_PRESS + "3";
                } else if (wParam == WM_MBUTTONUP) {
                    message = Constants::InputMessages::MOUSE_RELEASE + "3";
                } else if (wParam == WM_MOUSEWHEEL) {
                    short wheel_delta = GET_WHEEL_DELTA_WPARAM(pMSLLHookStruct->mouseData);
                    if (wheel_delta > 0) {
                        message = Constants::InputMessages::MOUSE_SCROLL + Constants::InputMessages::MOUSE_SCROLL_UP;
                    } else {
                        message = Constants::InputMessages::MOUSE_SCROLL + Constants::InputMessages::MOUSE_SCROLL_DOWN;
                    }
                }
            }

            if (!message.empty() && s_udpSocket_win != INVALID_SOCKET) {
                Utils::sendSecureMessage(s_udpSocket_win, s_clientAddr_win, message, 
                                       *s_encryptionManager_win, *s_isEncrypted_win);
            }
        }
        return CallNextHookEx(mouseHook, nCode, wParam, lParam);
    }

    void capture_input_windows(SOCKET udpSocket, const sockaddr_in& clientAddr, EncryptionManager& encryptionManager, bool& isEncrypted) {
        // Store parameters in static variables for hook procedures to access
        s_udpSocket_win = udpSocket;
        s_clientAddr_win = clientAddr;
        s_encryptionManager_win = &encryptionManager;
        s_isEncrypted_win = &isEncrypted;

        keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
        mouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, GetModuleHandle(NULL), 0);

        if (!keyboardHook || !mouseHook) {
            std::cerr << "Failed to set hooks." << std::endl;
            return;
        }

        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        UnhookWindowsHookEx(keyboardHook);
        UnhookWindowsHookEx(mouseHook);
    }

#else // Linux X11 specific
    // Linux-specific includes
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>
    #include <X11/extensions/XTest.h>
    #include <X11/cursorfont.h> // For XC_left_ptr

    bool onLocalScreen = true; // Flag for server's local screen control

    void capture_input_linux(int udp_sock, const sockaddr_in& client_udp_addr, EncryptionManager& encryptionManager, bool& isEncrypted) {
        Display *display = XOpenDisplay(nullptr);
        if (!display) {
            std::cerr << "Error: Could not open X display." << std::endl;
            return;
        }

        Window root = DefaultRootWindow(display);
        int screen_num = DefaultScreen(display);
        int screen_width = XDisplayWidth(display, screen_num);
        // int screen_height = XDisplayHeight(display, screen_num); // Unused

        XGrabKeyboard(display, root, True, GrabModeAsync, GrabModeAsync, CurrentTime);
        XGrabPointer(display, root, False, PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
                     GrabModeAsync, GrabModeAsync, None, None, CurrentTime);

        XEvent event;
        while (true) {
            XNextEvent(display, &event);
            std::string message;

            if (event.type == MotionNotify) {
                int x = event.xmotion.x;
                // int y = event.xmotion.y; // Unused

                if (onLocalScreen) {
                    if (x >= screen_width - Constants::ScreenTransition::DEFAULT_EDGE_SENSITIVITY) {
                        message = Constants::InputMessages::SCREEN_EXIT + Constants::ScreenTransition::DIRECTION_RIGHT;
                        onLocalScreen = false;
                        // Hide local cursor
                        XDefineCursor(display, root, XCreateFontCursor(display, 0)); // Invisible cursor
                        XFlush(display);
                        Utils::logMessage(Constants::LogTypes::INFO, "Server: Exit to remote screen, hiding cursor");
                    } else {
                        message = Constants::InputMessages::MOUSE_MOVE + std::to_string(x) + "," + std::to_string(event.xmotion.y);
                    }
                } else { // Mouse is on client screen, waiting to return
                    if (x <= Constants::ScreenTransition::DEFAULT_EDGE_SENSITIVITY) {
                        onLocalScreen = true;
                        message = Constants::InputMessages::SCREEN_ENTER + Constants::ScreenTransition::DIRECTION_LEFT;
                        // Show local cursor
                        XDefineCursor(display, root, XCreateFontCursor(display, XC_left_ptr)); // Default cursor
                        XFlush(display);
                        Utils::logMessage(Constants::LogTypes::INFO, "Server: Return to local screen, showing cursor");
                    }
                    // Do not send mouse move events if not onLocalScreen
                }
            } else if (event.type == KeyPress && onLocalScreen) {
                KeySym key_sym = XLookupKeysym(&event.xkey, 0);
                const char* key_name = XKeysymToString(key_sym);
                if (key_name != nullptr) {
                    message = Constants::InputMessages::KEY_PRESS + std::string(key_name);
                }
            } else if (event.type == KeyRelease && onLocalScreen) {
                KeySym key_sym = XLookupKeysym(&event.xkey, 0);
                const char* key_name = XKeysymToString(key_sym);
                if (key_name != nullptr) {
                    message = Constants::InputMessages::KEY_RELEASE + std::string(key_name);
                }
            } else if (event.type == ButtonPress && onLocalScreen) {
                message = Constants::InputMessages::MOUSE_PRESS + std::to_string(event.xbutton.button);
            } else if (event.type == ButtonRelease && onLocalScreen) {
                message = Constants::InputMessages::MOUSE_RELEASE + std::to_string(event.xbutton.button);
            }

            if (!message.empty()) {
                Utils::sendSecureMessage(udp_sock, client_udp_addr, message, 
                                       encryptionManager, isEncrypted);
            }
        }

        XUngrabKeyboard(display, CurrentTime);
        XUngrabPointer(display, CurrentTime);
        XCloseDisplay(display);
    }
#endif
