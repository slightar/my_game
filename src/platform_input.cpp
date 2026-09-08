#include "platform_input.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <imm.h>
#endif

void PrepareGameWindowInput(void* nativeWindowHandle) {
#ifdef _WIN32
    const HWND window = static_cast<HWND>(nativeWindowHandle);
    if (window == nullptr) {
        return;
    }

    // The game never accepts text. Detaching the IME keeps letter-key controls
    // independent of the user's Chinese/English input-method state.
    ImmAssociateContext(window, nullptr);
    SetForegroundWindow(window);
    SetFocus(window);
#else
    (void)nativeWindowHandle;
#endif
}
