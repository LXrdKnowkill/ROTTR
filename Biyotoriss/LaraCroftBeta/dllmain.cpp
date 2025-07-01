// ===================================================================================
// ARQUIVO: dllmain.cpp (VERSÃO AOB SCANNING + IMGUI)
// PROJETO: ROTTR_Trainer
// TÉCNICA: Array of Bytes Scanning + Code Injection + ImGui Interface
// BASEADO: Cheat Tables do Cheat Engine
// ===================================================================================

#include "pch.h"
#include <Windows.h>
#include <iostream>
#include <vector>
#include <iomanip>
#include <TlHelp32.h>
#include <Psapi.h>

// ImGui Headers
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"
#include <d3d11.h>
#include <dxgi.h>

// Trainer Data Header
#include "TrainerData.h"

// Menu Header
#include "Menu/Menu.h"

// Manual DirectX hook implementation

// --- CONSTANTES ---
constexpr DWORD SLEEP_INTERVAL = 50;

// --- VARIÁVEIS GLOBAIS ---
TrainerData* g_trainerData = nullptr; // Pointer global para acesso no menu

// DirectX 11 Variables
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// Window variables
WNDPROC oWndProc;
HWND window = nullptr;
bool g_bInitialized = false;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// --- FUNÇÕES AUXILIARES ---

bool IsValidMemoryAddress(uintptr_t address) {
    if (address == 0) return false;
    
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery((LPCVOID)address, &mbi, sizeof(mbi)) == 0) {
        return false;
    }
    
    return (mbi.State == MEM_COMMIT) && 
           (mbi.Protect & (PAGE_READWRITE | PAGE_READONLY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE));
}

bool SafeReadMemory(uintptr_t address, void* buffer, size_t size) {
    if (!IsValidMemoryAddress(address)) {
        return false;
    }
    
    __try {
        memcpy(buffer, (void*)address, size);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool SafeWriteMemory(uintptr_t address, const void* buffer, size_t size) {
    if (!IsValidMemoryAddress(address)) {
        return false;
    }
    
    DWORD oldProtect;
    if (!VirtualProtect((LPVOID)address, size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }
    
    bool success = false;
    __try {
        memcpy((void*)address, buffer, size);
        success = true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        std::cout << "[ERRO] Exceção ao escrever memória: 0x" << std::hex << address << std::endl;
    }
    
    VirtualProtect((LPVOID)address, size, oldProtect, &oldProtect);
    return success;
}

bool InitializeConsole() {
    if (!AllocConsole()) {
        return false;
    }
    
    FILE* f;
    if (freopen_s(&f, "CONOUT$", "w", stdout) != 0) {
        FreeConsole();
        return false;
    }
    
    std::cout << "=================================" << std::endl;
    std::cout << "[HikariSystem] ROTTR Trainer v3.0 AOB + ImGui" << std::endl;
    std::cout << "[HikariSystem] Array of Bytes Scanning" << std::endl;
    std::cout << "=================================" << std::endl;
    
    return true;
}

uintptr_t GetGameModuleBase(DWORD* moduleSize = nullptr) {
    HMODULE hModule = GetModuleHandleW(L"ROTTR.exe");
    
    if (!hModule) {
        hModule = GetModuleHandleW(nullptr);
        std::cout << "[AVISO] Usando módulo principal do processo" << std::endl;
    }
    
    if (hModule && moduleSize) {
        MODULEINFO modInfo;
        if (GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(modInfo))) {
            *moduleSize = modInfo.SizeOfImage;
        }
    }
    
    uintptr_t baseAddr = (uintptr_t)hModule;
    std::cout << "[INFO] Endereço base: 0x" << std::hex << baseAddr << std::endl;
    if (moduleSize) {
        std::cout << "[INFO] Tamanho do módulo: 0x" << std::hex << *moduleSize << std::endl;
    }
    
    return baseAddr;
}

// FUNÇÃO PARA COMPARAR PADRÃO DE BYTES COM WILDCARDS
bool CompareBytePattern(const BYTE* data, const BYTE* pattern, const char* mask) {
    for (; *mask; ++mask, ++data, ++pattern) {
        if (*mask == 'x' && *data != *pattern) {
            return false;
        }
    }
    return true;
}

// FUNÇÃO PRINCIPAL DE AOB SCANNING
uintptr_t FindPattern(uintptr_t moduleBase, DWORD moduleSize, const BYTE* pattern, const char* mask) {
    for (DWORD i = 0; i < moduleSize - strlen(mask); i++) {
        if (CompareBytePattern((BYTE*)(moduleBase + i), pattern, mask)) {
            return moduleBase + i;
        }
    }
    return 0;
}

// BUSCA PADRÃO PARA "REDUCABLES" (No Ammo/Inventory Reductions)
uintptr_t FindReducablesPattern(uintptr_t moduleBase, DWORD moduleSize) {
    // Pattern: 66 89 18 48 8B 8D A8 03 00 00 E8
    BYTE pattern[] = { 0x66, 0x89, 0x18, 0x48, 0x8B, 0x8D, 0xA8, 0x03, 0x00, 0x00, 0xE8 };
    const char* mask = "xxxxxxxxxxx";
    
    std::cout << "[INFO] Procurando padrão Reducables..." << std::endl;
    uintptr_t address = FindPattern(moduleBase, moduleSize, pattern, mask);
    
    if (address) {
        std::cout << "[SUCESSO] Padrão Reducables encontrado em: 0x" << std::hex << address << std::endl;
    } else {
        std::cout << "[ERRO] Padrão Reducables não encontrado!" << std::endl;
    }
    
    return address;
}

// BUSCA PADRÃO PARA "AMMO IN CHAMBER" (No Reload/Infinite Clip)
uintptr_t FindAmmoInChamberPattern(uintptr_t moduleBase, DWORD moduleSize) {
    std::cout << "[INFO] Procurando padrão AmmoInChamber..." << std::endl;
    
    // Padrão 1: Padrão original
    BYTE pattern1[] = { 0x75, 0x00, 0x49, 0x8B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x29, 0x18, 0x49, 0x8B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00, 0x7D };
    const char* mask1 = "x?xxx?????xxxx?????x??x";
    
    uintptr_t address = FindPattern(moduleBase, moduleSize, pattern1, mask1);
    if (address) {
        std::cout << "[SUCESSO] Padrão AmmoInChamber (v1) encontrado em: 0x" << std::hex << address << std::endl;
        return address;
    }
    
    // Padrão 2: Alternativo mais simples
    BYTE pattern2[] = { 0x29, 0x18, 0x49, 0x8B };
    const char* mask2 = "xxxx";
    
    address = FindPattern(moduleBase, moduleSize, pattern2, mask2);
    if (address) {
        std::cout << "[SUCESSO] Padrão AmmoInChamber (v2) encontrado em: 0x" << std::hex << address << std::endl;
        return address;
    }
    
    // Padrão 3: Buscar por instrução SUB específica
    BYTE pattern3[] = { 0x2B, 0x00, 0x89, 0x00 };
    const char* mask3 = "x?x?";
    
    address = FindPattern(moduleBase, moduleSize, pattern3, mask3);
    if (address) {
        std::cout << "[SUCESSO] Padrão AmmoInChamber (v3) encontrado em: 0x" << std::hex << address << std::endl;
        return address;
    }
    
    std::cout << "[AVISO] Padrão AmmoInChamber não encontrado - função pode estar desabilitada temporariamente" << std::endl;
    return 0;
}

// APLICA PATCH NO REDUCABLES (No Ammo/Inventory Reductions)
bool ApplyReducablesPatch(TrainerData& trainer) {
    if (trainer.reducablesAddress == 0) {
        std::cout << "[ERRO] Endereço Reducables não encontrado!" << std::endl;
        return false;
    }
    
    if (trainer.isReducablesPatched) {
        std::cout << "[INFO] Reducables já está patchado!" << std::endl;
        return true;
    }
    
    std::cout << "[INFO] Aplicando patch Reducables (No Ammo/Inventory Reductions)..." << std::endl;
    
    // Salva bytes originais (primeiros 3 bytes: 66 89 18)
    trainer.originalReducablesBytes.resize(3);
    if (!SafeReadMemory(trainer.reducablesAddress, trainer.originalReducablesBytes.data(), 3)) {
        std::cout << "[ERRO] Falha ao ler bytes originais do Reducables!" << std::endl;
        return false;
    }
    
    // Patch: substitui "66 89 18" por "90 90 90" (NOPs)
    BYTE nopPatch[] = { 0x90, 0x90, 0x90 };
    if (!SafeWriteMemory(trainer.reducablesAddress, nopPatch, sizeof(nopPatch))) {
        std::cout << "[ERRO] Falha ao aplicar patch Reducables!" << std::endl;
        return false;
    }
    
    trainer.isReducablesPatched = true;
    std::cout << "[SUCESSO] Patch Reducables aplicado! (Munição infinita ativada)" << std::endl;
    return true;
}

// APLICA PATCH NO AMMO IN CHAMBER (No Reload/Infinite Clip)
bool ApplyAmmoInChamberPatch(TrainerData& trainer) {
    if (trainer.ammoInChamberAddress == 0) {
        std::cout << "[ERRO] Endereço AmmoInChamber não encontrado!" << std::endl;
        return false;
    }
    
    if (trainer.isAmmoInChamberPatched) {
        std::cout << "[INFO] AmmoInChamber já está patchado!" << std::endl;
        return true;
    }
    
    std::cout << "[INFO] Aplicando patch AmmoInChamber (No Reload/Infinite Clip)..." << std::endl;
    
    // O patch é no offset +9 da instrução encontrada
    uintptr_t patchAddress = trainer.ammoInChamberAddress + 9;
    
    // Salva bytes originais (2 bytes: 29 18)
    trainer.originalAmmoInChamberBytes.resize(2);
    if (!SafeReadMemory(patchAddress, trainer.originalAmmoInChamberBytes.data(), 2)) {
        std::cout << "[ERRO] Falha ao ler bytes originais do AmmoInChamber!" << std::endl;
        return false;
    }
    
    // Patch: substitui "29 18" por "90 90" (NOPs)
    BYTE nopPatch[] = { 0x90, 0x90 };
    if (!SafeWriteMemory(patchAddress, nopPatch, sizeof(nopPatch))) {
        std::cout << "[ERRO] Falha ao aplicar patch AmmoInChamber!" << std::endl;
        return false;
    }
    
    trainer.isAmmoInChamberPatched = true;
    std::cout << "[SUCESSO] Patch AmmoInChamber aplicado! (Recarga infinita ativada)" << std::endl;
    return true;
}

// REMOVE PATCH DO REDUCABLES
bool RemoveReducablesPatch(TrainerData& trainer) {
    if (!trainer.isReducablesPatched) {
        std::cout << "[INFO] Reducables não está patchado!" << std::endl;
        return true;
    }
    
    std::cout << "[INFO] Removendo patch Reducables..." << std::endl;
    
    if (!SafeWriteMemory(trainer.reducablesAddress, trainer.originalReducablesBytes.data(), trainer.originalReducablesBytes.size())) {
        std::cout << "[ERRO] Falha ao restaurar bytes originais do Reducables!" << std::endl;
        return false;
    }
    
    trainer.isReducablesPatched = false;
    std::cout << "[SUCESSO] Patch Reducables removido! (Munição normal restaurada)" << std::endl;
    return true;
}

// REMOVE PATCH DO AMMO IN CHAMBER
bool RemoveAmmoInChamberPatch(TrainerData& trainer) {
    if (!trainer.isAmmoInChamberPatched) {
        std::cout << "[INFO] AmmoInChamber não está patchado!" << std::endl;
        return true;
    }
    
    std::cout << "[INFO] Removendo patch AmmoInChamber..." << std::endl;
    
    uintptr_t patchAddress = trainer.ammoInChamberAddress + 9;
    if (!SafeWriteMemory(patchAddress, trainer.originalAmmoInChamberBytes.data(), trainer.originalAmmoInChamberBytes.size())) {
        std::cout << "[ERRO] Falha ao restaurar bytes originais do AmmoInChamber!" << std::endl;
        return false;
    }
    
    trainer.isAmmoInChamberPatched = false;
    std::cout << "[SUCESSO] Patch AmmoInChamber removido! (Recarga normal restaurada)" << std::endl;
    return true;
}

// --- DIRECTX 11 HOOK FUNCTIONS ---

void CleanupDeviceD3D()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
    g_pSwapChain = nullptr;
    g_pd3dDeviceContext = nullptr;
    g_pd3dDevice = nullptr;
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer != nullptr)
    {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }
}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    
    if (g_bInitialized && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        return true;

    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

// Função removida - usando abordagem de overlay simples

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    wchar_t windowTitle[256];
    wchar_t className[256];
    
    if (GetWindowTextW(hwnd, windowTitle, sizeof(windowTitle)/sizeof(wchar_t)) > 0) {
        GetClassNameW(hwnd, className, sizeof(className)/sizeof(wchar_t));
        
        // Procurar por palavras-chave relacionadas ao jogo
        if (wcsstr(windowTitle, L"Rise") || wcsstr(windowTitle, L"Tomb") || 
            wcsstr(windowTitle, L"Raider") || wcsstr(windowTitle, L"ROTTR")) {
            std::wcout << L"[DEBUG] Janela encontrada: '" << windowTitle << L"' [" << className << L"]" << std::endl;
        }
    }
    return TRUE;
}

HWND FindGameWindow() {
    std::cout << "[INFO] Procurando janela do jogo..." << std::endl;
    
    // Debug: listar janelas relevantes
    std::cout << "[DEBUG] Procurando janelas do jogo..." << std::endl;
    EnumWindows(EnumWindowsProc, 0);
    
    // Lista de possíveis títulos de janela
    const wchar_t* gameTitles[] = {
        L"Rise of the Tomb Raider",
        L"Rise of the Tomb Raider™",
        L"ROTTR",
        L"RiseOfTheTombRaider",
        L"Rise of the Tomb Raider: 20 Year Celebration",
        nullptr
    };
    
    HWND gameWindow = nullptr;
    
    // Tentar encontrar por título exato
    for (int i = 0; gameTitles[i] != nullptr; i++) {
        gameWindow = FindWindowW(nullptr, gameTitles[i]);
        if (gameWindow) {
            std::wcout << L"[INFO] Janela encontrada com título: " << gameTitles[i] << std::endl;
            return gameWindow;
        }
    }
    
    // Tentar encontrar por nome de classe ou executável
    gameWindow = FindWindowW(L"UnrealWindow", nullptr);
    if (gameWindow) {
        std::cout << "[INFO] Janela encontrada pela classe UnrealWindow" << std::endl;
        return gameWindow;
    }
    
    // Fallback: procurar por processo ROTTR.exe
    HWND hwnd = GetTopWindow(GetDesktopWindow());
    while (hwnd) {
        DWORD processId;
        GetWindowThreadProcessId(hwnd, &processId);
        
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
        if (hProcess) {
            wchar_t processName[MAX_PATH];
            if (GetModuleBaseNameW(hProcess, nullptr, processName, MAX_PATH)) {
                if (wcsstr(processName, L"ROTTR") || wcsstr(processName, L"Rise")) {
                    CloseHandle(hProcess);
                    std::wcout << L"[INFO] Janela encontrada pelo processo: " << processName << std::endl;
                    return hwnd;
                }
            }
            CloseHandle(hProcess);
        }
        hwnd = GetNextWindow(hwnd, GW_HWNDNEXT);
    }
    
    // Último fallback: janela ativa
    gameWindow = GetForegroundWindow();
    if (gameWindow) {
        std::cout << "[INFO] Usando janela ativa como fallback" << std::endl;
        return gameWindow;
    }
    
    return nullptr;
}

bool InitializeImGui()
{
    std::cout << "[INFO] Inicializando ImGui com overlay simples..." << std::endl;
    
    // Encontrar a janela do jogo
    window = FindGameWindow();
    if (!window) {
        std::cout << "[ERRO] Não foi possível encontrar a janela do jogo!" << std::endl;
        std::cout << "[INFO] Certifique-se de que o jogo está rodando e ativo" << std::endl;
        return false;
    }

    // Obter dimensões da janela
    RECT windowRect;
    GetClientRect(window, &windowRect);
    int width = windowRect.right - windowRect.left;
    int height = windowRect.bottom - windowRect.top;
    
    if (width <= 0) width = 1920;
    if (height <= 0) height = 1080;
    
    std::cout << "[INFO] Dimensões da janela: " << width << "x" << height << std::endl;

    // Criar um contexto DirectX simples para overlay
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = window;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };

    HRESULT result = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        featureLevelArray,
        2,
        D3D11_SDK_VERSION,
        &sd,
        &g_pSwapChain,
        &g_pd3dDevice,
        &featureLevel,
        &g_pd3dDeviceContext);

    if (FAILED(result)) {
        std::cout << "[ERRO] Falha ao criar contexto DirectX! Código: 0x" << std::hex << result << std::endl;
        
        // Fallback: tentar sem flags específicas
        sd.Flags = 0;
        sd.BufferDesc.Width = 800;
        sd.BufferDesc.Height = 600;
        
        result = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            0,
            featureLevelArray,
            2,
            D3D11_SDK_VERSION,
            &sd,
            &g_pSwapChain,
            &g_pd3dDevice,
            &featureLevel,
            &g_pd3dDeviceContext);
        
        if (FAILED(result)) {
            std::cout << "[ERRO] Falha no fallback DirectX! Código: 0x" << std::hex << result << std::endl;
            return false;
        }
        
        std::cout << "[INFO] Fallback DirectX funcionou!" << std::endl;
    }

    CreateRenderTarget();

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup Dear ImGui style
    Menu::Style();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Hook window procedure
    oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);

    g_bInitialized = true;
    std::cout << "[SUCESSO] ImGui inicializado!" << std::endl;
    return true;
}

void RenderImGui()
{
    if (!g_bInitialized || !g_pd3dDevice)
        return;

    // Start the Dear ImGui frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Render menu if visible
    if (Menu::IsVisible) {
        Menu::Render();
    }

    // Rendering
    ImGui::Render();
    const float clear_color_with_alpha[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    g_pSwapChain->Present(1, 0); // Present with vsync
}

// THREAD PRINCIPAL
DWORD WINAPI TrainerMain(HMODULE hMod) {
    TrainerData trainer = {};
    g_trainerData = &trainer; // Atribui o pointer global
    
    if (!InitializeConsole()) {
        FreeLibraryAndExitThread(hMod, 1);
        return 1;
    }
    
    // Aguarda um pouco para o jogo carregar completamente
    std::cout << "[INFO] Aguardando inicialização do jogo..." << std::endl;
    Sleep(3000);
    
    trainer.moduleBase = GetGameModuleBase(&trainer.moduleSize);
    if (trainer.moduleBase == 0) {
        std::cout << "[ERRO CRÍTICO] Não foi possível encontrar o módulo do jogo!" << std::endl;
        std::cout << "[INFO] Pressione ENTER para sair..." << std::endl;
        std::cin.get();
        FreeConsole();
        FreeLibraryAndExitThread(hMod, 1);
        return 1;
    }
    
    // Busca padrões AOB
    std::cout << "\n[INFO] === BUSCA DE PADRÕES AOB ===" << std::endl;
    trainer.reducablesAddress = FindReducablesPattern(trainer.moduleBase, trainer.moduleSize);
    trainer.ammoInChamberAddress = FindAmmoInChamberPattern(trainer.moduleBase, trainer.moduleSize);
    
    // Inicializa ImGui
    std::cout << "[INFO] Inicializando interface ImGui..." << std::endl;
    if (!InitializeImGui()) {
        std::cout << "[ERRO] Falha ao inicializar ImGui!" << std::endl;
    }
    
    std::cout << "\n[INFO] Trainer inicializado com sucesso!" << std::endl;
    std::cout << "[INFO] Controles:" << std::endl;
    std::cout << "[INFO] - F1: Toggle Munição Infinita (Reducables)" << std::endl;
    std::cout << "[INFO] - F2: Toggle Recarga Infinita (AmmoInChamber)" << std::endl;
    std::cout << "[INFO] - F3: Desativar todos os patches" << std::endl;
    std::cout << "[INFO] - INSERT: Abrir/Fechar Menu ImGui" << std::endl;
    std::cout << "[INFO] - END: Sair" << std::endl;
    std::cout << "=================================" << std::endl;
    
    bool lastF1State = false;
    bool lastF2State = false;
    bool lastF3State = false;
    bool lastInsertState = false;
    
    while (!(GetAsyncKeyState(VK_END) & 0x8000)) {
        // F1 - Toggle Reducables (Munição Infinita)
        bool currentF1State = (GetAsyncKeyState(VK_F1) & 0x8000) != 0;
        if (currentF1State && !lastF1State) {
            std::cout << "\n[INFO] === TOGGLE MUNIÇÃO INFINITA ===" << std::endl;
            if (trainer.isReducablesPatched) {
                RemoveReducablesPatch(trainer);
            } else {
                ApplyReducablesPatch(trainer);
            }
        }
        lastF1State = currentF1State;
        
        // F2 - Toggle AmmoInChamber (Recarga Infinita)
        bool currentF2State = (GetAsyncKeyState(VK_F2) & 0x8000) != 0;
        if (currentF2State && !lastF2State) {
            std::cout << "\n[INFO] === TOGGLE RECARGA INFINITA ===" << std::endl;
            if (trainer.isAmmoInChamberPatched) {
                RemoveAmmoInChamberPatch(trainer);
            } else {
                ApplyAmmoInChamberPatch(trainer);
            }
        }
        lastF2State = currentF2State;
        
        // F3 - Desativar todos os patches
        bool currentF3State = (GetAsyncKeyState(VK_F3) & 0x8000) != 0;
        if (currentF3State && !lastF3State) {
            std::cout << "\n[INFO] === DESATIVANDO TODOS OS PATCHES ===" << std::endl;
            RemoveReducablesPatch(trainer);
            RemoveAmmoInChamberPatch(trainer);
        }
        lastF3State = currentF3State;
        
        // INSERT - Toggle Menu ImGui
        bool currentInsertState = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
        if (currentInsertState && !lastInsertState) {
            Menu::IsVisible = !Menu::IsVisible;
            std::cout << "[INFO] Menu ImGui: " << (Menu::IsVisible ? "ABERTO" : "FECHADO") << std::endl;
        }
        lastInsertState = currentInsertState;
        
        // Render ImGui
        RenderImGui();
        
        Sleep(SLEEP_INTERVAL);
    }
    
    // Limpeza automática ao sair
    std::cout << "\n[INFO] Encerrando trainer..." << std::endl;
    RemoveReducablesPatch(trainer);
    RemoveAmmoInChamberPatch(trainer);
    std::cout << "[INFO] Patches removidos. Jogo restaurado ao estado original." << std::endl;
    
    // Cleanup ImGui
    if (g_bInitialized) {
        if (window && oWndProc) {
            SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)oWndProc);
        }
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        CleanupDeviceD3D();
    }
    
    Sleep(2000);
    FreeConsole();
    FreeLibraryAndExitThread(hMod, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hMod, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hMod);
        {
            HANDLE hThread = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)TrainerMain, hMod, 0, nullptr);
            if (hThread) {
                CloseHandle(hThread);
            } else {
                return FALSE;
            }
        }
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}