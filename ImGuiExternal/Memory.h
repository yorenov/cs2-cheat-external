#include <iostream>
#include <Windows.h>
#include <string>
#include <sstream>
#include <iomanip>

#include "Header.h"

#define ENTITY_LIST  0x178C888 //client +
#define dwEntityList 0x178C888
#define LOCALPLAYER 0x17DB108 //client + 
#define VIEW_MATRIX 0x187A6E0 //engine

#define NAME_LIST 0x0050E368 //client
#define HEALTH 0x808 //player
#define dwPlayerPawn 0x5dc
#define m_vecOrigin 0x1204 // 0x12AC
#define TEAM 0x3bf //player
#define DORMANT 0x17C //player
#define LIFESTATE 0x93 //player
#define BONEMATRIX 0x578 //player https://youtu.be/elKUMiqitxY
#define NAME 0x610

#define CROSSHAID_ID 0x1580

struct Vector2
{
    float x, y;
    Vector2()
    {
        x = y = 0;
    }
    Vector2(float x, float y)
    {
        this->x = x;
        this->y = y;
    }

    Vector2 operator= (Vector2 a)
    {
        this->x = a.x;
        this->y = a.y;
        return *this;
    }



    bool IsZero()
    {
        return (this->x < 0.4f) && (this->y < 0.4f);

    }

};

struct Vector3
{
    float x, y, z;
    Vector3()
    {
        x = y = z = 0;
    }
    Vector3(float x, float y, float z)
    {
        this->x = x;
        this->y = y;
        this->z = z;
    }


    Vector3 operator / (float a)
    {
        Vector3 vec;
        vec.x = this->x / a;
        vec.y = this->y / a;
        vec.z = this->z / a;
        return vec;
    }

    Vector3 operator - (Vector3 a)
    {
        Vector3 vec;
        vec.x = this->x - a.x;
        vec.y = this->y - a.y;
        vec.z = this->z - a.z;
        return vec;
    }

    Vector3 operator=(Vector3 a)
    {
        this->x = a.x;
        this->y = a.y;
        this->z = a.z;
        return *this;
    }

    bool IsZero()
    {
        return x == 0 && y == 0 && z == 0;
    }
};

struct Vector4
{
    float x, y, z, w;
    Vector4()
    {
        x = y = z = w = 0;
    }
    Vector4(float x, float y, float z, float w)
    {
        this->x = x;
        this->y = y;
        this->z = z;
        this->w = w;
    }
};



//template<typename TYPE>
//TYPE RPM(uintptr_t address) {
//    TYPE buffer;
//    ReadProcessMemory(Game::handle, (LPCVOID)address, &buffer, sizeof(buffer), 0);
//    return buffer;
//}

float vmatrix_t[4][4];
bool WorldToScreen(const Vector3& pos, Vector3& out)
{
   // float matrix[16];
   // ReadProcessMemory(Game::handle, reinterpret_cast<void*>(Game::client + VIEW_MATRIX), reinterpret_cast<void*>(matrix), (16 * sizeof(float)), NULL);
    matrix4_4 mat = RPM<matrix4_4>((Game::client + VIEW_MATRIX));
    float* matrix = reinterpret_cast<float*>(&mat);

    Vector4 clipCoords;
    clipCoords.w = pos.x * matrix[12] + pos.y * matrix[13] + pos.z * matrix[14] + matrix[15];
    if (clipCoords.w < 0.1f)
    {
        return false;
    }

    int width = Process::WindowWidth;
    int height = Process::WindowHeight;

    clipCoords.x = pos.x * matrix[0] + pos.y * matrix[1] + pos.z * matrix[2] + matrix[3];
    clipCoords.y = pos.x * matrix[4] + pos.y * matrix[5] + pos.z * matrix[6] + matrix[7];
    clipCoords.z = pos.x * matrix[8] + pos.y * matrix[9] + pos.z * matrix[10] + matrix[11];

    Vector3 NDC;
    NDC.x = clipCoords.x / clipCoords.w;
    NDC.y = clipCoords.y / clipCoords.w;
    NDC.z = clipCoords.z / clipCoords.w;

    out.x = (width / 2 * NDC.x) + (NDC.x + width / 2);
    out.y = -(height / 2 * NDC.y) + (NDC.y + height / 2);


    return true;
}


uintptr_t GetEntity(int index) {
    //return RPM<uintptr_t>(entity_list + (index * 0x10));

    uintptr_t entity_list = RPM<uintptr_t>(Game::client + ENTITY_LIST);

    uintptr_t list_entry = RPM<uintptr_t>(entity_list + (8 * (index & 0x7FFF) >> 9) + 0);
    if (!list_entry)
        return NULL;
    uintptr_t player = RPM<uintptr_t>(list_entry + 120 * (index & 0x1FF));
    return player;


}

uintptr_t GetPlayerPawn(uintptr_t player)
{
    uintptr_t entity_list = RPM<uintptr_t>(Game::client + ENTITY_LIST);

    if (!player || !entity_list) return NULL;

    std::uint32_t playerpawn = RPM<std::uint32_t>(player + dwPlayerPawn);
    //playerpawn = 0x106;

    uintptr_t addr = (entity_list + 0x8 * ((playerpawn & 0x7FFF) >> 9) + 0);

    uintptr_t list_entry2 = RPM<uintptr_t>(addr);
    if (!list_entry2) return NULL;
    uintptr_t pCSPlayerPawn = RPM<uintptr_t>(list_entry2 + 120 * (playerpawn & 0x1FF));

    return pCSPlayerPawn;
}

uintptr_t GetPlayerFromPawn(uintptr_t pawn)
{
    
    for(int i = 1; i < 64; ++i)
    {
        uintptr_t csPlayer = GetEntity(i);
        if (GetPlayerPawn(csPlayer) == pawn)
            return csPlayer;
    }

    return NULL;

}

Vector3 GetPlayerPos(uintptr_t player)
{
    Vector3 pos{ 0, 0, 0 };
    uintptr_t entity_list = RPM<uintptr_t>(Game::client + ENTITY_LIST);

    if (!player || !entity_list) return pos;

    std::uint32_t playerpawn = RPM<std::uint32_t>(player + dwPlayerPawn);
   // playerpawn = 0XE3;

    uintptr_t addr = (entity_list + 0x8 * ((playerpawn & 0x7FFF) >> 9) + 0);

    uintptr_t list_entry2 = RPM<uintptr_t>(addr);
    if (!list_entry2) return pos;
    uintptr_t pCSPlayerPawn = RPM<uintptr_t>(list_entry2 + 120 * (playerpawn & 0x1FF));

    // if (pCSPlayerPawn == localPlayer) continue;


     // https://github.com/UnnamedZ03/CS2-external-base/blob/main/source/CSSPlayer.hpp#L132
    Vector3 origin = RPM<Vector3>(pCSPlayerPawn + m_vecOrigin);
    return origin;
}

uintptr_t GetPlayerFromCrosshair()
{
    uintptr_t entity_list = RPM<uintptr_t>(Game::client + ENTITY_LIST);

    uintptr_t localPlayer = RPM<uintptr_t>(Game::client + LOCALPLAYER);
    if (localPlayer == NULL)
        return NULL;

    uintptr_t localPlayerPawn = GetPlayerPawn(localPlayer);
    int crosshairPawnId = RPM<int>(localPlayerPawn + CROSSHAID_ID);
    if (crosshairPawnId == 0 || crosshairPawnId == -1)
        return NULL;

    uintptr_t addr = (entity_list + 0x8 * ((crosshairPawnId & 0x7FFF) >> 9) + 0);

    uintptr_t list_entry2 = RPM<uintptr_t>(addr);
    if (!list_entry2) return NULL;
    uintptr_t pCSPlayerPawn = RPM<uintptr_t>(list_entry2 + 120 * (crosshairPawnId & 0x1FF));

    return GetPlayerFromPawn(pCSPlayerPawn);
}



inline MODULEENTRY32 get_module(const char* modName, DWORD proc_id) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, proc_id);
    if (hSnap != INVALID_HANDLE_VALUE) {
        MODULEENTRY32 modEntry;
        modEntry.dwSize = sizeof(modEntry);
        if (Module32First(hSnap, &modEntry)) {
            do {
                if (!strcmp(modEntry.szModule, modName)) {
                    CloseHandle(hSnap);
                    return modEntry;
                }
            } while (Module32Next(hSnap, &modEntry));
        }
    }
    MODULEENTRY32 module = { (DWORD)-1 };
    return module;
}

void init_modules() {
    Game::client = (uintptr_t)get_module(xorstr_("client.dll"), Game::PID).modBaseAddr;

}

std::string GetPlayerName(uintptr_t entityAddr)
{
    uintptr_t entityNameAddr = entityAddr + NAME;
    std::string entityName = "";

    for (int i = 0; RPM<char>(entityNameAddr + i) != '\0'; ++i)
    {
        entityName += RPM<char>(entityNameAddr + i);
    }
    return entityName;
}