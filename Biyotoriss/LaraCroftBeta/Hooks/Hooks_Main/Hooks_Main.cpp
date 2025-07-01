#include "Hooks_Main.h"
#include <iostream>
#include "../Minhook/MinHook.h"

void Hooks_Init::HooksInit()
{
    std::cout << "Initializing Gamemem \n" << std::endl;
    Hooks_Init::InitGameMem();
    std::cout << "Initialized Gamemem \n" << std::endl;
    std::cout << "Initializing Hooks \n" << std::endl;

    if (MH_Initialize() != MH_OK)
    {
        std::cout << "Error in MH_Initialize \n" << std::endl;
    }

    if (MH_CreateHook(reinterpret_cast<LPVOID*>(tGamemem::ROTTR + 0x02C196F0), &Hooks::InfiniteAmmo::GetInfiniteAmmo, reinterpret_cast<LPVOID*>(&Hooks::InfiniteAmmo::Oinfiniteammo)) != MH_OK)
    {
        std::cout << "Error in MH_CreateHook \n" << std::endl;
    }

    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
    {
        std::cout << "Error in MH_EnableHook \n" << std::endl;
    }
}

BOOL Hooks_Init::InitGameMem()
{
    tGamemem::ROTTR = (uintptr_t)GetModuleHandleA("ROTTR.exe");
    if (tGamemem::ROTTR == NULL)
        return FALSE;

    return TRUE;
}
