#include "win32-opengl.h"
#include "opengl-util.h"
#include "app.h"

static bool window_resized;
static bool running;
static bool active;
static HGLRC glrc;

LRESULT CALLBACK window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE:
		running = false;
		DestroyWindow(hwnd);
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_SIZE:
		active = wParam != SIZE_MINIMIZED;
		window_resized = true;
		break;

	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	return TRUE;
}

double GetHighResolutionTime(LARGE_INTEGER freq)
{
	LARGE_INTEGER time;
	QueryPerformanceCounter(&time);
	time.QuadPart *= 1000000;
	time.QuadPart /= freq.QuadPart;
	return time.QuadPart / 1000.;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
#ifdef _DEBUG
	AllocConsole();
	FILE *f;
	freopen_s(&f, "CONOUT$", "w", stdout);
#endif

	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);

    WNDCLASSEXA window_class = {};
	window_class.cbSize = sizeof(WNDCLASSEX);
	window_class.lpfnWndProc = window_proc;
	window_class.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
	window_class.hInstance = GetModuleHandle(0);
	window_class.hbrBackground = GetSysColorBrush(COLOR_BACKGROUND);
	window_class.hCursor = (HCURSOR)LoadImage(0, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED);
	window_class.lpszClassName = "TerrainGenerator";
	RegisterClassExA(&window_class);

	HWND hwnd = CreateWindowExA(
		NULL, "TerrainGenerator", "Terrain Generator", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT
		, 1366, 768, NULL, NULL, GetModuleHandle(NULL), 0
	);

	if (!hwnd) {
		MessageBoxA(0, "Failed to create window", "Fatal Error", 0);
		return 1;
	}

	win32_init_opengl_extensions();
	glrc = win32_create_gl_context(hwnd);

	ShowWindow(hwnd, nShowCmd);

	RECT rc;
	GetWindowRect(hwnd, &rc);
	int xPos = (GetSystemMetrics(SM_CXSCREEN) - rc.right) / 2;
	int yPos = (GetSystemMetrics(SM_CYSCREEN) - rc.bottom) / 2;

	SetWindowPos(hwnd, 0, xPos, yPos, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
	
	UpdateWindow(hwnd);

	RECT client;
	GetClientRect(hwnd, &client);
	unsigned int w = client.right;
	unsigned int h = client.bottom;
	app_state *state = app_init(w, h);

	if (state) {
		double dt = 0;
		double dt_elapsed = 0;
		running = true;

		const unsigned int FPS = 60;
		const float ms_per_frame = 1000. / FPS;

		while (running) {
			MSG msg;

			double start;
			start = GetHighResolutionTime(freq);

			app_input input = {};
			app_window_info window_info = {};
			window_resized = false;

			BYTE keys[256];
			GetKeyboardState(keys);

			while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
				if (msg.message == WM_QUIT) {
					running = false;
				}

				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			if (active) {
				GetKeyboardState(keys);
				input.keyboard.forward.ended_down = keys['W'] & 0x80;
				input.keyboard.backward.ended_down = keys['S'] & 0x80;
				input.keyboard.left.ended_down = keys['A'] & 0x80;
				input.keyboard.right.ended_down = keys['D'] & 0x80;
				input.keyboard.cam_up.ended_down = keys[VK_UP] & 0x80;
				input.keyboard.cam_down.ended_down = keys[VK_DOWN] & 0x80;
				input.keyboard.cam_left.ended_down = keys[VK_LEFT] & 0x80;
				input.keyboard.cam_right.ended_down = keys[VK_RIGHT] & 0x80;

				POINT p;
				GetCursorPos(&p);
				ScreenToClient(hwnd, &p);
				input.mouse.pos.x = p.x;
				input.mouse.pos.y = p.y;

				input.mouse.left.ended_down = (GetAsyncKeyState(VK_LBUTTON) & 0x8000);

				RECT client;
				GetClientRect(hwnd, &client);
				window_info.w = client.right - client.left;
				window_info.h = client.bottom - client.top;
				window_info.resize = window_resized;
				window_info.running = running;

				app_update_and_render(ms_per_frame / 1000.f, state, &input, &window_info);

				double finish = GetHighResolutionTime(freq);
				dt = finish - start;

				if (dt < ms_per_frame) {
					while (GetHighResolutionTime(freq) - start < ms_per_frame);
					SwapBuffers(wglGetCurrentDC());
				}
			}
			else {
				Sleep(10);
			}
		}

		delete state;
	}

    wglMakeCurrent(0, 0);
    wglDeleteContext(glrc);

    return 0;
}