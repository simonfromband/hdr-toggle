#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>

#pragma comment(lib, "Kernel32.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Gdi32.lib")

const std::wstring TARGET_PROCESS = L"AsusMyASUS.exe";
const int POLL_INTERVAL_MS = 100;
const UINT WM_TRAY = WM_USER + 1;
const UINT TRAY_MENU_CLOSE = 1001;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
bool g_Exit = false;

bool isProcessRunning(std::vector<DWORD>& newPIDs) {
    bool found = false;
    newPIDs.clear();
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32 pe = {};
    pe.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(hSnap, &pe)) {
        do {
            std::wstring exeName = pe.szExeFile;
            if (exeName.find(TARGET_PROCESS) != std::wstring::npos) {
                found = true;
                newPIDs.push_back(pe.th32ProcessID);
            }
        } while (Process32Next(hSnap, &pe));
    }
    CloseHandle(hSnap);
    return found;
}

void KillMyASUS(const std::vector<DWORD>& pids) {
    for (DWORD pid : pids) {
        HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        if (hProc) {
            TerminateProcess(hProc, 0);
            CloseHandle(hProc);
        }
    }
}

void ToggleHDRHotkey() {
    INPUT input[6] = {};
    input[0].type = INPUT_KEYBOARD; input[0].ki.wVk = VK_LWIN;
    input[1].type = INPUT_KEYBOARD; input[1].ki.wVk = VK_MENU;
    input[2].type = INPUT_KEYBOARD; input[2].ki.wVk = 'B';
    input[3].type = INPUT_KEYBOARD; input[3].ki.wVk = 'B'; input[3].ki.dwFlags = KEYEVENTF_KEYUP;
    input[4].type = INPUT_KEYBOARD; input[4].ki.wVk = VK_MENU; input[4].ki.dwFlags = KEYEVENTF_KEYUP;
    input[5].type = INPUT_KEYBOARD; input[5].ki.wVk = VK_LWIN; input[5].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(6, input, sizeof(INPUT));
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"MyASUS_HDR_Toggle_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) return 0;

    std::vector<DWORD> triggeredPIDs;
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MyASUSHDRToggleClass";
    RegisterClass(&wc);
    HWND hwnd = CreateWindowEx(0, wc.lpszClassName, L"MyASUS HDR Toggle", 0,
        0, 0, 0, 0, HWND_MESSAGE, nullptr, hInstance, nullptr);
    NOTIFYICONDATA nid = {};
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAY;
    nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wcscpy_s(nid.szTip, L"MyASUS HDR Toggle");
    Shell_NotifyIcon(NIM_ADD, &nid);

    while (!g_Exit) {
        std::vector<DWORD> currentPIDs;
        if (isProcessRunning(currentPIDs)) {
            for (DWORD pid : currentPIDs) {
                if (std::find(triggeredPIDs.begin(), triggeredPIDs.end(), pid) == triggeredPIDs.end()) {
                    KillMyASUS(currentPIDs);
                    ToggleHDRHotkey();
                    triggeredPIDs.push_back(pid);
                }
            }
        }
        triggeredPIDs.erase(
            std::remove_if(triggeredPIDs.begin(), triggeredPIDs.end(),
                [](DWORD pid) {
                    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
                    if (!h) return true;
                    CloseHandle(h);
                    return false;
                }),
            triggeredPIDs.end()
        );
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(POLL_INTERVAL_MS));
    }

    Shell_NotifyIcon(NIM_DELETE, &nid);
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_TRAY && lParam == WM_RBUTTONUP) {
        HMENU hMenu = CreatePopupMenu();
        AppendMenu(hMenu, MF_STRING, TRAY_MENU_CLOSE, L"Close MyASUS HDR Toggle");
        POINT pt;
        GetCursorPos(&pt);
        SetForegroundWindow(hwnd);
        TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
        DestroyMenu(hMenu);
    }
    else if (uMsg == WM_COMMAND) {
        if (LOWORD(wParam) == TRAY_MENU_CLOSE) g_Exit = true;
    }
    else if (uMsg == WM_DESTROY) PostQuitMessage(0);
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
