#include "pch.h"

#include <iostream>
#include <Windows.h>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_opengl3.h"
#include "glad/glad.h"
#include "app/Application.h"


#include "util/utils.h"


#define MAX_LOADSTRING 100

// ȫ�ֱ���:
HINSTANCE hInst;                                // ��ǰʵ��
HWND hWnd;                                      // 
WCHAR szTitle[MAX_LOADSTRING]       = APP_TITLE;// �������ı�
WCHAR szWindowClass[MAX_LOADSTRING] = APP_TITLE;// ����������

// Data stored per platform window
struct WGL_WindowData { HDC hDC; };

// Data
static HGLRC            g_hRC;
static WGL_WindowData   g_MainWindow;
static int              g_Width;
static int              g_Height;

static const float      g_dpi_fixed = 96.0f;
static float            g_dpi_screen = 96.0f;
static float            g_dpi_scale = 1.0f;


static ::ApplicationBase* g_app = nullptr;


// �˴���ģ���а����ĺ�����ǰ������:
static ATOM                MyRegisterClass(HINSTANCE hInstance);
static BOOL                InitInstance(HINSTANCE, int);
static void                CleanupInstance();
static LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
static INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// WGL Context
static int initWGL(HWND hwnd);
static void CleanupWGL(HWND hwnd);
static void InitImGuiContext();
static void CleanupImGuiContext();

// ImGui multi viewport
static void Hook_Renderer_CreateWindow(ImGuiViewport* viewport);
static void Hook_Renderer_DestroyWindow(ImGuiViewport* viewport);
static void Hook_Platform_RenderWindow(ImGuiViewport* viewport, void*);
static void Hook_Renderer_SwapBuffers(ImGuiViewport* viewport, void*);

#define PROGRAM_HANDLER_DPI 1

void UpdateDPI(float screen_dpi)
{
    LOG_INFO("DPI update: %f", screen_dpi);

    g_dpi_screen = screen_dpi;
    g_dpi_scale = screen_dpi / g_dpi_fixed;

    RECT windowRect;
    if (GetWindowRect(hWnd, &windowRect))
    {
        SetWindowPos(hWnd, NULL,
            windowRect.left,
            windowRect.top,
            (int)(APP_WIDTH * g_dpi_scale),
            (int)(APP_HEIGHT * g_dpi_scale), SWP_NOMOVE | SWP_NOZORDER);
    }

    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = g_dpi_scale; // �ʵ�������������
}


inline float dpiScale()
{
    return g_dpi_scale;
}

std::shared_ptr<spdlog::logger> logger;
BOOL GetNtVersionNumbers(DWORD& dwMajorVer, DWORD& dwMinorVer, DWORD& dwBuildNumber)
{
    BOOL bRet = FALSE;
    HMODULE hModNtdll = NULL;
    if (hModNtdll = ::LoadLibraryW(L"ntdll.dll"))
    {
        typedef void (WINAPI* pfRTLGETNTVERSIONNUMBERS)(DWORD*, DWORD*, DWORD*);
        pfRTLGETNTVERSIONNUMBERS pfRtlGetNtVersionNumbers;
        pfRtlGetNtVersionNumbers = (pfRTLGETNTVERSIONNUMBERS)::GetProcAddress(hModNtdll, "RtlGetNtVersionNumbers");
        if (pfRtlGetNtVersionNumbers)
        {
            pfRtlGetNtVersionNumbers(&dwMajorVer, &dwMinorVer, &dwBuildNumber);
            dwBuildNumber &= 0x0ffff;
            bRet = TRUE;
        }

        ::FreeLibrary(hModNtdll);
        hModNtdll = NULL;
    }

    return bRet;
}

int GetDpiForWindow_Load(HWND hWnd)
{
    HMODULE hModule = LoadLibrary(L"USER32.dll");
    if (hModule != NULL) {
        typedef UINT(*FunctionPtr)(HWND);
        FunctionPtr pFunction = (FunctionPtr)GetProcAddress(hModule, "GetDpiForWindow");
        if (pFunction != NULL) {
            return pFunction(hWnd);
        }
        FreeLibrary(hModule);
    }
    return 96;
}

int GetWindowDpi(HWND hWnd)
{
    DWORD dwMajorVer, dwMinorVer, dwBuildNumber;
    if (GetNtVersionNumbers(dwMajorVer, dwMinorVer, dwBuildNumber)) {

        logger->info("win version number:{0}, {1}", dwMajorVer, dwMinorVer);

        if (dwMajorVer == 10 && dwMinorVer == 0) {
            return GetDpiForWindow_Load(hWnd);
        }
        else if (dwMajorVer == 6 && dwMinorVer == 1) {
            HDC hdc = GetDC(NULL); // ��ȡ��Ļ�豸������
            return GetDeviceCaps(hdc, LOGPIXELSX);
        }
    }
    return 96;
}


int WINAPI WinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow)
{
    LOG_INFO("START");
    try {
        // log
        logger = spdlog::basic_logger_mt("temp", "temp_log.txt");

        UNREFERENCED_PARAMETER(hPrevInstance);
        UNREFERENCED_PARAMETER(lpCmdLine);

        // ����������
        HANDLE hMutex = CreateMutex(NULL, TRUE, szWindowClass);

        if (NULL == hMutex)
            return 0;

        // ��黥�����Ƿ��Ѵ���
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            // �Ѿ���һ��ʵ�������У��˳�����
            CloseHandle(hMutex);
            return 0;
        }

        // ��ʼ��ȫ���ַ���
        MyRegisterClass(hInstance);

        // ִ��Ӧ�ó����ʼ��:
        if (!InitInstance(hInstance, nCmdShow))
        {
            ::UnregisterClassW(szWindowClass, hInst);
            return FALSE;
        }

        if (!initWGL(hWnd))
        {
            CleanupInstance();
            ::UnregisterClassW(szWindowClass, hInst);
            return FALSE;
        }

        InitImGuiContext();

#if PROGRAM_HANDLER_DPI 
        float dpi = (float)GetWindowDpi(hWnd);
        logger->info("dpi:{}", dpi);
        UpdateDPI(dpi);
#endif

        g_app = ::RFToolApplication::getInstance();
        g_app->main_init(__argc, __argv);

        LOG_INFO("Main Loop start.");
        // ����Ϣѭ��
        bool done = false;
        while (!done)
        {
            // Poll and handle messages (inputs, window resize, etc.)
            // See the WndProc() function below for our to dispatch events to the Win32 backend.
            MSG msg;
            while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
            {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
                if (msg.message == WM_QUIT)
                    done = true;
            }
            if (done)
                break;

            /* Limit the FPS */
            static const int fixed_fps = 60;
            static const int fixed_frame_time = 1000000 / 60;
            static long long last = utils::get_current_system_time_us();
            long long curr = utils::get_current_system_time_us();
            long long frameTime = curr - last;
            if (frameTime < fixed_frame_time)
                std::this_thread::sleep_for(std::chrono::microseconds(fixed_frame_time - frameTime));
            last = curr;

            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplWin32_NewFrame();

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, APP_WIDTH, APP_HEIGHT);

            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glEnable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            ImGui::NewFrame();

            int r = g_app->main_gui();

            // Rendering
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            // Update and Render additional Platform Windows
            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();

                // Restore the OpenGL rendering context to the main window DC, since platform windows might have changed it.
                wglMakeCurrent(g_MainWindow.hDC, g_hRC);
            }

            // Present
            ::SwapBuffers(g_MainWindow.hDC);

            static bool showWindow = false;
            if (!showWindow) {
                showWindow = true;
                ShowWindow(hWnd, nCmdShow);
            }
        }
        LOG_INFO("Main loop over.");
        
        g_app->main_shutdown();

        CleanupImGuiContext();
        CleanupInstance();
        ::UnregisterClassW(szWindowClass, hInst);

    }
    catch (::std::exception&) {
    }

    return (int)0;
}




//
//  ����: MyRegisterClass()
//
//  Ŀ��: ע�ᴰ���ࡣ
//
static ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    //wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wcex.hIcon = NULL;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    //wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.hbrBackground = NULL;
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szWindowClass;
    //wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wcex.hIconSm = NULL;

    return RegisterClassExW(&wcex);
}


//
//   ����: InitInstance(HINSTANCE, int)
//
//   Ŀ��: ����ʵ����������������ں�WGL������
//
//   ע��:
//
//        �ڴ˺����У�������ȫ�ֱ����б���ʵ�������
//        ��������ʾ�����򴰿ڡ�
//
static BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    LOG_INFO("Init hInstance");
    hInst = hInstance; // ��ʵ������洢��ȫ�ֱ�����

    hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        APP_X, APP_Y, APP_WIDTH, APP_HEIGHT, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    UpdateWindow(hWnd);

    return TRUE;
}

//
//   ����: CleanupInstance()
//
//   Ŀ��: ����ʵ���������WGL������
//
static void CleanupInstance()
{
    CleanupWGL(hWnd);
    ::DestroyWindow(hWnd);
}


static bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data)
{
    HDC hDc = ::GetDC(hWnd);
    PIXELFORMATDESCRIPTOR pfd = { 0 };
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;

    const int pf = ::ChoosePixelFormat(hDc, &pfd);
    if (pf == 0)
        return false;
    if (::SetPixelFormat(hDc, pf, &pfd) == FALSE)
        return false;
    ::ReleaseDC(hWnd, hDc);

    data->hDC = ::GetDC(hWnd);
    if (!g_hRC)
        g_hRC = wglCreateContext(data->hDC);
    return true;
}

static void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data)
{
    wglMakeCurrent(NULL, NULL);
    ::ReleaseDC(hWnd, data->hDC);
}

//
//   ����: initWGL(HWND)
//
//   Ŀ��: ����WGL������
//
//
static int initWGL(HWND hwnd)
{
    LOG_INFO("Init WGL");
    if (!CreateDeviceWGL(hwnd, &g_MainWindow))
    {
        CleanupDeviceWGL(hwnd, &g_MainWindow);
        ::DestroyWindow(hwnd);
        ::UnregisterClassW(szWindowClass, hInst);
        return FALSE;
    }
    wglMakeCurrent(g_MainWindow.hDC, g_hRC);

    LOG_INFO("GLAD Load");
    gladLoadGL();
    return TRUE;
}

//
//   ����: CleanupWGL()
//
//   Ŀ��: ����WGL������
//
//
static void CleanupWGL(HWND hwnd)
{
    CleanupDeviceWGL(hwnd, &g_MainWindow);
    wglDeleteContext(g_hRC);
}

//
//   ����: InitImGuiContext()
//
//   Ŀ��: ����ImGui������
//
static void InitImGuiContext()
{
    LOG_INFO("Init ImGUI");
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;    // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;       // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;     // Enable Multi-Viewport / Platform Windows

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_InitForOpenGL(hWnd);
    ImGui_ImplOpenGL3_Init();


    // Win32+GL needs specific hooks for viewport, as there are specific things needed to tie Win32 and GL api.
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
        IM_ASSERT(platform_io.Renderer_CreateWindow == NULL);
        IM_ASSERT(platform_io.Renderer_DestroyWindow == NULL);
        IM_ASSERT(platform_io.Renderer_SwapBuffers == NULL);
        IM_ASSERT(platform_io.Platform_RenderWindow == NULL);
        platform_io.Renderer_CreateWindow = Hook_Renderer_CreateWindow;
        platform_io.Renderer_DestroyWindow = Hook_Renderer_DestroyWindow;
        platform_io.Renderer_SwapBuffers = Hook_Renderer_SwapBuffers;
        platform_io.Platform_RenderWindow = Hook_Platform_RenderWindow;
    }
}

//
//   ����: CleanupImGuiContext()
//
//   Ŀ��: ����ImGui������
//
static void CleanupImGuiContext()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

// Support function for multi-viewports
// Unlike most other backend combination, we need specific hooks to combine Win32+OpenGL.
// We could in theory decide to support Win32-specific code in OpenGL backend via e.g. an hypothetical ImGui_ImplOpenGL3_InitForRawWin32().
static void Hook_Renderer_CreateWindow(ImGuiViewport* viewport)
{
    assert(viewport->RendererUserData == NULL);

    WGL_WindowData* data = IM_NEW(WGL_WindowData);
    CreateDeviceWGL((HWND)viewport->PlatformHandle, data);
    viewport->RendererUserData = data;
}

static void Hook_Renderer_DestroyWindow(ImGuiViewport* viewport)
{
    if (viewport->RendererUserData != NULL)
    {
        WGL_WindowData* data = (WGL_WindowData*)viewport->RendererUserData;
        CleanupDeviceWGL((HWND)viewport->PlatformHandle, data);
        IM_DELETE(data);
        viewport->RendererUserData = NULL;
    }
}

static void Hook_Platform_RenderWindow(ImGuiViewport* viewport, void*)
{
    // Activate the platform window DC in the OpenGL rendering context
    if (WGL_WindowData* data = (WGL_WindowData*)viewport->RendererUserData)
        wglMakeCurrent(data->hDC, g_hRC);
}

static void Hook_Renderer_SwapBuffers(ImGuiViewport* viewport, void*)
{
    if (WGL_WindowData* data = (WGL_WindowData*)viewport->RendererUserData)
        ::SwapBuffers(data->hDC);
}


// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_CREATE:
        break;
    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED)
        {
            g_Width = LOWORD(lParam);
            g_Height = HIWORD(lParam);
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
#if PROGRAM_HANDLER_DPI 
    case WM_DPICHANGED:
    {
        // ��ȡ�µ� DPI ��������
        int newDpiX = HIWORD(wParam);
        int newDpiY = LOWORD(wParam);

        UpdateDPI((float)newDpiX);
    }
    break;
#endif
    case WM_KEYDOWN:
    {
        // if (wParam == 'W')
        // {
        // }
        break;
    }
    case WM_KEYUP:
        break;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
