#include <Windows.h>
#include <math.h>
#include <signal.h>
#include <shellapi.h>
#include <stdio.h>
#include <commctrl.h>
#include "settings.h"
#pragma comment(lib, "comctl32.lib")

// most of this code is AI generated.

volatile sig_atomic_t running = 1;

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAYICON 1
#define ID_EXIT 1001
#define ID_TOGGLE_DOT 1002
#define ID_TOGGLE_TSTYLE 1003
#define ID_TOGGLE_RGB 1004

#define CURSOR_SIZE 256
#define CURSOR_CENTER 128
#define MIN_GAP 5
#define MAX_GAP 122
#define HISTORY_SIZE 5
#define SLEEP_MS 16
#define RGB_HUE_STEP 5.0f

NOTIFYICONDATA nid;
DWORD cursorstyles[] = {32512, 32513, 32514, 32515, 32516, 32642, 32643, 32644, 32645, 32646, 32648, 32649, 32650};

static void handle_sigint(int sig)
{
    running = 0;
}

struct CursorData
{
    BITMAPINFO bmiHeader;
    unsigned char *pixels;
};

float lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

static void initializeSysTray(HWND hwnd)
{
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = ID_TRAYICON;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    strcpy(nid.szTip, "Spelis's Crosshair Cursor (Click to exit)");
    Shell_NotifyIcon(NIM_ADD, &nid);
}

static void ShowContextMenu(HWND hwnd, POINT pt)
{
    HMENU hMenu = CreatePopupMenu();
    char menuText[64];

    // Checkable items
    InsertMenu(hMenu, -1, MF_BYPOSITION | (config.center_dot ? MF_CHECKED : MF_UNCHECKED),
               ID_TOGGLE_DOT, "Center Dot");
    InsertMenu(hMenu, -1, MF_BYPOSITION | (config.t_style ? MF_CHECKED : MF_UNCHECKED),
               ID_TOGGLE_TSTYLE, "T-Style");
    InsertMenu(hMenu, -1, MF_BYPOSITION | (config.rgb ? MF_CHECKED : MF_UNCHECKED),
               ID_TOGGLE_RGB, "RGB Mode");

    InsertMenu(hMenu, -1, MF_BYPOSITION, ID_EXIT, "Exit");

    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN,
                   pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(hMenu);
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP)
        {
            POINT pt;
            GetCursorPos(&pt);
            ShowContextMenu(hwnd, pt);
        }
        break;

    case WM_COMMAND:
    {
        int cmd = LOWORD(wParam);
        POINT clickPt;
        GetCursorPos(&clickPt);

        switch (cmd)
        {
        case ID_EXIT:
            running = 0;
            DestroyWindow(hwnd);
            break;

        case ID_TOGGLE_DOT:
            config.center_dot = !config.center_dot;
            break;

        case ID_TOGGLE_TSTYLE:
            config.t_style = !config.t_style;
            break;

        case ID_TOGGLE_RGB:
            config.rgb = !config.rgb;
            break;
        }
    }
    break;

    case WM_DESTROY:
        Shell_NotifyIcon(NIM_DELETE, &nid);
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

struct CursorData createCursorBitmap(int width, int height)
{
    struct CursorData data;
    ZeroMemory(&data.bmiHeader, sizeof(BITMAPINFO));
    data.bmiHeader.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    data.bmiHeader.bmiHeader.biWidth = width;
    data.bmiHeader.bmiHeader.biHeight = height; // Changed to positive height
    data.bmiHeader.bmiHeader.biPlanes = 1;
    data.bmiHeader.bmiHeader.biBitCount = 32;
    data.bmiHeader.bmiHeader.biCompression = BI_RGB;
    data.pixels = (unsigned char *)calloc(width * height * 4, 1);
    return data;
}

void drawPixel(struct CursorData *cursor, int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
    if (x < 0 || x >= cursor->bmiHeader.bmiHeader.biWidth ||
        y < 0 || y >= cursor->bmiHeader.bmiHeader.biHeight)
        return;

    int offset = (y * cursor->bmiHeader.bmiHeader.biWidth + x) * 4;
    cursor->pixels[offset] = b;
    cursor->pixels[offset + 1] = g;
    cursor->pixels[offset + 2] = r;
    cursor->pixels[offset + 3] = 255;
}

void drawLine(struct CursorData *cursor, int x1, int y1, int x2, int y2, int thickness, int rgb[3])
{
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx - dy;

    while (1)
    {
        for (int i = 0; i <= thickness / 2; i++)
        {
            for (int j = 0; j <= thickness / 2; j++)
            {
                drawPixel(cursor, x1 + i, y1 + j, rgb[0], rgb[1], rgb[2]);
            }
        }

        if (x1 == x2 && y1 == y2)
            break;
        int e2 = 2 * err;
        if (e2 > -dy)
        {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            y1 += sy;
        }
    }
}
void drawDot(struct CursorData *cursor, int centerX, int centerY, int size, int rgb[3])
{
    for (int y = centerY; y <= centerY + size / 2; y++)
    {
        for (int x = centerX; x <= centerX + size / 2; x++)
        {
            drawPixel(cursor, x, y, rgb[0], rgb[1], rgb[2]);
        }
    }
}

HCURSOR createCursorFromMemory(struct CursorData *cursorData)
{
    ICONINFO ii = {0};
    ii.fIcon = FALSE;
    ii.xHotspot = cursorData->bmiHeader.bmiHeader.biWidth / 2;
    ii.yHotspot = cursorData->bmiHeader.bmiHeader.biHeight / 2;

    HDC hdc = GetDC(NULL);
    HBITMAP hBitmap = CreateDIBitmap(hdc,
                                     &cursorData->bmiHeader.bmiHeader,
                                     CBM_INIT,
                                     cursorData->pixels,
                                     &cursorData->bmiHeader,
                                     DIB_RGB_COLORS);

    HBITMAP hMask = CreateBitmap(cursorData->bmiHeader.bmiHeader.biWidth,
                                 cursorData->bmiHeader.bmiHeader.biHeight,
                                 1, 1, NULL);

    ii.hbmMask = hMask;
    ii.hbmColor = hBitmap;

    HCURSOR hCursor = CreateIconIndirect(&ii);

    DeleteObject(hBitmap);
    DeleteObject(hMask);
    ReleaseDC(NULL, hdc);

    return hCursor;
}

float clamp(float v)
{
    if (v < 0)
        return 0;
    if (v > 255)
        return 255;
    return (float)(v + 0.5);
}

void transformHue(int *in, float degrees)
{
    // Convert degrees to radians
    const float cosA = cos(degrees * 3.1415 / 180);
    const float sinA = sin(degrees * 3.1415 / 180);

    // Rotation matrix coefficients
    const float neo[3] = {
        cosA + (1 - cosA) / 3,
        (1 - cosA) / 3 - sqrt(1 / 3.0) * sinA,
        (1 - cosA) / 3 + sqrt(1 / 3.0) * sinA};

    // Store original values
    int temp[3] = {in[0], in[1], in[2]};

    // Apply transformation
    in[0] = clamp(temp[0] * neo[0] + temp[1] * neo[1] + temp[2] * neo[2]);
    in[1] = clamp(temp[0] * neo[2] + temp[1] * neo[0] + temp[2] * neo[1]);
    in[2] = clamp(temp[0] * neo[1] + temp[1] * neo[2] + temp[2] * neo[0]);
}

void updateCrosshair(struct CrosshairConfig *config)
{
    static POINT lastPos = {0};
    static DWORD lastTime = 0;
    POINT currentPos;
    DWORD currentTime = GetTickCount();
    GetCursorPos(&currentPos);

    // Only calculate speed if time has passed
    if (lastTime && currentTime != lastTime)
    {
        float dx = currentPos.x - lastPos.x;
        float dy = currentPos.y - lastPos.y;
        float dt = (currentTime - lastTime) / 1000.0f;
        float speed = sqrt(dx * dx + dy * dy) / dt;

        // Pre-calculate new gap
        float new_gap = MIN_GAP + speed * config->expansion_factor;

        // Update moving average more efficiently
        float sum = new_gap;
        for (int i = 0; i < HISTORY_SIZE - 1; i++)
        {
            config->gap_history[i] = config->gap_history[i + 1];
            sum += config->gap_history[i];
        }
        config->gap_history[HISTORY_SIZE - 1] = new_gap;

        // Update gap with bounds checking
        config->gap = (int)fmin(MAX_GAP, fmax(MIN_GAP, sum / HISTORY_SIZE));
    }

    lastPos = currentPos;
    lastTime = currentTime;

    // Create cursor bitmap only once
    static struct CursorData cursor;
    static int initialized = 0;

    if (!initialized)
    {
        cursor = createCursorBitmap(CURSOR_SIZE, CURSOR_SIZE);
        initialized = 1;
    }
    else
    {
        // Clear previous frame
        memset(cursor.pixels, 0, CURSOR_SIZE * CURSOR_SIZE * 4);
    }

    // Draw crosshair elements
    int center = CURSOR_CENTER;
    drawLine(&cursor, center - config->length - config->gap, center, center - config->gap, center, config->thickness, config->color);
    drawLine(&cursor, center + config->gap, center, center + config->length + config->gap, center, config->thickness, config->color);
    drawLine(&cursor, center, center - config->length - config->gap, center, center - config->gap, config->thickness, config->color);

    if (!config->t_style)
    {
        drawLine(&cursor, center, center + config->gap, center, center + config->length + config->gap, config->thickness, config->color);
    }

    if (config->center_dot)
    {
        drawDot(&cursor, center, center, config->thickness, config->color);
    }

    // Create and set cursor
    HCURSOR hCursor = createCursorFromMemory(&cursor);
    for (size_t i = 0; i < sizeof(cursorstyles) / sizeof(cursorstyles[0]); i++)
    {
        SetSystemCursor(hCursor, cursorstyles[i]);
    }
}

int main()
{
    INITCOMMONCONTROLSEX icex = {0};
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);
    // Initialize gap history
    for (int i = 0; i < 5; i++)
    {
        config.gap_history[i] = 5.0f;
    }

    HCURSOR originalCursor = CopyIcon(LoadCursor(NULL, IDC_ARROW));

    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);

    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "CrosshairTrayClass";
    RegisterClassEx(&wc);

    HWND hwnd = CreateWindowEx(0, "CrosshairTrayClass", "Crosshair",
                               WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                               CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL,
                               GetModuleHandle(NULL), NULL);

    initializeSysTray(hwnd);

    MSG msg;
    while (running)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (config.rgb)
        {
            transformHue(config.color, config.rgb_step);
        }
        updateCrosshair(&config);
        Sleep(16);
    }

    SetSystemCursor(originalCursor, 32512);
    DestroyCursor(originalCursor);

    return 0;
}
