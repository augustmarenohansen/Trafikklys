// Trafikklys.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Trafikklys.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>

// --- Norsk dokumentasjon for tilleggene ---
// Følgende globale variabler, strukturer og funksjoner ble lagt til etter
// startfilen for å implementere biler, automatisk ankomst og enkel
// trafikklogikk. Kommentarene under forklarer kort hva hver del gjør.

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
// Traffic light state: 0=Rød, 1=Rød+Gul, 2=Grønn, 3=Gul, 4=Rød
// (tekst på norsk): g_lightState styrer hvilken farge som vises.
int g_lightState = 0;
// Timer for å bytte lys automatisk
const UINT_PTR G_TIMER_ID = 1;
const UINT G_TIMER_INTERVAL = 1000; // millisekunder (1s tick)
// Timer for oppdatering av bilbevegelse
const UINT_PTR G_MOVE_TIMER_ID = 2;
const UINT G_MOVE_INTERVAL = 50; // ms

// Biler: enkel struktur som holder posisjon, retning og hastighet
enum Direction { DIR_WEST_EAST, DIR_NORTH_SOUTH };
struct Car { float x, y; Direction dir; float speed; float maxSpeed; };
// Liste over alle biler i simuleringen
std::vector<Car> g_cars;

// Ankomstsannsynlighet per sekund (0.0 - 1.0)
// g_pw: sannsynlighet for bil fra vest (left->right)
// g_pn: sannsynlighet for bil fra nord (top->bottom)
double g_pw = 0.15; // vest->øst
double g_pn = 0.10; // nord->sør

bool g_randSeeded = false;
int g_accumMs = 0;
// State durations in seconds for the 5-state sequence:
// 0=Red, 1=Red+Yellow, 2=Green, 3=Yellow, 4=Red
int g_stateDurations[5] = { 5, 1, 5, 1, 1 };
int g_stateTicksRemaining = g_stateDurations[0];

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_TRAFIKKLYS, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TRAFIKKLYS));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TRAFIKKLYS));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_TRAFIKKLYS);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}


// Spawn-funksjoner: oppretter en bil og legger den til i g_cars.
// Disse ble lagt til for å kunne sende biler inn i simuleringen ved
// museklikk eller ved sannsynlighetsbasert ankomst.
// SpawnCarFromWest: oppretter en bil som kommer fra venstre og kjører mot høyre.
void SpawnCarFromWest(HWND hWnd)
{
    RECT rc; GetClientRect(hWnd, &rc);
    int h = rc.bottom - rc.top;
    int y = h/2; // center of horizontal road (bruk klient-arealens midt)
    Car c;
    c.dir = DIR_WEST_EAST;
    c.y = (float)y;
    c.x = -40.0f; // start off-screen left
    c.speed = 0.0f; // start stopped, will accelerate
    c.maxSpeed = 2.8f; // pixels per tick
    g_cars.push_back(c);
}

// SpawnCarFromNorth: oppretter en bil som kommer fra toppen og kjører nedover.
void SpawnCarFromNorth(HWND hWnd)
{
    RECT rc; GetClientRect(hWnd, &rc);
    int w = rc.right - rc.left;
    int x = w/2; // center of vertical road (bruk klient-arealens midt)
    Car c;
    c.dir = DIR_NORTH_SOUTH;
    c.x = (float)x;
    c.y = -40.0f;
    c.speed = 0.0f;
    c.maxSpeed = 2.8f;
    g_cars.push_back(c);
}

// Oppdatering av biler: enklere implementasjon uten sortering
// For hver bil beregner vi ønsket hastighet basert på:
// - lysstatus (kan den krysse?)
// - nærmeste bil foran i samme retning (unngå kollisjon)
// Biler akselererer eller bremser mot mål-hastigheten.
void UpdateCars(HWND hWnd)
{
    if (!g_randSeeded) { std::srand((unsigned)std::time(NULL)); g_randSeeded = true; }
    RECT rc; GetClientRect(hWnd, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    int roadHalf = 75;

    // Sannsynlighetsbasert ankomst: utføres omtrent hver 1000 ms
    g_accumMs += G_MOVE_INTERVAL;
    if (g_accumMs >= 1000) {
        g_accumMs = 0;
        double r1 = (double)std::rand() / RAND_MAX;
        if (r1 < g_pw) SpawnCarFromWest(hWnd);
        double r2 = (double)std::rand() / RAND_MAX;
        if (r2 < g_pn) SpawnCarFromNorth(hWnd);
    }

    // intersection bounds
    int interL = w/2 - roadHalf;
    int interR = w/2 + roadHalf;
    int interT = h/2 - roadHalf;
    int interB = h/2 + roadHalf;

    const float accel = 0.08f; // akselerasjon per tick
    const float minGap = 28.0f; // ønsket minste avstand mellom biler

    // For å unngå kollisjoner oppdaterer vi biler per retning fra fremst til bakerst.
    // Dette sørger for at bakre biler ser de oppdaterte posisjonene til bilene foran
    // i samme tick og kan tilpasse farten korrekt.
    std::vector<int> we, ns;
    for (size_t i = 0; i < g_cars.size(); ++i) {
        if (g_cars[i].dir == DIR_WEST_EAST) we.push_back((int)i);
        else ns.push_back((int)i);
    }
    // sorter etter posisjon (x eller y) stigende
    std::sort(we.begin(), we.end(), [&](int a, int b){ return g_cars[a].x < g_cars[b].x; });
    std::sort(ns.begin(), ns.end(), [&](int a, int b){ return g_cars[a].y < g_cars[b].y; });

    // oppdater west->east fra front (stor x) til bak (liten x)
    for (int k = (int)we.size()-1; k >= 0; --k) {
        int i = we[k];
        Car &c = g_cars[i];
        c.y = (float)(h/2);
        bool inIntersection = (c.x >= interL && c.x <= interR && c.y >= interT && c.y <= interB);
        bool horizGreen = (((g_lightState + 2) % 5) == 2);
        float stopX = (float)(interL - 12);
        float distToStop = stopX - c.x;
        float targetSpeed = c.maxSpeed;
        if (!horizGreen && !inIntersection && distToStop > 0.0f && distToStop <= 20.0f) targetSpeed = 0.0f;

        // finn nærmeste bil foran (oppdatert posisjon) - større x
        float frontX = 1e9f;
        for (int idx = 0; idx < (int)we.size(); ++idx) {
            int j = we[idx];
            if (j == i) continue;
            if (g_cars[j].dir == c.dir && g_cars[j].x > c.x && g_cars[j].x < frontX) frontX = g_cars[j].x;
        }
        if (frontX < 1e8f) {
            float gap = frontX - c.x;
            if (gap < minGap) targetSpeed = 0.0f;
            else if (gap < minGap + 40.0f) {
                float factor = (gap - minGap) / 40.0f;
                float limited = c.maxSpeed * factor;
                targetSpeed = (targetSpeed < limited ? targetSpeed : limited);
            }
        }
        if (c.speed < targetSpeed) c.speed = (c.speed + accel < targetSpeed ? c.speed + accel : targetSpeed);
        else if (c.speed > targetSpeed) c.speed = (c.speed - accel*2 > targetSpeed ? c.speed - accel*2 : targetSpeed);
        c.x += c.speed;
    }

    // oppdater north->south fra front (stor y) til bak (liten y)
    for (int k = (int)ns.size()-1; k >= 0; --k) {
        int i = ns[k];
        Car &c = g_cars[i];
        c.x = (float)(w/2);
        bool inIntersection = (c.x >= interL && c.x <= interR && c.y >= interT && c.y <= interB);
        bool vertGreen = (g_lightState == 2);
        float stopY = (float)(interT - 12);
        float distToStopY = stopY - c.y;
        float targetSpeed = c.maxSpeed;
        if (!vertGreen && !inIntersection && distToStopY > 0.0f && distToStopY <= 20.0f) targetSpeed = 0.0f;

        float frontY = 1e9f;
        for (int idx = 0; idx < (int)ns.size(); ++idx) {
            int j = ns[idx];
            if (j == i) continue;
            if (g_cars[j].dir == c.dir && g_cars[j].y > c.y && g_cars[j].y < frontY) frontY = g_cars[j].y;
        }
        if (frontY < 1e8f) {
            float gap = frontY - c.y;
            if (gap < minGap) targetSpeed = 0.0f;
            else if (gap < minGap + 40.0f) {
                float factor = (gap - minGap) / 40.0f;
                float limited = c.maxSpeed * factor;
                targetSpeed = (targetSpeed < limited ? targetSpeed : limited);
            }
        }
        if (c.speed < targetSpeed) c.speed = (c.speed + accel < targetSpeed ? c.speed + accel : targetSpeed);
        else if (c.speed > targetSpeed) c.speed = (c.speed - accel*2 > targetSpeed ? c.speed - accel*2 : targetSpeed);
        c.y += c.speed;
    }

    // remove cars outside window
    for (auto it = g_cars.begin(); it != g_cars.end(); ) {
        if (it->x > w + 200 || it->y > h + 200) it = g_cars.erase(it);
        else ++it;
    }
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        // start a timer so lights change automatically
        SetTimer(hWnd, G_TIMER_ID, G_TIMER_INTERVAL, NULL);
        SetTimer(hWnd, G_MOVE_TIMER_ID, G_MOVE_INTERVAL, NULL);
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // Draw simple crossing roads and two traffic lights
            RECT rc;
            GetClientRect(hWnd, &rc);
            int w = rc.right - rc.left;
            int h = rc.bottom - rc.top;

            // draw roads (vertical and horizontal)
            HBRUSH roadBrush = CreateSolidBrush(RGB(50,50,50));
            HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, roadBrush);
            // vertical road
            int roadHalf = 75;
            Rectangle(hdc, w/2 - roadHalf, 0, w/2 + roadHalf, h);
            // horizontal road
            Rectangle(hdc, 0, h/2 - roadHalf, w, h/2 + roadHalf);

            SelectObject(hdc, oldBrush);
            DeleteObject(roadBrush);

            // Helper to draw a traffic light housing and lights (vertical stack)
            auto DrawLight = [&](int x, int y, int state, bool horizontal)->void{
                // housing
                RECT box;
                int cx = x + 30;
                int cy = y + 30;
                int r = 22;
                if (horizontal) {
                    box = { x, y, x+160, y+60 };
                } else {
                    box = { x, y, x+60, y+160 };
                }
                HBRUSH black = CreateSolidBrush(RGB(30,30,30));
                FillRect(hdc, &box, black);
                DeleteObject(black);

                // Determine which of the 3 lights are on based on state
                bool onRed = (state == 0 || state == 1 || state == 4);
                bool onYellow = (state == 1 || state == 3);
                bool onGreen = (state == 2);

                // draw red
                HBRUSH bRed = CreateSolidBrush(onRed ? RGB(255,0,0) : RGB(90,0,0));
                HBRUSH old = (HBRUSH)SelectObject(hdc, bRed);
                Ellipse(hdc, cx-r, cy-r, cx+r, cy+r);
                SelectObject(hdc, old);
                DeleteObject(bRed);

                if (horizontal) {
                    // yellow to the right
                    cx += 55;
                    HBRUSH bYellow = CreateSolidBrush(onYellow ? RGB(255,255,0) : RGB(90,90,0));
                    old = (HBRUSH)SelectObject(hdc, bYellow);
                    Ellipse(hdc, cx-r, cy-r, cx+r, cy+r);
                    SelectObject(hdc, old);
                    DeleteObject(bYellow);

                    // green further right
                    cx += 55;
                    HBRUSH bGreen = CreateSolidBrush(onGreen ? RGB(0,180,0) : RGB(0,60,0));
                    old = (HBRUSH)SelectObject(hdc, bGreen);
                    Ellipse(hdc, cx-r, cy-r, cx+r, cy+r);
                    SelectObject(hdc, old);
                    DeleteObject(bGreen);
                } else {
                    // yellow below
                    cy += 55;
                    HBRUSH bYellow = CreateSolidBrush(onYellow ? RGB(255,255,0) : RGB(90,90,0));
                    old = (HBRUSH)SelectObject(hdc, bYellow);
                    Ellipse(hdc, cx-r, cy-r, cx+r, cy+r);
                    SelectObject(hdc, old);
                    DeleteObject(bYellow);

                    // green further below
                    cy += 55;
                    HBRUSH bGreen = CreateSolidBrush(onGreen ? RGB(0,180,0) : RGB(0,60,0));
                    old = (HBRUSH)SelectObject(hdc, bGreen);
                    Ellipse(hdc, cx-r, cy-r, cx+r, cy+r);
                    SelectObject(hdc, old);
                    DeleteObject(bGreen);
                }
            };

            // Draw two lights positioned outside the road so they don't block traffic
            int interL = w/2 - roadHalf;
            int interR = w/2 + roadHalf;
            int interT = h/2 - roadHalf;
            int interB = h/2 + roadHalf;

            // Plasser lys nær skjæringspunktet, men uten å blokkere kjørebanen.
            // Vertical (venstre) lys: plasser rett til venstre for skjæringspunktet,
            // slik at lyshuset står utenfor veien og er synlig for nord-sør trafikk.
            int lx = interL - 70;            // litt til venstre for intersection
            int ly = h/2 - 280;               // sentrert vertikalt ved veisenter
            DrawLight(lx, ly, g_lightState, false);

            // Horizontal (høyre/østgående) lys: plasser til høyre for skjæringspunktet og
            // under den østgående veien, slik at det ikke dekker kjørefeltet.
            int horizState = (g_lightState + 2) % 5;
            int rx = interR + 10;           // rett til høyre for intersection
            int ry = h/2 + 80;              // litt under den horisontale veien
            DrawLight(rx, ry, horizState, true);

            // draw cars
            HBRUSH carBrush = CreateSolidBrush(RGB(200,10,10));
            HBRUSH oldB = (HBRUSH)SelectObject(hdc, carBrush);
            for (const Car &c : g_cars) {
                if (c.dir == DIR_WEST_EAST) {
                    RECT r = {(int)c.x - 12, (int)c.y - 12, (int)c.x + 12, (int)c.y + 12};
                    RoundRect(hdc, r.left, r.top, r.right, r.bottom, 4,4);
                } else {
                    RECT r = {(int)c.x - 12, (int)c.y - 12, (int)c.x + 12, (int)c.y + 12};
                    RoundRect(hdc, r.left, r.top, r.right, r.bottom, 4,4);
                }
            }
            SelectObject(hdc, oldB);
            DeleteObject(carBrush);

            EndPaint(hWnd, &ps);
        }
        break;
    case WM_LBUTTONDOWN:
        // Opprett bil fra vest
        SpawnCarFromWest(hWnd);
        InvalidateRect(hWnd, NULL, TRUE);
        break;
    case WM_RBUTTONDOWN:
        // opprett bil fra nord
        SpawnCarFromNorth(hWnd);
        InvalidateRect(hWnd, NULL, TRUE);
        break;
    case WM_TIMER:
        if (wParam == G_TIMER_ID) {
            // én tick per sekund. Senk antall sekunder igjen for gjeldende state
            g_stateTicksRemaining--;
            if (g_stateTicksRemaining <= 0) {
                // gå til neste state og sett varighet
                g_lightState = (g_lightState + 1) % 5;
                g_stateTicksRemaining = g_stateDurations[g_lightState];
            }
            InvalidateRect(hWnd, NULL, TRUE);
        } else if (wParam == G_MOVE_TIMER_ID) {
            UpdateCars(hWnd);
            InvalidateRect(hWnd, NULL, TRUE);
        }
        break;
    case WM_DESTROY:
        // stop timer
        KillTimer(hWnd, G_TIMER_ID);
        KillTimer(hWnd, G_MOVE_TIMER_ID);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
