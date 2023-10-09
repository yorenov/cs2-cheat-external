#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <iostream>
#include <dwmapi.h>
#include <d3d9.h>


#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "dwmapi.lib")


DWORD GetProcessId(LPCSTR ProcessName) {
	PROCESSENTRY32 pt;
	HANDLE hsnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	pt.dwSize = sizeof(PROCESSENTRY32);
	if (Process32First(hsnap, &pt)) {
		do {
			if (!lstrcmpi(pt.szExeFile, ProcessName)) {
				CloseHandle(hsnap);
				return pt.th32ProcessID;
			}
		} while (Process32Next(hsnap, &pt));
	}
	CloseHandle(hsnap);
	return 0;
}

std::string RandomString(const int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::string tmp_s;
    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i) {
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    
    return tmp_s;
}



struct GetMemData
{
	uintptr_t addrToHandle;
	void* needAddr;
	void* buffer;
	DWORD sizeRead;
	uintptr_t addrBuffer;
	uintptr_t addrReadProcessMemory;
	uintptr_t addrSleep;
	uintptr_t addrTimeBeginPeriod;
};

struct GetHandleData
{
	DWORD pId;
	uintptr_t addrToHandle;
	uintptr_t OpenProcessAddr;

	uintptr_t createFileAddr;
	char path[MAX_PATH];
};


void GetHandleProcessAddress(GetHandleData* data)
{

	HANDLE* handle;

	typedef HANDLE(__stdcall* _OpenProcess) (DWORD, BOOL, DWORD);
	_OpenProcess _openProcess = (_OpenProcess)data->OpenProcessAddr;
	handle = (HANDLE*)(data->addrToHandle);
	*handle = _openProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, data->pId);

	typedef HANDLE(__stdcall* _CreateFile)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
	_CreateFile _createFile = (_CreateFile)data->createFileAddr;

	_createFile(data->path, GENERIC_READ, NULL, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	return;
}

void GetHandleProcessAddres() { return; }


void GetMemCode(GetMemData* data)
{
	typedef BOOL(__stdcall* _ReadProcessMemory)(HANDLE, LPCVOID, LPVOID, SIZE_T, SIZE_T*);
	_ReadProcessMemory _readProcessMemory = (_ReadProcessMemory)data->addrReadProcessMemory;

	typedef void(__stdcall* _Sleep)(DWORD);
	_Sleep __sleep = (_Sleep)data->addrSleep;

	typedef MMRESULT(__stdcall* _TimeBeginPeriod)(UINT);
	_TimeBeginPeriod _timeBeginPeriod = (_TimeBeginPeriod)data->addrTimeBeginPeriod;

	GetMemData* buf = (GetMemData*)data->addrBuffer;
	HANDLE handle = *(HANDLE*)data->addrToHandle;
	void* buffer = data->buffer;

	int tick = 0;
	_timeBeginPeriod(1);


	while (buf->addrReadProcessMemory != 1001)
	{

		if (buf->addrReadProcessMemory == 777)
		{
			_readProcessMemory(handle, buf->needAddr, buffer, buf->sizeRead, NULL);
			buf->addrReadProcessMemory = 333;
			//__sleep(1);

		}
		++tick;
		if (tick % 10000 == 0)
		{
			_timeBeginPeriod(1);
			//__sleep(1);
		}
	}

}

void GetMemCodeEnd() { return; }


float GetDist2D(float x1, float y1, float x2, float y2)
{
	return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
}

float GetDist3D(float x1, float y1, float z1, float x2, float y2, float z2)
{
	return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2) + pow(z2 - z1, 2));
}




DWORD GetWowTransitionAddres()
{
	void* addrNtSendInput = GetProcAddress(GetModuleHandle("win32u.dll"), "NtUserSendInput");
	addrNtSendInput = (void*)((byte*)addrNtSendInput + 0x6);
	DWORD oldProtect, addrRet;

	VirtualProtect(addrNtSendInput, 4, PAGE_EXECUTE_READWRITE, &oldProtect);
	memcpy(&addrRet, addrNtSendInput, 4);
	VirtualProtect(addrNtSendInput, 4, oldProtect, &oldProtect);
	return addrRet;
}


