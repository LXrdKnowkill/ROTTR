#pragma once
#include <Windows.h>
#include <vector>

// --- ESTRUTURA PARA DADOS DO TRAINER ---
struct TrainerData {
    uintptr_t moduleBase = 0;
    DWORD moduleSize = 0;
    uintptr_t reducablesAddress = 0;
    uintptr_t ammoInChamberAddress = 0;
    bool isReducablesPatched = false;
    bool isAmmoInChamberPatched = false;
    std::vector<BYTE> originalReducablesBytes;
    std::vector<BYTE> originalAmmoInChamberBytes;
};

// --- DECLARAÇÕES DAS FUNÇÕES DO TRAINER ---
bool ApplyReducablesPatch(TrainerData& trainer);
bool RemoveReducablesPatch(TrainerData& trainer);
bool ApplyAmmoInChamberPatch(TrainerData& trainer);
bool RemoveAmmoInChamberPatch(TrainerData& trainer);

// --- VARIÁVEL GLOBAL EXTERNA ---
extern TrainerData* g_trainerData; 