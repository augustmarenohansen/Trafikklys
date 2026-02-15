// Trafikklys.cpp

#include "framework.h"
#include "Trafikklys.h"
#include <vector>

#define MAX_LOADSTRING 100

// Global variables
HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];
RECT client;
int pw = 30;
int pn = 30;

// 1 = red, 2 = red+yellow, 3 = green, 4 = yellow
int showCircle = 1;

struct Car
{
    int x, y;
    int dx, dy;     // movement direction
    bool fromWest;  // true = west→east, false = north→south
};

std::vector<Car> cars;



// Forward declarations
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// =======================
// Helper drawing functions
// =======================

void DrawCircle(HDC hdc, int x, int y, int radius, COLORREF color)
{
    HBRUSH brush = CreateSolidBrush(color);
    HGDIOBJ oldBrush = SelectObject(hdc, brush);
    Ellipse(hdc, x - radius, y - radius, x + radius, y + radius);
    SelectObject(hdc, oldBrush);
    DeleteObject(brush);
}

void DrawRoad(HDC hdc, RECT client)
{
    int roadWidth = 80;
    int centerX = client.right / 2;
    int centerY = client.bottom / 2;

    HBRUSH roadBrush = CreateSolidBrush(RGB(60, 60, 60));

    // Vertical road
    RECT verticalRoad{
        centerX - roadWidth / 2,
        0,
        centerX + roadWidth / 2,
        client.bottom
    };
    FillRect(hdc, &verticalRoad, roadBrush);

    // Horizontal road
    RECT horizontalRoad{
        0,
        centerY - roadWidth / 2,
        client.right,
        centerY + roadWidth / 2
    };
    FillRect(hdc, &horizontalRoad, roadBrush);

    DeleteObject(roadBrush);
}

void DrawCar(HDC hdc, int x, int y)
{
    Rectangle(hdc, x, y, x + 20, y + 20);
}


void DrawTrafficLightVertical(HDC hdc, int x, int y, int state)
{
    HBRUSH housing = CreateSolidBrush(RGB(0, 0, 0));
    HGDIOBJ oldBrush = SelectObject(hdc, housing);
    Rectangle(hdc, x, y, x + 60, y + 180);
    SelectObject(hdc, oldBrush);
    DeleteObject(housing);

    int cx = x + 30;
    int r = 18;

    if (state == 1 || state == 2)
        DrawCircle(hdc, cx, y + 30, r, RGB(255, 0, 0));

    if (state == 2 || state == 4)
        DrawCircle(hdc, cx, y + 90, r, RGB(255, 255, 0));

    if (state == 3)
        DrawCircle(hdc, cx, y + 150, r, RGB(0, 255, 0));
}

void DrawTrafficLightHorizontal(HDC hdc, int x, int y, int state)
{
    HBRUSH housing = CreateSolidBrush(RGB(0, 0, 0));
    HGDIOBJ oldBrush = SelectObject(hdc, housing);
    Rectangle(hdc, x, y, x + 180, y + 60);
    SelectObject(hdc, oldBrush);
    DeleteObject(housing);

    int cy = y + 30;
    int r = 18;

    if (state == 1 )
        DrawCircle(hdc, x + 30, cy, r, RGB(0, 255, 0));

    if (state == 2 || state == 4)
        DrawCircle(hdc, x + 90, cy, r, RGB(255, 255, 0));

    if (state == 3 || state == 4)
        DrawCircle(hdc, x + 150, cy, r, RGB(255, 0, 0));
}

// =======================
// WinMain
// =======================

int APIENTRY wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_TRAFIKKLYS, szWindowClass, MAX_LOADSTRING);

    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow))
        return FALSE;

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

// =======================
// Window setup
// =======================

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex{};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = szWindowClass;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TRAFIKKLYS));
    wcex.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

    HWND hWnd = CreateWindowW(
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0,
        CW_USEDEFAULT, 0,
        nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
        return FALSE;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    return TRUE;
}

// =======================
// Window procedure
// =======================



LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    SetTimer(hWnd, 1, 1000, nullptr);
    switch (message)
    {

    case WM_LBUTTONDOWN:
    {
        // Car from west
        Car c;
        c.x = 0;
        c.y = (client.bottom / 2) + 20;
        c.dx = 16;
        c.dy = 0;
        c.fromWest = true;
        cars.push_back(c);
        break;
    }

    case WM_RBUTTONDOWN:
    {
        // Car from north
        Car c;
        c.x = (client.right / 2) - 20;
        c.y = 0;
        c.dx = 0;
        c.dy = 16;
        c.fromWest = false;
        cars.push_back(c);
        break;
    }

     
    case WM_TIMER:
    {
        showCircle = (showCircle % 4) + 1;

        if (rand() % 100 < pw)
        {
            Car c;
            c.x = 0;
            c.y = (client.bottom / 2) + 20;
            c.dx = 16;
            c.dy = 0;
            c.fromWest = true;
            cars.push_back(c);
        }

        if (rand() % 100 < pn)
        {
            Car c;
            c.x = (client.right / 2) - 20;
            c.y = 0;
            c.dx = 0;
            c.dy = 16;
            c.fromWest = false;
            cars.push_back(c);
        }

        // Define intersection rectangle
        RECT intersection{
            client.right / 2 - 40,
            client.bottom / 2 - 40,
            client.right / 2 + 40,
            client.bottom / 2 + 40
        };

        for (size_t i = 0; i < cars.size(); i++)
        {
            Car& c = cars[i];

            // Horizontal cars (west → east)
            if (c.fromWest)
            {
                int nextX = c.x + c.dx;

                // Before intersection: free movement unless car in front
                if (c.x + 20 < intersection.left)
                {
                    bool blocked = false;
                    for (const Car& other : cars)
                    {
                        if (&other == &c) continue;
                        if (!other.fromWest) continue;
                        if (other.x > c.x && other.x - (c.x + 20) < 25)
                            blocked = true;
                    }
                    if (!blocked)
                        c.x = nextX;
                }
                // Approaching intersection
                else if (c.x + 20 >= intersection.left && c.x < intersection.right)
                {
                    if (showCircle == 1) // horizontal green
                        c.x = intersection.right; // jump to far side
                }
                // Past intersection: keep moving freely
                else
                {
                    c.x = nextX;
                }
            }
            else // Vertical cars (north → south)
            {
                int nextY = c.y + c.dy;

                if (c.y + 20 < intersection.top)
                {
                    bool blocked = false;
                    for (const Car& other : cars)
                    {
                        if (&other == &c) continue;
                        if (other.fromWest) continue;
                        if (other.y > c.y && other.y - (c.y + 20) < 25)
                            blocked = true;
                    }
                    if (!blocked)
                        c.y = nextY;
                }
                else if (c.y + 20 >= intersection.top && c.y < intersection.bottom)
                {
                    if (showCircle == 3) // vertical green
                        c.y = intersection.bottom; // jump to far side
                }
                else
                {
                    c.y = nextY;
                }
            }
        }

        InvalidateRect(hWnd, nullptr, TRUE);
        break;
    }



    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        GetClientRect(hWnd, &client);

        DrawRoad(hdc, client);

        int centerX = client.right / 2;
        int centerY = client.bottom / 2;

        // Vertical light (right of vertical road)
        DrawTrafficLightVertical(
            hdc,
            centerX + 40,
            centerY + 40,
            showCircle
        );

        // Horizontal light (above horizontal road)
        DrawTrafficLightHorizontal(
            hdc,
            centerX - 220,
            centerY - 100,
            showCircle
        );

        for (const Car& c : cars)
        {
            DrawCar(hdc, c.x, c.y);
        }


        EndPaint(hWnd, &ps);
    }
    break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// =======================
// About dialog
// =======================

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    if (message == WM_COMMAND &&
        (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL))
    {
        EndDialog(hDlg, LOWORD(wParam));
        return (INT_PTR)TRUE;
    }
    return (INT_PTR)FALSE;
}
