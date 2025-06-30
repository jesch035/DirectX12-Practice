#include <windows.h>

#include "HelloTriangle.h"

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}

	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	const int width = 1280;
	const int height = 720;

	WNDCLASS wc = {};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"HelloTriangleSample";
	RegisterClass(&wc);

	HWND hwnd = CreateWindow(wc.lpszClassName, L"HelloTriangleSample", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, hInstance, nullptr);


	HelloTriangle app(hwnd, width, height, "HelloTriangleSample");
	app.OnInit();

	ShowWindow(hwnd, nCmdShow);

	bool running = true;
	while (running)
	{
		app.OnUpdate();
		app.OnRender();

		MSG msg = {};
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				running = false;
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	app.OnDestroy();
}