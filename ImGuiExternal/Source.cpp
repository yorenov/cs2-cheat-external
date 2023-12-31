#include "xorstr.hpp"
//#include "Functions.h"
#include "Memory.h"
#include "Overlay.h"

LPCSTR TargetProcess = "cs2.exe";
bool ShowMenu = false;
bool Unhook = false;
bool ally_esp = false;
bool enemy_esp = true;
bool enemy_box = false;
bool enemy_lines = false;
bool enemy_armor = false;
bool enemy_skeleton = false;
bool enemy_esp_health_bar = false;
bool enemy_name = false;
bool crosshair = true;
bool ImGui_Initialised = false;
bool CreateConsole = false;

namespace OverlayWindow {
	WNDCLASSEX WindowClass;
	HWND Hwnd;
	LPCSTR Name;
}

namespace DirectX9Interface {
	IDirect3D9Ex* Direct3D9 = NULL;
	IDirect3DDevice9Ex* pDevice = NULL;
	D3DPRESENT_PARAMETERS pParams = { NULL };
	MARGINS Margin = { -1 };
	MSG Message = { NULL };
}

void Draw() {
	RGBA Cyan = { 0, 231, 255, 255 };
	if (crosshair) DrawCircleFilled(Process::WindowWidth / 2, Process::WindowHeight / 2, 3, &Cyan);

	if (!ally_esp && !enemy_esp) {
		return;
	}

	RGBA White = { 255, 255, 255, 255 };
	if (!Game::client) {
		init_modules();
	}
	//0x946598
	uintptr_t entity_list = RPM<uintptr_t>(Game::client + dwEntityList);
	uintptr_t localplayer = RPM<uintptr_t>(Game::client + LOCALPLAYER);
	int localplayer_team = RPM<uintptr_t>(localplayer + TEAM);
	int ent_idx = 0;

	for (int i = 0; i < 64; i++) 
	{
		uintptr_t ent = GetEntity(i);
		if (ent == NULL || ent == localplayer) {
			continue;
		}
	
		Vector3 absOrigin = GetPlayerPos(ent);
		Vector3 w2s_absOrigin;

		if (!WorldToScreen(absOrigin, w2s_absOrigin)) {
			continue;
		}

		int ent_team = RPM<int>(ent + TEAM);
		if (ent_team == localplayer_team && !ally_esp) {
			continue;
		}

		int health = RPM<int>(ent + HEALTH);
		if (health <= 0 || health > 120) {
			continue;
		}

		Vector3 headpos = absOrigin;
		headpos.z += 35;
		headpos.z += headpos.z - absOrigin.z;

		Vector3 w2s_headpos;

		if (!WorldToScreen(headpos, w2s_headpos)) {
			continue;
		}


#ifdef  _DEBUG
		char ent_text[256];
		sprintf(ent_text, xorstr_("%s --> %d		%f  %f  %f"), name, health, headpos.x, headpos.y, headpos.z);
		DrawStrokeText(30, 100 + 16 * ent_idx, &White, ent_text);
		ent_idx++;
#endif //  _DEBUG

	
		RGBA color = { 255, 255, 255, 255 };

		if(ent_team == 2)
		{
			color = { 255, 0, 0, 255 };
		}
		else
		{
			color = { 0, 0, 255, 255 };
		}

		if (enemy_box) {
			DrawEspBox2D(w2s_absOrigin, w2s_headpos, &color, 1);
		}

		if (enemy_name) {
			DrawNameTag(w2s_absOrigin, w2s_headpos, (char*)GetPlayerName(ent).c_str());
		}

		if (enemy_esp_health_bar) {
			DrawHealthBar(w2s_absOrigin, w2s_headpos, health);
		}
		
		

	//	if (enemy_skeleton) {
		//	DrawBones(bonematrix_addr, &color, 1);
	//	}
	}

	if (GetAsyncKeyState(VK_MENU))
	{
		uintptr_t playerFromCrosshair = GetPlayerFromCrosshair();
		if (playerFromCrosshair != NULL)
		{
			uintptr_t localPlayer = RPM<uintptr_t>(Game::client + LOCALPLAYER);
			int lpTeam = RPM<int>(localPlayer + TEAM);
			int entTeam = RPM<int>(playerFromCrosshair + TEAM);

			if ((lpTeam != entTeam) || ally_esp)
			{
				INPUT inp;
				ZeroMemory(&inp, sizeof(inp));
				inp.type = INPUT_MOUSE;
				inp.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
				inp.mi.time = 0;
				SendInput(1, &inp, sizeof(INPUT));
				Sleep(30);
				inp.mi.dwFlags = MOUSEEVENTF_LEFTUP;
				SendInput(1, &inp, sizeof(INPUT));
			}
		}
	}

}

void DrawMenu() {
	static int item = 0;
	const auto flags = (ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);


	ImGui::SetNextWindowSize({ 545.f,379.f });
	ImGui::Begin("Hack", NULL, flags);
	ImGui::SetCursorPos({ 25.f,40.f });
	
	ImGui::SetCursorPos({ 25.f,100.f });
	if (ImGui::Button("Visuals", { 100.f,40.f }))
	{
		item = 1;
	}

	ImGui::SetCursorPos({ 145.f,23.f });
	ImGui::BeginChild("child0", { 379.f,349.f }, true);

	if (item == 1)
	{
		ImGui::Text("ESP");
		ImGui::Checkbox("box##visuals box", &enemy_box);
		
		ImGui::Checkbox("health##visuals health", &enemy_esp_health_bar);
		ImGui::Checkbox("nicks##visuals nicks", &enemy_name);
		ImGui::Checkbox("recoil##visual crosshair", &crosshair);

	}

	ImGui::EndChild();


	ImGui::End();
}

void Render() {
	if (GetAsyncKeyState(VK_DELETE) & 1) Unhook = true;
	if (GetAsyncKeyState(VK_INSERT) & 1) ShowMenu = !ShowMenu;

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	Draw();

	if (ShowMenu)
		DrawMenu();

	ImGui::EndFrame();

	DirectX9Interface::pDevice->SetRenderState(D3DRS_ZENABLE, false);
	DirectX9Interface::pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
	DirectX9Interface::pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, false);

	DirectX9Interface::pDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
	if (DirectX9Interface::pDevice->BeginScene() >= 0) {
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		DirectX9Interface::pDevice->EndScene();
	}

	HRESULT result = DirectX9Interface::pDevice->Present(NULL, NULL, NULL, NULL);
	if (result == D3DERR_DEVICELOST && DirectX9Interface::pDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) {
		ImGui_ImplDX9_InvalidateDeviceObjects();
		DirectX9Interface::pDevice->Reset(&DirectX9Interface::pParams);
		ImGui_ImplDX9_CreateDeviceObjects();
	}
}

void MainLoop() {
	static RECT OldRect;
	ZeroMemory(&DirectX9Interface::Message, sizeof(MSG));

	while (DirectX9Interface::Message.message != WM_QUIT) {
		if (Unhook) {
			ImGui_ImplDX9_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();
			DestroyWindow(OverlayWindow::Hwnd);
			UnregisterClass(OverlayWindow::WindowClass.lpszClassName, OverlayWindow::WindowClass.hInstance);
			exit(0);
		}

		if (PeekMessage(&DirectX9Interface::Message, OverlayWindow::Hwnd, 0, 0, PM_REMOVE)) {
			TranslateMessage(&DirectX9Interface::Message);
			DispatchMessage(&DirectX9Interface::Message);
		}
		HWND ForegroundWindow = GetForegroundWindow();
		if (ForegroundWindow == Process::Hwnd) {
			HWND TempProcessHwnd = GetWindow(ForegroundWindow, GW_HWNDPREV);
			SetWindowPos(OverlayWindow::Hwnd, TempProcessHwnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		}

		RECT TempRect;
		POINT TempPoint;
		ZeroMemory(&TempRect, sizeof(RECT));
		ZeroMemory(&TempPoint, sizeof(POINT));

		GetClientRect(Process::Hwnd, &TempRect);
		ClientToScreen(Process::Hwnd, &TempPoint);

		TempRect.left = TempPoint.x;
		TempRect.top = TempPoint.y;
		ImGuiIO& io = ImGui::GetIO();
		io.ImeWindowHandle = Process::Hwnd;

		POINT TempPoint2;
		GetCursorPos(&TempPoint2);
		io.MousePos.x = TempPoint2.x - TempPoint.x;
		io.MousePos.y = TempPoint2.y - TempPoint.y;

		if (GetAsyncKeyState(0x1)) {
			io.MouseDown[0] = true;
			io.MouseClicked[0] = true;
			io.MouseClickedPos[0].x = io.MousePos.x;
			io.MouseClickedPos[0].x = io.MousePos.y;
		}
		else {
			io.MouseDown[0] = false;
		}

		if (TempRect.left != OldRect.left || TempRect.right != OldRect.right || TempRect.top != OldRect.top || TempRect.bottom != OldRect.bottom) {
			OldRect = TempRect;
			Process::WindowWidth = TempRect.right;
			Process::WindowHeight = TempRect.bottom;
			DirectX9Interface::pParams.BackBufferWidth = Process::WindowWidth;
			DirectX9Interface::pParams.BackBufferHeight = Process::WindowHeight;
			SetWindowPos(OverlayWindow::Hwnd, (HWND)0, TempPoint.x, TempPoint.y, Process::WindowWidth, Process::WindowHeight, SWP_NOREDRAW);
			DirectX9Interface::pDevice->Reset(&DirectX9Interface::pParams);
		}
		Render();
	}
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	if (DirectX9Interface::pDevice != NULL) {
		DirectX9Interface::pDevice->EndScene();
		DirectX9Interface::pDevice->Release();
	}
	if (DirectX9Interface::Direct3D9 != NULL) {
		DirectX9Interface::Direct3D9->Release();
	}
	DestroyWindow(OverlayWindow::Hwnd);
	UnregisterClass(OverlayWindow::WindowClass.lpszClassName, OverlayWindow::WindowClass.hInstance);
}

bool DirectXInit() {
	if (FAILED(Direct3DCreate9Ex(D3D_SDK_VERSION, &DirectX9Interface::Direct3D9))) {
		return false;
	}

	D3DPRESENT_PARAMETERS Params = { 0 };
	Params.Windowed = TRUE;
	Params.SwapEffect = D3DSWAPEFFECT_DISCARD;
	Params.hDeviceWindow = OverlayWindow::Hwnd;
	Params.MultiSampleQuality = D3DMULTISAMPLE_NONE;
	Params.BackBufferFormat = D3DFMT_A8R8G8B8;
	Params.BackBufferWidth = Process::WindowWidth;
	Params.BackBufferHeight = Process::WindowHeight;
	Params.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	Params.EnableAutoDepthStencil = TRUE;
	Params.AutoDepthStencilFormat = D3DFMT_D16;
	Params.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	Params.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;

	if (FAILED(DirectX9Interface::Direct3D9->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, OverlayWindow::Hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &Params, 0, &DirectX9Interface::pDevice))) {
		DirectX9Interface::Direct3D9->Release();
		return false;
	}

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantTextInput || ImGui::GetIO().WantCaptureKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.Fonts->AddFontDefault();

	ImGui_ImplWin32_Init(OverlayWindow::Hwnd);
	ImGui_ImplDX9_Init(DirectX9Interface::pDevice);
	DirectX9Interface::Direct3D9->Release();
	return true;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WinProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, Message, wParam, lParam))
		return true;

	switch (Message) {
	case WM_DESTROY:
		if (DirectX9Interface::pDevice != NULL) {
			DirectX9Interface::pDevice->EndScene();
			DirectX9Interface::pDevice->Release();
		}
		if (DirectX9Interface::Direct3D9 != NULL) {
			DirectX9Interface::Direct3D9->Release();
		}
		PostQuitMessage(0);
		exit(4);
		break;
	case WM_SIZE:
		if (DirectX9Interface::pDevice != NULL && wParam != SIZE_MINIMIZED) {
			ImGui_ImplDX9_InvalidateDeviceObjects();
			DirectX9Interface::pParams.BackBufferWidth = LOWORD(lParam);
			DirectX9Interface::pParams.BackBufferHeight = HIWORD(lParam);
			//HRESULT hr = DirectX9Interface::pDevice->Reset(&DirectX9Interface::pParams);
			//if (hr == D3DERR_INVALIDCALL)
				//IM_ASSERT(0);
			ImGui_ImplDX9_CreateDeviceObjects();
		}
		break;
	default:
		return DefWindowProc(hWnd, Message, wParam, lParam);
		break;
	}
	return 0;
}

void SetupWindow() {
	OverlayWindow::WindowClass = {
		sizeof(WNDCLASSEX), 0, WinProc, 0, 0, nullptr, LoadIcon(nullptr, IDI_APPLICATION), LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr, OverlayWindow::Name, LoadIcon(nullptr, IDI_APPLICATION)
	};

	RegisterClassEx(&OverlayWindow::WindowClass);
	if (Process::Hwnd) {
		static RECT TempRect = { NULL };
		static POINT TempPoint;
		GetClientRect(Process::Hwnd, &TempRect);
		ClientToScreen(Process::Hwnd, &TempPoint);
		TempRect.left = TempPoint.x;
		TempRect.top = TempPoint.y;
		Process::WindowWidth = TempRect.right;
		Process::WindowHeight = TempRect.bottom;
	}

	OverlayWindow::Hwnd = CreateWindowEx(NULL, OverlayWindow::Name, OverlayWindow::Name, WS_POPUP | WS_VISIBLE, Process::WindowLeft, Process::WindowTop, Process::WindowWidth, Process::WindowHeight, NULL, NULL, 0, NULL);
	DwmExtendFrameIntoClientArea(OverlayWindow::Hwnd, &DirectX9Interface::Margin);
	SetWindowLong(OverlayWindow::Hwnd, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TRANSPARENT);
	ShowWindow(OverlayWindow::Hwnd, SW_SHOW);
	UpdateWindow(OverlayWindow::Hwnd);
}

DWORD WINAPI ProcessCheck(LPVOID lpParameter) {
	while (true) {
		if (Process::Hwnd != NULL) {
			if (Game::PID == 0) {
				exit(0);
			}
		}
	}
}

int main() {
	if (CreateConsole == false) {
		ShowWindow(GetConsoleWindow(), SW_HIDE);
		FreeConsole();
	}

	bool WindowFocus = false;
	Game::PID = GetProcessId(TargetProcess);
	while (WindowFocus == false) {
		DWORD ForegroundWindowProcessID;
		GetWindowThreadProcessId(GetForegroundWindow(), &ForegroundWindowProcessID);
		if (Game::PID == ForegroundWindowProcessID) {
			Process::ID = GetCurrentProcessId();
			Process::Handle = GetCurrentProcess();
			Process::Hwnd = GetForegroundWindow();

			RECT TempRect;
			GetWindowRect(Process::Hwnd, &TempRect);
			Process::WindowWidth = TempRect.right - TempRect.left;
			Process::WindowHeight = TempRect.bottom - TempRect.top;
			Process::WindowLeft = TempRect.left;
			Process::WindowRight = TempRect.right;
			Process::WindowTop = TempRect.top;
			Process::WindowBottom = TempRect.bottom;

			char TempTitle[MAX_PATH];
			GetWindowText(Process::Hwnd, TempTitle, sizeof(TempTitle));
			Process::Title = TempTitle;

			char TempClassName[MAX_PATH];
			GetClassName(Process::Hwnd, TempClassName, sizeof(TempClassName));
			Process::ClassName = TempClassName;

			char TempPath[MAX_PATH];
			GetModuleFileNameEx(Process::Handle, NULL, TempPath, sizeof(TempPath));
			Process::Path = TempPath;

			WindowFocus = true;
		}
	}

	//Game::handle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, false, Game::PID);

	std::string s = RandomString(10);
	OverlayWindow::Name = s.c_str();
	SetupWindow();
	DirectXInit();
	//CreateThread(0, 0, ProcessCheck, 0, 0, 0); TOFIX
	while (TRUE) {
		MainLoop();
	}
}