#include "character_generation.h"
#include <chrono>
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
#include <winhttp.h>
#undef DrawText
#undef CloseWindow
#undef ShowCursor
#undef Rectangle
#undef LoadImage

namespace {
std::wstring Wide(const std::string& s) {
    const int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (!n && !s.empty()) throw std::runtime_error("Invalid URL encoding");
    std::wstring w(n, L' ');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), static_cast<int>(s.size()), w.data(), n);
    return w;
}
struct Handle {
    HINTERNET value = nullptr;
    ~Handle() { if (value) WinHttpCloseHandle(value); }
    operator HINTERNET() const { return value; }
};
void Check(BOOL ok) {
    if (!ok) throw std::runtime_error("Network/TLS request failed (Windows " + std::to_string(GetLastError()) + ")");
}
}
#endif

CharacterHttpResponse SendCharacterHttp(const CharacterHttpRequest& input, const std::atomic_bool& cancelled) {
    if (cancelled) throw std::runtime_error("Cancelled");
#ifdef _WIN32
    if (!IsCharacterImageUrl(input.url) || input.body.size() > 16 * 1024 * 1024)
        throw std::runtime_error("Invalid HTTPS request");
    auto url = Wide(input.url);
    URL_COMPONENTS parts{}; parts.dwStructSize = sizeof(parts);
    parts.dwHostNameLength = parts.dwUrlPathLength = parts.dwExtraInfoLength = static_cast<DWORD>(-1);
    Check(WinHttpCrackUrl(url.c_str(), static_cast<DWORD>(url.size()), 0, &parts));
    if (parts.nScheme != INTERNET_SCHEME_HTTPS || parts.dwUserNameLength || parts.dwPasswordLength)
        throw std::runtime_error("Only credential-free HTTPS URLs are supported");
    std::wstring host(parts.lpszHostName, parts.dwHostNameLength);
    std::wstring path(parts.lpszUrlPath, parts.dwUrlPathLength);
    if (parts.dwExtraInfoLength) path.append(parts.lpszExtraInfo, parts.dwExtraInfoLength);
    Handle session{WinHttpOpen(L"ArknightsGo-CharacterCreator/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0)};
    Check(session.value != nullptr);
    Check(WinHttpSetTimeouts(session, 10000, 10000, 30000, 180000));
    Handle connection{WinHttpConnect(session, host.c_str(), parts.nPort, 0)};
    Check(connection.value != nullptr);
    const auto method = Wide(input.method);
    Handle request{WinHttpOpenRequest(connection, method.c_str(), path.c_str(), nullptr,
        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE)};
    Check(request.value != nullptr);
    // Do not forward API credentials through redirects, or silently downgrade TLS.
    DWORD policy = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    Check(WinHttpSetOption(request, WINHTTP_OPTION_REDIRECT_POLICY, &policy, sizeof(policy)));
    std::wstring headers = L"Content-Type: application/json\r\n";
    if (!input.bearer.empty()) {
        if (input.bearer.find_first_of("\r\n") != std::string::npos) throw std::runtime_error("Invalid API key");
        headers += L"Authorization: Bearer " + Wide(input.bearer) + L"\r\n";
    }
    Check(WinHttpSendRequest(request, headers.c_str(), static_cast<DWORD>(headers.size()),
        input.body.empty() ? WINHTTP_NO_REQUEST_DATA : const_cast<char*>(input.body.data()),
        static_cast<DWORD>(input.body.size()), static_cast<DWORD>(input.body.size()), 0));
    if (cancelled) throw std::runtime_error("Cancelled");
    Check(WinHttpReceiveResponse(request, nullptr));
    CharacterHttpResponse result;
    DWORD status = 0, size = sizeof(status);
    Check(WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX));
    result.status = static_cast<int>(status);
    char buffer[16384];
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::minutes(3);
    while (!cancelled) {
        if (std::chrono::steady_clock::now() > deadline) throw std::runtime_error("Download timed out");
        DWORD count = 0;
        Check(WinHttpReadData(request, buffer, sizeof(buffer), &count));
        if (!count) return result;
        if (result.body.size() + count > input.maxResponseBytes) throw std::runtime_error("Response exceeds size limit");
        result.body.append(buffer, count);
    }
    throw std::runtime_error("Cancelled");
#else
    (void)input;
    throw std::runtime_error("Character API transport currently requires Windows");
#endif
}
