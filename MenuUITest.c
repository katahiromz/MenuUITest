#include <windows.h>
#include <stdio.h>
#include <math.h>

#define SUB_PROGRAM L"MenuUITestSub.exe"
#define CLASSNAME L"MenuUITest"
#define WC_MENU L"#32768"

#define DELAY 100
#define INTERVAL 200
static HANDLE s_hThread = NULL;

typedef enum tagAUTO_CLICK
{
    AUTO_LEFT_CLICK,
    AUTO_RIGHT_CLICK,
    AUTO_LEFT_DOUBLE_CLICK,
    AUTO_RIGHT_DOUBLE_CLICK,
} AUTO_CLICK;

typedef enum tagAUTO_KEY
{
    AUTO_KEY_DOWN,
    AUTO_KEY_UP,
    AUTO_KEY_DOWN_UP,
} AUTO_KEY;

#define ASSERT(x) \
    ((x) ? TRUE : printf("Assertion failed! Line:%d, Expression:%s\n", __LINE__, #x))

static VOID
AutoKey(AUTO_KEY type, UINT vKey)
{
    if (type == AUTO_KEY_DOWN_UP)
    {
        AutoKey(AUTO_KEY_DOWN, vKey);
        AutoKey(AUTO_KEY_UP, vKey);
        return;
    }

    INPUT input;
    ZeroMemory(&input, sizeof(input));

    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vKey;
    input.ki.dwFlags = ((type == AUTO_KEY_UP) ? KEYEVENTF_KEYUP : 0);
    SendInput(1, &input, sizeof(INPUT));
    Sleep(DELAY);
}

static VOID
AutoClick(AUTO_CLICK type, INT x, INT y)
{
    INPUT input;
    ZeroMemory(&input, sizeof(input));

    INT nScreenWidth = GetSystemMetrics(SM_CXSCREEN) - 1;
    INT nScreenHeight = GetSystemMetrics(SM_CYSCREEN) - 1;

    input.type = INPUT_MOUSE;
    input.mi.dx = (LONG)round(x * (65535.0f / nScreenWidth));
    input.mi.dy = (LONG)round(y * (65535.0f / nScreenHeight));
    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    SendInput(1, &input, sizeof(INPUT));
    Sleep(DELAY);

    input.mi.dx = input.mi.dy = 0;

    INT i, count = 1;
    switch (type)
    {
        case AUTO_LEFT_DOUBLE_CLICK:
            count = 2;
            // FALL THROUGH
        case AUTO_LEFT_CLICK:
        {
            for (i = 0; i < count; ++i)
            {
                input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
                SendInput(1, &input, sizeof(INPUT));
                Sleep(DELAY);

                input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
                SendInput(1, &input, sizeof(INPUT));
                Sleep(DELAY);
            }
            break;
        }
        case AUTO_RIGHT_DOUBLE_CLICK:
            count = 2;
            // FALL THROUGH
        case AUTO_RIGHT_CLICK:
        {
            for (i = 0; i < count; ++i)
            {
                input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
                SendInput(1, &input, sizeof(INPUT));
                Sleep(DELAY);

                input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
                SendInput(1, &input, sizeof(INPUT));
                Sleep(DELAY);
            }
            break;
        }
    }
}

static POINT
CenterPoint(const RECT *prc)
{
    POINT pt = { (prc->left + prc->right) / 2, (prc->top + prc->bottom) / 2 };
    return pt;
}

static INT
GetHitID(HWND hwndTarget)
{
    return HandleToUlong(GetPropW(hwndTarget, L"Hit"));
}

typedef struct tagFINDMENUSUB
{
    HWND hwndMenuTarget;
    HWND hwndMenuSub;
} FINDMENUSUB, *PFINDMENU2SUB;

static BOOL CALLBACK
EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    if (!IsWindowVisible(hwnd))
        return TRUE;

    WCHAR szClass[64];
    GetClassNameW(hwnd, szClass, _countof(szClass));
    if (lstrcmpiW(szClass, WC_MENU) != 0)
        return TRUE;

    PFINDMENU2SUB pData = (PFINDMENU2SUB)lParam;
    if (hwnd == pData->hwndMenuTarget)
        return TRUE;

    pData->hwndMenuSub = hwnd;
    return FALSE;
}

static HWND
FindMenuSub(HWND hwndMenuTarget)
{
    FINDMENUSUB data = { NULL, hwndMenuTarget };
    EnumWindows(EnumWindowsProc, (LPARAM)&data);
    return data.hwndMenuSub;
}

static DWORD WINAPI
ThreadFunc(LPVOID arg)
{
    HWND hwnd = FindWindowW(L"MenuUITest", L"MenuUITest");
    ShowWindow(hwnd, SW_HIDE);

    SHELLEXECUTEINFOW sei = { sizeof(sei), SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS };

    sei.lpFile = SUB_PROGRAM;
    sei.nShow = SW_SHOWNORMAL;

    sei.lpParameters = L"#1";
    if (!ShellExecuteExW(&sei))
        ASSERT(0);
    WaitForInputIdle(sei.hProcess, INFINITE);
    CloseHandle(sei.hProcess);

    sei.lpParameters = L"#2";
    if (!ShellExecuteExW(&sei))
        ASSERT(0);
    WaitForInputIdle(sei.hProcess, INFINITE);
    CloseHandle(sei.hProcess);

    Sleep(INTERVAL);
    HWND hwnd1 = FindWindowW(L"MenuUITestSub", L"#1");
    HWND hwnd2 = FindWindowW(L"MenuUITestSub", L"#2");
    ASSERT(hwnd);
    ASSERT(hwnd1);
    ASSERT(hwnd2);
    ASSERT(hwnd1 != hwnd2);

    RECT rcWork;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &rcWork, 0);
    INT cxWork = (rcWork.right - rcWork.left);
    INT cyWork = (rcWork.bottom - rcWork.top);

    RECT rc1 = { rcWork.left, rcWork.top, rcWork.left + cxWork / 2, rcWork.bottom };
    RECT rc2 = { rcWork.left + cxWork / 2, rcWork.top, rcWork.right, rcWork.bottom };
    MoveWindow(hwnd1, rc1.left, rc1.top, rc1.right - rc1.left, rc1.bottom - rc1.top, TRUE);
    MoveWindow(hwnd2, rc2.left, rc2.top, rc2.right - rc2.left, rc2.bottom - rc2.top, TRUE);

    POINT pt1 = CenterPoint(&rc1);
    POINT pt2 = CenterPoint(&rc2);

    AutoClick(AUTO_RIGHT_CLICK, pt1.x, pt1.y);
    Sleep(INTERVAL);

    HWND hwndMenu1 = FindWindowW(WC_MENU, L"");
    ASSERT(IsWindowVisible(hwndMenu1));

    AutoClick(AUTO_RIGHT_CLICK, pt2.x, pt2.y);

    HWND hwndMenu2 = FindWindowW(WC_MENU, L"");
    ASSERT(IsWindowVisible(hwndMenu2));
    ASSERT(hwndMenu1 != hwndMenu2);

    AutoKey(AUTO_KEY_DOWN_UP, VK_ESCAPE);

    Sleep(INTERVAL);
    ASSERT(GetHitID(hwnd2) == 0);
    HWND hwndMenu0 = FindWindowW(WC_MENU, L"");
    ASSERT(!IsWindowVisible(hwndMenu0));

    AutoClick(AUTO_RIGHT_CLICK, pt1.x, pt1.y);
    hwndMenu1 = FindWindowW(WC_MENU, L"");
    ASSERT(IsWindowVisible(hwndMenu1));

    RECT rcMenu1;
    GetWindowRect(hwndMenu1, &rcMenu1);
    POINT ptMenu1 = CenterPoint(&rcMenu1);

    AutoClick(AUTO_LEFT_CLICK, ptMenu1.x, ptMenu1.y);
    hwndMenu1 = FindWindowW(WC_MENU, L"");
    ASSERT(IsWindowVisible(hwndMenu1));

    AutoClick(AUTO_LEFT_DOUBLE_CLICK, ptMenu1.x, ptMenu1.y);
    hwndMenu1 = FindWindowW(WC_MENU, L"");
    ASSERT(IsWindowVisible(hwndMenu1));

    AutoClick(AUTO_RIGHT_CLICK, ptMenu1.x, ptMenu1.y);
    hwndMenu1 = FindWindowW(WC_MENU, L"");
    ASSERT(IsWindowVisible(hwndMenu1));

    AutoClick(AUTO_RIGHT_DOUBLE_CLICK, ptMenu1.x, ptMenu1.y);
    hwndMenu1 = FindWindowW(WC_MENU, L"");
    ASSERT(IsWindowVisible(hwndMenu1));

    POINT pt1_3 = { ptMenu1.x, (2 * rcMenu1.top + 1 * rcMenu1.bottom) / (1 + 2) };
    AutoClick(AUTO_LEFT_CLICK, pt1_3.x, pt1_3.y);

    Sleep(INTERVAL);
    hwndMenu1 = FindWindowW(WC_MENU, L"");
    ASSERT(!IsWindowVisible(hwndMenu1));
    ASSERT(GetHitID(hwnd1) == 100);

    AutoClick(AUTO_RIGHT_CLICK, pt2.x, pt2.y);
    hwndMenu2 = FindWindowW(WC_MENU, L"");
    ASSERT(IsWindowVisible(hwndMenu2));

    AutoKey(AUTO_KEY_DOWN_UP, VK_DOWN);
    hwndMenu2 = FindWindowW(WC_MENU, L"");
    ASSERT(IsWindowVisible(hwndMenu2));

    AutoKey(AUTO_KEY_DOWN_UP, VK_RETURN);

    Sleep(INTERVAL);
    hwndMenu2 = FindWindowW(WC_MENU, L"");
    ASSERT(!IsWindowVisible(hwndMenu2));
    ASSERT(GetHitID(hwnd2) == 100);

    AutoClick(AUTO_RIGHT_CLICK, pt2.x, pt2.y);
    hwndMenu2 = FindWindowW(WC_MENU, L"");
    ASSERT(IsWindowVisible(hwndMenu2));

    AutoKey(AUTO_KEY_DOWN_UP, VK_UP);
    hwndMenu2 = FindWindowW(WC_MENU, L"");
    ASSERT(IsWindowVisible(hwndMenu2));

    AutoKey(AUTO_KEY_DOWN_UP, VK_RETURN);

    Sleep(INTERVAL);
    hwndMenu2 = FindWindowW(WC_MENU, L"");
    ASSERT(!IsWindowVisible(hwndMenu2));
    ASSERT(GetHitID(hwnd2) == 101);

    AutoKey(AUTO_KEY_DOWN, VK_SHIFT);
    AutoClick(AUTO_RIGHT_CLICK, pt2.x, pt2.y);
    AutoKey(AUTO_KEY_UP, VK_SHIFT);
    hwndMenu2 = FindWindowW(WC_MENU, L"");
    ASSERT(IsWindowVisible(hwndMenu2));

    AutoKey(AUTO_KEY_DOWN_UP, VK_UP);
    hwndMenu2 = FindWindowW(WC_MENU, L"");
    ASSERT(IsWindowVisible(hwndMenu2));

    AutoKey(AUTO_KEY_DOWN_UP, VK_RETURN);
    hwndMenu2 = FindWindowW(WC_MENU, L"");
    ASSERT(IsWindowVisible(hwndMenu2));
    HWND hwndMenu2Sub = FindMenuSub(hwndMenu2);
    ASSERT(IsWindowVisible(hwndMenu2Sub));

    AutoClick(AUTO_RIGHT_CLICK, pt1.x, pt1.y);
    ASSERT(!IsWindowVisible(hwndMenu2Sub));
    AutoKey(AUTO_KEY_DOWN_UP, VK_DOWN);
    AutoKey(AUTO_KEY_DOWN_UP, VK_RETURN);

    Sleep(INTERVAL);
    hwndMenu1 = FindWindowW(WC_MENU, L"");
    ASSERT(!IsWindowVisible(hwndMenu1));

    PostMessageW(hwnd1, WM_CLOSE, 0, 0);
    PostMessageW(hwnd2, WM_CLOSE, 0, 0);
    ShowWindow(hwnd, SW_SHOWNORMAL);

    PostMessage(hwnd, WM_CLOSE, 0, 0);
    return 0;
}

static INT
OnCreate(HWND hwnd)
{
    s_hThread = CreateThread(NULL, 0, ThreadFunc, NULL, 0, NULL);
    return 0;
}

static VOID
OnDestroy(HWND hwnd)
{
    if (s_hThread)
    {
        CloseHandle(s_hThread);
        s_hThread = NULL;
    }

    PostQuitMessage(0);
}

static
LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
            return OnCreate(hwnd);
        case WM_DESTROY:
            OnDestroy(hwnd);
            break;
        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int main(void)
{
    HINSTANCE hInstance = GetModuleHandleW(NULL);

    WNDCLASSW wc = { CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS, WindowProc };
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = CLASSNAME;
    if (!RegisterClassW(&wc))
        return 1;

    HWND hwnd = CreateWindowW(CLASSNAME, L"MenuUITest", WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
                              NULL, NULL, hInstance, NULL);
    if (!hwnd)
        return 1;

    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (INT)msg.wParam;
}
