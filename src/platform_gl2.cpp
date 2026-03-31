#include "platform_gl2.h"
#include "imgui.h"
#include "backends/imgui_impl_win32.h"

HGLRC g_hRC = NULL;
HDC   g_hDC = NULL;
HWND  g_hWnd = NULL;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam
);

bool CreateDeviceGL(HWND hWnd)
{
    g_hWnd = hWnd;
    g_hDC = ::GetDC(hWnd);
    if (!g_hDC) return false;

    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int fmt = ::ChoosePixelFormat(g_hDC, &pfd);
    if (fmt == 0) return false;
    if (!::SetPixelFormat(g_hDC, fmt, &pfd)) return false;

    g_hRC = wglCreateContext(g_hDC);
    if (!g_hRC) return false;
    if (!wglMakeCurrent(g_hDC, g_hRC)) return false;
    return true;
}

void CleanupDeviceGL()
{
    if (wglGetCurrentContext() == g_hRC) wglMakeCurrent(NULL, NULL);
    if (g_hRC) { wglDeleteContext(g_hRC); g_hRC = NULL; }
    if (g_hWnd && g_hDC) { ReleaseDC(g_hWnd, g_hDC); g_hDC = NULL; }
    g_hWnd = NULL;
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    switch (msg)
    {
    case WM_SIZE: return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
