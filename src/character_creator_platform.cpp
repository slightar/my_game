#include "character_creator.h"
#include <exception>
#include <stdexcept>
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
#include <commdlg.h>
#include <imm.h>
#undef DrawText
#undef CloseWindow
#undef ShowCursor
#undef Rectangle
#undef LoadImage

namespace {
constexpr UINT ActivatePicker = WM_APP + 1;
constexpr UINT_PTR RepaintTimer = 1;
struct PickerContext {
    const std::function<void()>& redraw;
    std::exception_ptr failure;
    bool drawing = false;
};
void RaiseReferenceDialog(HWND child) {
    const HWND picker = GetParent(child);
    // A borderless game can occupy the topmost band. HWND_TOP only reorders
    // within the normal band, so it cannot reliably expose the file picker.
    // Only the short-lived picker is topmost; the game's style is unchanged.
    SetWindowPos(picker, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    SetForegroundWindow(picker);
    SetActiveWindow(picker);
}
UINT_PTR CALLBACK ActivateReferenceDialog(HWND child, UINT message, WPARAM wparam, LPARAM data) {
    if (message == WM_INITDIALOG) {
        const auto* dialog = reinterpret_cast<OPENFILENAMEW*>(data);
        SetWindowLongPtrW(child, GWLP_USERDATA, dialog->lCustData);
    }
    auto* context = reinterpret_cast<PickerContext*>(GetWindowLongPtrW(child, GWLP_USERDATA));
    if (message == WM_NOTIFY && reinterpret_cast<NMHDR*>(data)->code == CDN_INITDONE) {
        RaiseReferenceDialog(child);
        // The common dialog still performs its initial show/activation after
        // CDN_INITDONE. Run once more in its message loop, after that work.
        PostMessageW(child, ActivatePicker, 0, 0);
        // GetOpenFileName runs its own message loop on the render thread.
        // Keep presenting the current page while that loop owns the thread.
        if (context && context->redraw && !SetTimer(child, RepaintTimer, 33, nullptr)) {
            context->failure = std::make_exception_ptr(std::runtime_error("无法刷新选图背景，请重试"));
            PostMessageW(GetParent(child), WM_CLOSE, 0, 0);
        }
    } else if (message == ActivatePicker) {
        RaiseReferenceDialog(child);
    } else if (message == WM_TIMER && wparam == RepaintTimer && context &&
               !context->drawing && !context->failure) {
        // EndDrawing pumps window events; guard against nested timer delivery.
        context->drawing = true;
        try { context->redraw(); }
        catch (...) {
            context->failure = std::current_exception();
            PostMessageW(GetParent(child), WM_CLOSE, 0, 0);
        }
        context->drawing = false;
    } else if (message == WM_DESTROY) {
        KillTimer(child, RepaintTimer);
        SetWindowLongPtrW(child, GWLP_USERDATA, 0);
    }
    return 0;
}
}
#endif

void SetCharacterTextInput(bool enabled) {
#ifdef _WIN32
    static bool wasEnabled = false;
    if (enabled == wasEnabled) return;
    wasEnabled = enabled;
    const auto window = static_cast<HWND>(GetWindowHandle());
    if (enabled) ImmAssociateContextEx(window, nullptr, IACE_DEFAULT);
    else ImmAssociateContext(window, nullptr);
#else
    (void)enabled;
#endif
}
std::filesystem::path ChooseCharacterReference(const std::function<void()>& redraw) {
#ifdef _WIN32
    PickerContext context{redraw, {}, false};
    // Commit the workshop before the native dialog takes over message dispatch.
    if (redraw) redraw();
    wchar_t file[32768]{};
    OPENFILENAMEW dialog{}; dialog.lStructSize = sizeof(dialog);
    const HWND gameWindow = GetAncestor(static_cast<HWND>(GetWindowHandle()), GA_ROOT);
    dialog.hwndOwner = gameWindow;
    dialog.lpstrTitle = L"选择角色参考图片";
    dialog.lpstrFilter = L"Character images (*.png;*.jpg;*.jpeg;*.bmp)\0*.png;*.jpg;*.jpeg;*.bmp\0\0";
    dialog.lpstrFile = file; dialog.nMaxFile = 32768;
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR |
        OFN_EXPLORER | OFN_ENABLEHOOK | OFN_ENABLESIZING;
    dialog.lpfnHook = ActivateReferenceDialog;
    dialog.lCustData = reinterpret_cast<LPARAM>(&context);
    SetForegroundWindow(gameWindow);
    const BOOL selected = GetOpenFileNameW(&dialog);
    SetForegroundWindow(gameWindow);
    SetFocus(gameWindow);
    if (context.failure) std::rethrow_exception(context.failure);
    if (selected) return std::filesystem::path(file);
#else
    (void)redraw;
#endif
    return {};
}
