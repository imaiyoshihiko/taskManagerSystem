#ifndef PLATFORM_GL2_H
#define PLATFORM_GL2_H

#include <windows.h>
#include <GL/gl.h>

extern HGLRC g_hRC;
extern HDC   g_hDC;
extern HWND  g_hWnd;

bool CreateDeviceGL(HWND hWnd);
void CleanupDeviceGL();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif
