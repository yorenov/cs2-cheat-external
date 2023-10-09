#pragma once
#include <Windows.h>
#include <Windows.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <fstream>
#include "Functions.h"



#define STOP_THREAD_IN_READER_PROGRAM 0x300
#define NC_SENDINPUT 0X4D984A

namespace Process {
    DWORD ID;
    HANDLE Handle;
    HWND Hwnd;
    WNDPROC WndProc;
    int WindowWidth;
    int WindowHeight;
    int WindowLeft;
    int WindowRight;
    int WindowTop;
    int WindowBottom;
    LPCSTR Title;
    LPCSTR ClassName;
    LPCSTR Path;
}

namespace Game {
  //  HANDLE handle = NULL;
    uintptr_t client = NULL;
    DWORD PID = 0;
}

struct matrix4_4 {
	float m[16];
};


DWORD pIdReader = NULL;

BOOL GetMem(HANDLE proc, uintptr_t addrSource, void* dest, int len, void* countRead, DWORD offs)
{
	typedef BOOL(__stdcall* _ReadMem)(HANDLE, LPCVOID, LPVOID, SIZE_T, SIZE_T*);
	static _ReadMem readMem;
	static void* addrCall = nullptr;

	return ReadProcessMemory(proc, (LPCVOID)addrSource, dest, len, NULL);
}


bool finded = false;
uintptr_t addrParametr = NULL;
HANDLE hNvidia = NULL;
void* buffer;

template<typename TYPE>
TYPE RPM(uintptr_t address) {
	if (false)
	{
		TYPE buffer;
	//	ReadProcessMemory(Game::handle, (LPCVOID)address, &buffer, sizeof(buffer), 0);
		return buffer;
	}


	uintptr_t addrSource = address;
	DWORD offs = 0;
	static void* addrCall = nullptr;

	static uintptr_t addrToHandle = NULL;
	
	//uintptr_t addrParametr = NULL;
	HANDLE hRemoteThread;
	HANDLE hProc = hNvidia;

	if (addrSource == STOP_THREAD_IN_READER_PROGRAM)
	{
		if (hNvidia != NULL && addrCall != nullptr)
		{
			GetMemData getMemData;
			getMemData.addrReadProcessMemory = 1001;
			WriteProcessMemory(hProc, reinterpret_cast<void*>(addrParametr), &getMemData, sizeof(GetMemData), NULL);
			Sleep(200);
			char* zeros = new char[512];
			ZeroMemory(zeros, 512);
			WriteProcessMemory(hProc, addrCall, &zeros, 512, NULL);
			VirtualFreeEx(hProc, addrCall, NULL, MEM_RELEASE);
		}
		TYPE T;
		ZeroMemory(&T, sizeof(T));
		return T;
	}

	if (addrCall == nullptr && finded == false)
	{
		finded = true;
		PROCESSENTRY32* mEntry = new PROCESSENTRY32;
		HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
		mEntry->dwSize = sizeof(PROCESSENTRY32);
		DWORD procId = NULL;
		Process32First(snap, mEntry);
		int roaded = 0;
		do
		{
			if (!pIdReader)
			{
				HANDLE hRunningProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, mEntry->th32ProcessID);
				if (hRunningProcess == INVALID_HANDLE_VALUE)
				{
					CloseHandle(hRunningProcess);
					continue;
				}
				HANDLE hToken = NULL;
				OpenProcessToken(hRunningProcess, TOKEN_QUERY, &hToken);

				TOKEN_ELEVATION Elevation;
				DWORD cbSize = sizeof(TOKEN_ELEVATION);
				if (!GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize)) continue;
				if (!Elevation.TokenIsElevated)
				{
					CloseHandle(hRunningProcess);
					CloseHandle(hToken);
					continue;
				}
				DWORD exit_code = NULL;
				GetExitCodeProcess(hRunningProcess, &exit_code);
				if (exit_code != STILL_ACTIVE)
				{
					CloseHandle(hRunningProcess);
					CloseHandle(hToken);
					continue;
				}

				BOOL isX32;
				if (IsWow64Process(hRunningProcess, &isX32) || !isX32)
				{
					CloseHandle(hRunningProcess);
					CloseHandle(hToken);
					continue;
				}


				char* path = new char[MAX_PATH];
				char* buf = new char[1024];
				GetModuleFileNameEx(hRunningProcess, NULL, path, MAX_PATH);

				std::fstream file;
				file.open(path, std::ios::in | std::ios::binary);
				if (!file.is_open())
				{
					CloseHandle(hRunningProcess);
					CloseHandle(hToken);
					delete[] path;
					delete[] buf;
					continue;
				}
				file.read(buf, 1024);
				file.close();

				IMAGE_DOS_HEADER* pIDH = (IMAGE_DOS_HEADER*)buf;
				if (pIDH->e_magic != IMAGE_DOS_SIGNATURE)
				{
					CloseHandle(hRunningProcess);
					CloseHandle(hToken);
					delete[] path;
					delete[] buf;
					continue;

				}
				IMAGE_NT_HEADERS* pINH = (IMAGE_NT_HEADERS*)(buf + pIDH->e_lfanew);
				if (pINH->Signature != IMAGE_NT_SIGNATURE)
				{
					CloseHandle(hRunningProcess);
					CloseHandle(hToken);
					delete[] path;
					delete[] buf;
					continue;
				}

				DWORD podpis = *reinterpret_cast<DWORD*>(reinterpret_cast<byte*>(pINH) + 0x98);
				if (podpis == NULL)
				{
					CloseHandle(hRunningProcess);
					CloseHandle(hToken);
					delete[] path;
					delete[] buf;
					continue;
				}

				pIdReader = mEntry->th32ProcessID;
				CloseHandle(hRunningProcess);
				CloseHandle(hToken);
				delete[] path;
				delete[] buf;
				break;
			}
			else
			{
				CloseHandle(snap);
				procId = pIdReader;
				break;
			}


		} while (Process32Next(snap, mEntry));

		if(pIdReader == NULL)
		{
			pIdReader = GetProcessId("chrome.exe");
		}

		if (pIdReader == NULL)
		{
			MessageBox(0, "I don't finded suitable process", "Message", MB_ICONERROR);
			ExitProcess(FALSE);
		}

		CloseHandle(snap);
		//procId = mEntry->th32ProcessID;
		procId = pIdReader;

		delete mEntry;

		hNvidia = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procId);
		hProc = hNvidia;

		addrToHandle = (uintptr_t)VirtualAllocEx(hProc, 0, 4, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

		GetHandleData getHandleData;
		getHandleData.pId = Game::PID;
		getHandleData.addrToHandle = addrToHandle;
		getHandleData.OpenProcessAddr = (uintptr_t)GetProcAddress(GetModuleHandle("Kernel32.dll"), "OpenProcess");
		getHandleData.createFileAddr = (uintptr_t)GetProcAddress(GetModuleHandle("Kernel32.dll"), "CreateFileA");
		GetModuleFileNameEx(hProc, NULL, getHandleData.path, MAX_PATH);

		GetHandleData* parametr = (GetHandleData*)(VirtualAllocEx(hProc, 0, sizeof(GetHandleData), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
		WriteProcessMemory(hProc, (void*)parametr, &getHandleData, sizeof(GetHandleData), NULL);

		
//		GetHandleProcessAddress(&getHandleData);

		LPVOID allocAddr = VirtualAllocEx(hProc, NULL, 512, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		WriteProcessMemory(hProc, allocAddr, GetHandleProcessAddress, 512, NULL);
		hRemoteThread = CreateRemoteThread(hNvidia, 0, 0, (LPTHREAD_START_ROUTINE)allocAddr, (void*)parametr, 0, 0);

		while (true)
		{
			DWORD exitCode = NULL;
			BOOL bExitCode = GetExitCodeThread(hRemoteThread, &exitCode);
			if (exitCode == STILL_ACTIVE || bExitCode == STILL_ACTIVE)
			{
				Sleep(1);
				continue;
			}

			CloseHandle(hRemoteThread);
			break;
		}

		char* zeros = new char[256];
		ZeroMemory(zeros, 256);

		WriteProcessMemory(hProc, allocAddr, GetMemCode, 512, NULL);
		buffer = VirtualAllocEx(hProc, NULL, 2048, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		addrCall = allocAddr;

		addrParametr = (uintptr_t)(VirtualAllocEx(hProc, 0, sizeof(GetMemData), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));

		GetMemData getMemData;
		getMemData.addrToHandle = addrToHandle;
		getMemData.buffer = buffer;
		getMemData.needAddr = (void*)(addrSource);
		getMemData.sizeRead = sizeof(TYPE);
		getMemData.addrBuffer = addrParametr;
		getMemData.addrSleep = (uintptr_t)GetProcAddress(GetModuleHandle("Kernel32.dll"), "Sleep");
		getMemData.addrTimeBeginPeriod = (uintptr_t)GetProcAddress(GetModuleHandle("Kernel32.dll"), "timeBeginPeriod");

		void* addrOpProc = (void*)GetProcAddress(GetModuleHandle("Kernel32.dll"), "ReadProcessMemory");
		void* allocMem = VirtualAllocEx(hProc, 0, 256, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		DWORD old;

		WriteProcessMemory(hProc, allocMem, addrOpProc, 5, NULL);

		DWORD jmpAddr = -0x5 + ((uintptr_t)(addrOpProc)+0x6) - ((uintptr_t)(allocMem)+0x6);
		WriteProcessMemory(hProc, (void*)((uintptr_t)allocMem + 0x6), &jmpAddr, 4, NULL);
		byte jmp = 0xE9;
		WriteProcessMemory(hProc, (void*)((uintptr_t)allocMem + 0x5), &jmp, 1, NULL);
		//getMemData.addrReadProcessMemory = (uintptr_t)allocMem;
		getMemData.addrReadProcessMemory = (uintptr_t)GetProcAddress(GetModuleHandle("Kernel32.dll"), "ReadProcessMemory");;

		GetMemData* parametr2 = (GetMemData*)VirtualAllocEx(hProc, NULL, sizeof(GetMemData), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		WriteProcessMemory(hProc, (void*)parametr2, &getMemData, sizeof(GetMemData), NULL);
		WriteProcessMemory(hProc, (void*)addrParametr, &getMemData, sizeof(GetMemData), NULL);

		hRemoteThread = CreateRemoteThread(hNvidia, 0, 0, (LPTHREAD_START_ROUTINE)addrCall, parametr2, 0, 0);
		
	}

	uintptr_t addr = addrParametr;

	GetMemData getMemData;
	getMemData.needAddr = reinterpret_cast<void*>(addrSource);

	if (offs != NULL) getMemData.needAddr = reinterpret_cast<void*>(addrSource);

	getMemData.sizeRead = sizeof(TYPE);
	getMemData.addrReadProcessMemory = 777;
	WriteProcessMemory(hProc, reinterpret_cast<void*>(addr), &getMemData, sizeof(GetMemData), NULL);

	DWORD num = 0;
	void* p = reinterpret_cast<void*>(addr + 0x28);

	while (num != 333)
	{
		num = 0;
		GetMem(hProc, reinterpret_cast<uintptr_t>(p), &num, 4, NULL, NULL);
	}

	//++readed;
	//if (readed % 1000 == 0)
	//	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	TYPE dest;
    GetMem(hProc, reinterpret_cast<uintptr_t>(buffer), &dest, sizeof(TYPE), NULL, NULL);
	return dest;


}