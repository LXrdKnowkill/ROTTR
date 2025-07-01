#pragma once
#include <Windows.h>
#include <iostream>
#include "../Minhook/MinHook.h"

namespace Hooks_Init
{
    void HooksInit();
    BOOL InitGameMem();
}

namespace tGamemem
{
    extern uintptr_t ROTTR;  // Deve ser extern se usado em v�rios arquivos
}

namespace Hooks
{
    namespace InfiniteAmmo
    {
        extern void(__fastcall* Oinfiniteammo)();
        void GetInfiniteAmmo();  // Remova __declspec(naked) e use a declara��o padr�o
        extern bool InfiniteAmmo;  // Declare como extern se for utilizado em diferentes arquivos
        void SetHook();  // Declara��o da fun��o SetHook
    }
}
