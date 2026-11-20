#define WIN32_LEAN_AND_MEAN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#include <windows.h>
#include <wchar.h>

#define REG_PATH L"SYSTEM\\CurrentControlSet\\Services\\NVDisplay.ContainerLocalSystem\\Parameters\\LocalSystem\\_nvtopps"
#define T(de, en) (g_de ? (de) : (en))
#define P(v) MulDiv((v), g_scale, 100)

enum { ST_MISSING, ST_ENABLED, ST_DISABLED, ST_ERROR };

static HINSTANCE g_inst;
static HWND g_hwnd;
static BOOL g_de, g_pending;
static int g_state = ST_MISSING, g_hover = -1, g_pressed = -1, g_focus = -1, g_scale = 100;
static DWORD g_value = 1;
static LONG g_err;
static RECT g_btnOn, g_btnRestart;
static HFONT g_fTitle, g_fBody, g_fSmall;

static const COLORREF C_BG = 0x1F1612;      /* #12161f */
static const COLORREF C_CARD = 0x30211B;    /* #1b2130 */
static const COLORREF C_LINE = 0x48352C;    /* #2c3548 */
static const COLORREF C_TEXT = 0xF7EEE8;    /* #e8eef7 */
static const COLORREF C_MUTED = 0xB4A094;   /* #94a0b4 */
static const COLORREF C_DIM = 0x907A6B;     /* #6b7a90 */
static const COLORREF C_BLUE = 0xFFA34D;    /* #4da3ff */
static const COLORREF C_GREEN = 0x80DE4A;   /* #4ade80 */
static const COLORREF C_RED = 0x6B6BFF;     /* #ff6b6b */
static const COLORREF C_AMBER = 0x57C8FF;   /* #ffc857 */

static RECT mkrect(int l, int t, int r, int b)
{
    RECT q;
    q.left = l; q.top = t; q.right = r; q.bottom = b;
    return q;
}

static void refresh(void)
{
    HKEY hk;
    DWORD val = 0, type = 0, size = sizeof val;
    LONG r = RegOpenKeyExW(HKEY_LOCAL_MACHINE, REG_PATH, 0, KEY_READ, &hk);
    if (r == ERROR_FILE_NOT_FOUND) { g_state = ST_MISSING; g_value = 1; g_err = 0; return; }
    if (r != ERROR_SUCCESS) { g_state = ST_ERROR; g_value = 0; g_err = r; return; }
    r = RegQueryValueExW(hk, L"DisableLoad", NULL, &type, (LPBYTE)&val, &size);
    RegCloseKey(hk);
    if (r == ERROR_FILE_NOT_FOUND || (r == ERROR_SUCCESS && type != REG_DWORD)) {
        g_state = ST_ENABLED; g_value = 1; g_err = 0; return;
    }
    if (r != ERROR_SUCCESS) { g_state = ST_ERROR; g_value = 0; g_err = r; return; }
    g_value = val;
    g_state = (val == 0) ? ST_ENABLED : (val == 1 ? ST_DISABLED : ST_ERROR);
    g_err = 0;
}

static BOOL writeState(DWORD disable)
{
    HKEY hk;
    LONG r = RegCreateKeyExW(HKEY_LOCAL_MACHINE, REG_PATH, 0, NULL, 0,
                             KEY_SET_VALUE, NULL, &hk, NULL);
    if (r != ERROR_SUCCESS) { g_err = r; return FALSE; }
    r = RegSetValueExW(hk, L"DisableLoad", 0, REG_DWORD, (const BYTE *)&disable, sizeof disable);
    RegCloseKey(hk);
    g_err = r;
    return r == ERROR_SUCCESS;
}

static void errorBox(HWND owner, const wchar_t *what)
{
    wchar_t buf[256];
    swprintf(buf, 256, L"%ls\n%ls %lu", what,
             T(L"Fehlercode:", L"Error code:"), (unsigned long)g_err);
    MessageBoxW(owner, buf, T(L"Fehler", L"Error"), MB_ICONERROR | MB_OK);
}

static BOOL rebootNow(void)
{
    HANDLE tok;
    TOKEN_PRIVILEGES tp;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &tok)) {
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        if (LookupPrivilegeValueW(NULL, L"SeShutdownPrivilege", &tp.Privileges[0].Luid))
            AdjustTokenPrivileges(tok, FALSE, &tp, sizeof tp, NULL, NULL);
        CloseHandle(tok);
    }
    return ExitWindowsEx(EWX_REBOOT, 0);
}

static void applyToggle(void)
{
    DWORD disable = (g_state == ST_DISABLED) ? 0 : 1;
    const wchar_t *msg = disable
        ? T(L"Protokollierung deaktiviert.\nPC jetzt neu starten?",
            L"Logging disabled.\nRestart the PC now?")
        : T(L"Protokollierung aktiviert.\nPC jetzt neu starten?",
            L"Logging enabled.\nRestart the PC now?");
    if (!writeState(disable)) { errorBox(g_hwnd, T(L"Registry konnte nicht geschrieben werden.",
                                                   L"Could not write the registry.")); return; }
    refresh();
    g_pending = TRUE;
    if (MessageBoxW(g_hwnd, msg, T(L"Neustart erforderlich", L"Restart required"),
                    MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDYES) {
        if (!rebootNow()) errorBox(g_hwnd, T(L"Neustart konnte nicht gestartet werden.",
                                             L"Could not initiate the restart."));
    }
    InvalidateRect(g_hwnd, NULL, FALSE);
}

static void requestRestart(void)
{
    if (!g_pending) return;
    if (MessageBoxW(g_hwnd,
                    T(L"PC jetzt neu starten? Ungespeicherte Daten können verloren gehen.",
                      L"Restart the PC now? Unsaved work may be lost."),
                    T(L"Neustart bestätigen", L"Confirm restart"),
                    MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDYES) {
        if (!rebootNow()) errorBox(g_hwnd, T(L"Neustart konnte nicht gestartet werden.",
                                             L"Could not initiate the restart."));
    }
}

static void box(HDC hdc, int l, int t, int r, int b, COLORREF fill, COLORREF line, int rad)
{
    HBRUSH oldBrush;
    HPEN oldPen;
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, P(1), line);
    oldBrush = (HBRUSH)SelectObject(hdc, brush);
    oldPen = (HPEN)SelectObject(hdc, pen);
    RoundRect(hdc, l, t, r, b, rad, rad);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

static void label(HDC hdc, RECT r, const wchar_t *txt, HFONT font, COLORREF color, UINT flags)
{
    HFONT old = (HFONT)SelectObject(hdc, font);
    SetTextColor(hdc, color);
    SetBkMode(hdc, TRANSPARENT);
    DrawTextW(hdc, txt, -1, &r, flags);
    SelectObject(hdc, old);
}

static void button(HDC hdc, RECT r, const wchar_t *txt, int idx, BOOL enabled, COLORREF accent)
{
    COLORREF fill = !enabled ? 0x2A3024 /* #24302a */ :
                    (g_pressed == idx ? 0x2D2519 /* #19252d */ :
                     (g_hover == idx || g_focus == idx ? 0x1F8B4D /* #4d8b1f */ : accent));
    box(hdc, r.left, r.top, r.right, r.bottom, fill, !enabled ? C_LINE : fill, P(10));
    label(hdc, r, txt, g_fBody, enabled ? C_TEXT : C_MUTED,
          DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

static void draw(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    HDC mem = CreateCompatibleDC(hdc);
    RECT rc;
    HBITMAP bmp;
    HBRUSH bg;
    const wchar_t *status, *detail, *primary;
    COLORREF dot;
    wchar_t detailBuf[160];
    GetClientRect(hwnd, &rc);
    bmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
    SelectObject(mem, bmp);
    bg = CreateSolidBrush(C_BG);
    FillRect(mem, &rc, bg);
    DeleteObject(bg);

    label(mem, mkrect(P(24), P(20), P(416), P(46)),
          T(L"NVtopps Log-Steuerung", L"NVtopps Log Controller"), g_fTitle, C_TEXT,
          DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    label(mem, mkrect(P(24), P(52), P(416), P(74)),
          T(L"NVIDIA Container LocalSystem", L"NVIDIA Container LocalSystem"), g_fSmall, C_MUTED,
          DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    box(mem, P(24), P(86), P(416), P(178), C_CARD, C_LINE, P(12));
    switch (g_state) {
    case ST_MISSING:
        status = T(L"Schlüssel fehlt", L"Key missing");
        detail = T(L"Wird beim ersten Wechsel automatisch angelegt",
                   L"Created automatically on the first change");
        dot = C_AMBER;
        break;
    case ST_DISABLED:
        status = T(L"Protokollierung deaktiviert", L"Logging disabled");
        swprintf(detailBuf, 160, T(L"DisableLoad = %lu  (1 = aus)", L"DisableLoad = %lu  (1 = off)"),
                 (unsigned long)g_value);
        detail = detailBuf;
        dot = C_RED;
        break;
    case ST_ENABLED:
        status = T(L"Protokollierung aktiv", L"Logging enabled");
        swprintf(detailBuf, 160, T(L"DisableLoad = %lu  (0 = an)", L"DisableLoad = %lu  (0 = on)"),
                 (unsigned long)g_value);
        detail = detailBuf;
        dot = C_GREEN;
        break;
    default:
        status = T(L"Unbekannter Zustand", L"Unknown state");
        swprintf(detailBuf, 160, T(L"Registry-Fehler: %lu", L"Registry error: %lu"),
                 (unsigned long)g_err);
        detail = detailBuf;
        dot = C_AMBER;
        break;
    }
    label(mem, mkrect(P(44), P(96), P(396), P(112)),
          T(L"STATUS", L"STATUS"), g_fSmall, C_MUTED, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    {
        HBRUSH old = (HBRUSH)SelectObject(mem, CreateSolidBrush(dot));
        Ellipse(mem, P(44), P(128), P(56), P(140));
        DeleteObject((HBRUSH)SelectObject(mem, old));
    }
    label(mem, mkrect(P(66), P(120), P(400), P(148)), status, g_fBody, C_TEXT,
          DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    label(mem, mkrect(P(44), P(150), P(400), P(170)), detail, g_fSmall, C_MUTED,
          DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    primary = (g_state == ST_DISABLED)
        ? T(L"Logging aktivieren", L"Enable logging")
        : T(L"Logging deaktivieren", L"Disable logging");
    button(mem, g_btnOn, primary, 0, TRUE, C_BLUE);
    button(mem, g_btnRestart, T(L"PC neu starten", L"Restart PC"), 1, g_pending, 0x4A3D2A);

    label(mem, mkrect(P(24), P(254), P(416), P(282)),
          g_pending
              ? T(L"Neustart ausstehend - die Änderung wirkt danach.",
                  L"Restart pending - the change takes effect afterwards.")
              : T(L"Nach jeder Änderung ist ein Neustart erforderlich.",
                  L"A restart is required after every change."),
          g_fSmall, C_MUTED, DT_LEFT | DT_TOP | DT_WORDBREAK);
    label(mem, mkrect(P(24), P(284), P(416), P(298)),
          T(L"Administratorrechte aktiv", L"Running as administrator"), g_fSmall, C_DIM,
          DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
    DeleteObject(bmp);
    DeleteDC(mem);
    EndPaint(hwnd, &ps);
}

static int hitTest(LPARAM lp)
{
    POINT pt;
    pt.x = LOWORD(lp);
    pt.y = HIWORD(lp);
    if (PtInRect(&g_btnOn, pt)) return 0;
    if (PtInRect(&g_btnRestart, pt)) return 1;
    return -1;
}

static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    static TRACKMOUSEEVENT tme;
    switch (msg) {
    case WM_CREATE:
        g_hwnd = hwnd;
        g_de = PRIMARYLANGID((WORD)GetUserDefaultUILanguage()) == LANG_GERMAN;
        g_fTitle = CreateFontW(-P(19), 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET,
                               0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        g_fBody = CreateFontW(-P(14), 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET,
                              0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        g_fSmall = CreateFontW(-P(11), 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
                               0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        g_btnOn = mkrect(P(24), P(196), P(284), P(246));
        g_btnRestart = mkrect(P(294), P(196), P(416), P(246));
        refresh();
        return 0;
    case WM_PAINT:
        draw(hwnd);
        return 0;
    case WM_MOUSEMOVE:
        tme.cbSize = sizeof tme;
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hwnd;
        TrackMouseEvent(&tme);
        if (g_hover != (int)hitTest(lp)) {
            g_hover = hitTest(lp);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    case WM_MOUSELEAVE:
        g_hover = -1;
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    case WM_LBUTTONDOWN:
        g_pressed = hitTest(lp);
        if (g_pressed >= 0) SetCapture(hwnd);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    case WM_LBUTTONUP: {
        int up = hitTest(lp);
        if (g_pressed >= 0 && g_pressed == up) {
            if (g_pressed == 0) applyToggle(); else requestRestart();
        }
        g_pressed = -1;
        ReleaseCapture();
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }
    case WM_KEYDOWN:
        if (wp == VK_ESCAPE) DestroyWindow(hwnd);
        else if (wp == VK_TAB) {
            g_focus = (g_focus == 0) ? 1 : 0;
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (wp == VK_RETURN) {
            if (g_focus != 1) applyToggle(); else requestRestart();
        }
        return 0;
    case WM_DESTROY:
        DeleteObject(g_fTitle);
        DeleteObject(g_fBody);
        DeleteObject(g_fSmall);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show)
{
    WNDCLASSW wc = {0};
    RECT wr;
    HWND hwnd;
    MSG msg;
    (void)prev; (void)cmd; (void)show;
    g_inst = inst;
    g_scale = GetDpiForSystem() * 100 / 96;
    if (g_scale < 100) g_scale = 100;

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = wndProc;
    wc.hInstance = inst;
    wc.hIcon = LoadIconW(NULL, MAKEINTRESOURCEW(IDI_SHIELD));
    wc.hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(IDC_ARROW));
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"NvtoppsLogControl";
    RegisterClassW(&wc);

    wr.left = wr.top = 0;
    wr.right = P(440);
    wr.bottom = P(300);
    AdjustWindowRectEx(&wr, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE, 0);
    hwnd = CreateWindowExW(0, L"NvtoppsLogControl",
                           L"NVtopps Log Controller",
                           WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                           (GetSystemMetrics(SM_CXSCREEN) - (wr.right - wr.left)) / 2,
                           (GetSystemMetrics(SM_CYSCREEN) - (wr.bottom - wr.top)) / 2,
                           wr.right - wr.left, wr.bottom - wr.top, NULL, NULL, inst, NULL);
    if (!hwnd) return 1;
    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
