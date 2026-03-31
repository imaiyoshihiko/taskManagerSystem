#include <windows.h>
#include <tchar.h>

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_opengl2.h"

#include "platform_gl2.h"
#include "UserManager.h"
#include "ProjectManager.h"
#include "TaskManager.h"
#include "ScreenNavigator.h"
#include "UiRenderer.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    UserManager userManager;
    ProjectManager projectManager;
    TaskManager taskManager;
    ScreenNavigator navigator;
    UiRenderer ui(userManager, projectManager, taskManager, navigator);

    if (!ui.InitializeFromCli())
    {
    	MessageBoxA(
    	    NULL,
    	    navigator.GetErrorMessage().c_str(),
    	    "Initialization Error",
    	    MB_ICONERROR | MB_OK
    	);
        return 1;
    }

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = _T("TaskManagerImGuiWin32GL2");
    RegisterClassEx(&wc);

    HWND hwnd = CreateWindow(
        wc.lpszClassName,
        _T("Task Manager - Dear ImGui + Win32 + OpenGL2"),
        WS_OVERLAPPEDWINDOW,
        50, 50, 1400, 900,
        NULL, NULL, wc.hInstance, NULL
    );

    if (!CreateDeviceGL(hwnd))
    {
        CleanupDeviceGL();
        DestroyWindow(hwnd);
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsLight();

    ImFontConfig fontConfig;
    const ImWchar* ranges = io.Fonts->GetGlyphRangesJapanese();
    io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/meiryo.ttc", 18.0f, &fontConfig, ranges);

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplOpenGL2_Init();

    bool done = false;
    while (!done)
    {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ui.Render();

        ImGui::Render();
        RECT rect;
        GetClientRect(hwnd, &rect);
        int displayWidth = rect.right - rect.left;
        int displayHeight = rect.bottom - rect.top;
        glViewport(0, 0, displayWidth, displayHeight);
        glClearColor(0.96f, 0.96f, 0.98f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        SwapBuffers(g_hDC);
    }

    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceGL();
    DestroyWindow(hwnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);
    return 0;
}
