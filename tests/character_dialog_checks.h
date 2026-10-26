#pragma once
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define DrawText Win32DrawText
#define CloseWindow Win32CloseWindow
#define ShowCursor Win32ShowCursor
#define Rectangle Win32Rectangle
#include <windows.h>
#undef DrawText
#undef CloseWindow
#undef ShowCursor
#undef Rectangle
#undef LoadImage

namespace CharacterDialogChecks {
struct Probe {
    HWND game = nullptr, picker = nullptr;
    bool initiallyForeground = false, foreground = true, active = true, aboveOwner = true, modal = true;
    bool topmost = true, ownerStylePreserved = true, ownerWasTopmost = false;
    bool repainting = true;
    int ticks = 0, samples = 0, frames = 0, previousFrames = 0;
};
inline Probe* current = nullptr;
inline BOOL CALLBACK FindPicker(HWND window, LPARAM data) {
    auto& probe = *reinterpret_cast<Probe*>(data);
    wchar_t name[32]{}; GetClassNameW(window, name, 32);
    if (GetWindow(window, GW_OWNER) == probe.game && wcscmp(name, L"#32770") == 0 && IsWindowVisible(window)) {
        probe.picker = window;
        return FALSE;
    }
    return TRUE;
}
inline void CALLBACK InspectAndCancel(HWND, UINT, UINT_PTR timer, DWORD) {
    auto& probe = *current;
    EnumThreadWindows(GetCurrentThreadId(), FindPicker, reinterpret_cast<LPARAM>(&probe));
    if (probe.picker) {
        probe.foreground &= GetForegroundWindow() == probe.picker;
        probe.active &= GetActiveWindow() == probe.picker;
        bool aboveOwner = false;
        for (HWND above = GetWindow(probe.game, GW_HWNDPREV); above; above = GetWindow(above, GW_HWNDPREV))
            if (above == probe.picker) { aboveOwner = true; break; }
        probe.aboveOwner &= aboveOwner;
        probe.topmost &= (GetWindowLongPtrW(probe.picker, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
        probe.ownerStylePreserved &= ((GetWindowLongPtrW(probe.game, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0)
                                    == probe.ownerWasTopmost;
        probe.modal &= !IsWindowEnabled(probe.game);
        probe.repainting &= probe.frames > probe.previousFrames;
        probe.previousFrames = probe.frames;
        // Sample over a full second: an initial activation may be undone when
        // Windows finishes showing the common dialog.
        if (++probe.samples >= 4) {
            KillTimer(nullptr, timer);
            PostMessageW(probe.picker, WM_COMMAND, IDCANCEL, 0);
        }
    } else if (++probe.ticks >= 20) {
        KillTimer(nullptr, timer);
        const HWND popup = GetLastActivePopup(probe.game);
        if (popup != probe.game) PostMessageW(popup, WM_CLOSE, 0, 0);
    }
}
inline void RunOnce(const std::function<bool(const std::function<void()>&)>& open,
                    const std::function<void()>& redraw) {
    Probe probe; probe.game = static_cast<HWND>(GetWindowHandle()); current = &probe;
    probe.ownerWasTopmost = (GetWindowLongPtrW(probe.game, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
    ShowWindow(probe.game, SW_SHOW);
    SetForegroundWindow(probe.game);
    // Windows can deny foreground access to a test launched by a background shell.
    // In that case validate our own thread's activation and window ordering; when
    // launched from the foreground also require global foreground restoration.
    probe.initiallyForeground = GetForegroundWindow() == probe.game;
    const UINT_PTR timer = SetTimer(nullptr, 0, 250, InspectAndCancel);
    if (!timer) throw std::runtime_error("Cannot create file picker probe timer");
    bool cancelled = false;
    try {
        cancelled = open([&] { redraw(); ++probe.frames; });
    } catch (...) {
        KillTimer(nullptr, timer); current = nullptr; throw;
    }
    KillTimer(nullptr, timer); current = nullptr;
    const bool restored = IsWindowEnabled(probe.game) && GetActiveWindow() == probe.game &&
        (!probe.initiallyForeground || GetForegroundWindow() == probe.game) &&
        (((GetWindowLongPtrW(probe.game, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0) == probe.ownerWasTopmost);
    if (!probe.picker || probe.samples < 4 || !probe.active || !probe.aboveOwner ||
        !probe.topmost || !probe.ownerStylePreserved || !probe.repainting || probe.frames < 4 ||
        (probe.initiallyForeground && !probe.foreground) || !probe.modal || !restored || !cancelled)
        throw std::runtime_error("File picker check failed: found=" + std::to_string(probe.picker != nullptr) +
            " topmost=" + std::to_string(probe.topmost) + " samples=" + std::to_string(probe.samples) +
            " active=" + std::to_string(probe.active) + " aboveOwner=" + std::to_string(probe.aboveOwner) +
            " foreground=" + std::to_string(probe.foreground) + " modal=" + std::to_string(probe.modal) +
            " frames=" + std::to_string(probe.frames) + " repainting=" + std::to_string(probe.repainting) +
            " restored=" + std::to_string(restored) + " cancelled=" + std::to_string(cancelled));
}
inline void Run(const std::function<bool(const std::function<void()>&)>& open,
                const std::function<void()>& redraw) {
    redraw();
    RunOnce(open, redraw);
    // Also cover a game explicitly configured to stay above ordinary windows.
    SetWindowState(FLAG_WINDOW_TOPMOST);
    RunOnce(open, redraw);
    ClearWindowState(FLAG_WINDOW_TOPMOST);
    RunOnce(open, redraw); // Reopening after cancellation must still work.
}
}
#endif
