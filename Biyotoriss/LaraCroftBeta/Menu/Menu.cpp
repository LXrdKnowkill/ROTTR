#include "../pch.h"
#include "Menu.h"
#include "../ImGui/imgui.h"
#include "../TrainerData.h"

// Cores ImGui (valores corrigidos para 0.0f - 1.0f)
ImVec4 DarkViolet{ 148.0f/255.0f, 0.0f/255.0f, 211.0f/255.0f, 1.0f };
ImVec4 DarkBlue{ 0.0f/255.0f, 0.0f/255.0f, 138.0f/255.0f, 1.0f };
ImVec4 Black{ 0.0f/255.0f, 0.0f/255.0f, 0.0f/255.0f, 0.4f };
ImVec4 Gray{ 27.0f/255.0f, 27.0f/255.0f, 27.0f/255.0f, 1.0f };
ImVec4 C_Purple{ 59.0f/255.0f, 0.0f/255.0f, 144.0f/255.0f, 1.0f };
ImVec4 Red{ 1.0f, 0.0f, 0.0f, 1.0f };
ImVec4 Green{ 0.0f, 1.0f, 0.0f, 1.0f };

void Menu::Render()
{
    ImGui::SetNextWindowSize({ 450, 400 });
    ImGui::Begin("ROTTR Trainer - Hikari System", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize);
    {
        // Título
        ImGui::Text("ROTTR Trainer v3.0 AOB + ImGui");
        ImGui::Separator();
        
        if (g_trainerData) {
            // Status dos padrões AOB
            ImGui::Text("Status dos Padrões AOB:");
            
            // Status Reducables
            ImGui::Text("Reducables Pattern: ");
            ImGui::SameLine();
            if (g_trainerData->reducablesAddress != 0) {
                ImGui::TextColored(Green, "OK (0x%llX)", g_trainerData->reducablesAddress);
            } else {
                ImGui::TextColored(Red, "Não Encontrado");
            }
            
            // Status AmmoInChamber
            ImGui::Text("AmmoInChamber Pattern: ");
            ImGui::SameLine();
            if (g_trainerData->ammoInChamberAddress != 0) {
                ImGui::TextColored(Green, "OK (0x%llX)", g_trainerData->ammoInChamberAddress);
            } else {
                ImGui::TextColored(Red, "Não Encontrado");
            }
            
            ImGui::Separator();
            ImGui::Text("Controles:");
            
            // Checkbox Munição Infinita
            bool reducablesActive = g_trainerData->isReducablesPatched;
            if (ImGui::Checkbox("Munição Infinita (Reducables)", &reducablesActive)) {
                if (reducablesActive != g_trainerData->isReducablesPatched) {
                    if (reducablesActive) {
                        ApplyReducablesPatch(*g_trainerData);
                    } else {
                        RemoveReducablesPatch(*g_trainerData);
                    }
                }
            }
            
            // Status visual do patch Reducables
            ImGui::SameLine();
            if (g_trainerData->isReducablesPatched) {
                ImGui::TextColored(Green, "[ATIVO]");
            } else {
                ImGui::TextColored(Red, "[INATIVO]");
            }
            
            // Checkbox Recarga Infinita
            bool ammoInChamberActive = g_trainerData->isAmmoInChamberPatched;
            if (ImGui::Checkbox("Recarga Infinita (AmmoInChamber)", &ammoInChamberActive)) {
                if (ammoInChamberActive != g_trainerData->isAmmoInChamberPatched) {
                    if (ammoInChamberActive) {
                        ApplyAmmoInChamberPatch(*g_trainerData);
                    } else {
                        RemoveAmmoInChamberPatch(*g_trainerData);
                    }
                }
            }
            
            // Status visual do patch AmmoInChamber
            ImGui::SameLine();
            if (g_trainerData->isAmmoInChamberPatched) {
                ImGui::TextColored(Green, "[ATIVO]");
            } else {
                ImGui::TextColored(Red, "[INATIVO]");
            }
            
            ImGui::Separator();
            
            // Botão para desativar tudo
            if (ImGui::Button("Desativar Todos os Patches", ImVec2(-1, 0))) {
                RemoveReducablesPatch(*g_trainerData);
                RemoveAmmoInChamberPatch(*g_trainerData);
            }
            
            ImGui::Separator();
            
            // Informações de módulo
            ImGui::Text("Informações do Módulo:");
            ImGui::Text("Base Address: 0x%llX", g_trainerData->moduleBase);
            ImGui::Text("Module Size: 0x%X", g_trainerData->moduleSize);
            
            ImGui::Separator();
            
            // Informações de controles
            ImGui::Text("Controles de Teclado:");
            ImGui::BulletText("F1: Toggle Munição Infinita");
            ImGui::BulletText("F2: Toggle Recarga Infinita");
            ImGui::BulletText("F3: Desativar todos os patches");
            ImGui::BulletText("INSERT: Abrir/Fechar Menu");
            ImGui::BulletText("END: Sair do Trainer");
            
        } else {
            ImGui::TextColored(Red, "Trainer não inicializado!");
            ImGui::Text("Aguardando carregamento do jogo...");
            
            // Spinner de loading
            static float time = 0.0f;
            time += ImGui::GetIO().DeltaTime;
            ImGui::Text("Loading");
            ImGui::SameLine();
            
            const char* spinner = "|/-\\";
            int spinner_index = (int)(time / 0.25f) % 4;
            ImGui::Text("%c", spinner[spinner_index]);
        }
        
        ImGui::End();
    }
}

void Menu::Style()
{
    ImGui::StyleColorsDark(); // Base dark theme
    
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Configurações de estilo
    style.WindowRounding = 6.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.ScrollbarRounding = 3.0f;
    style.ChildRounding = 3.0f;
    style.PopupRounding = 3.0f;
    
    // Cores customizadas
    ImVec4* colors = ImGui::GetStyle().Colors;
    
    colors[ImGuiCol_Text] = DarkViolet;
    colors[ImGuiCol_SliderGrab] = Red;
    colors[ImGuiCol_SliderGrabActive] = DarkBlue;
    colors[ImGuiCol_Button] = Black;
    colors[ImGuiCol_ButtonHovered] = Gray;
    colors[ImGuiCol_ButtonActive] = C_Purple;
    colors[ImGuiCol_FrameBg] = Black;
    colors[ImGuiCol_FrameBgHovered] = C_Purple;
    colors[ImGuiCol_FrameBgActive] = Red;
    colors[ImGuiCol_TitleBg] = Gray;
    colors[ImGuiCol_TitleBgActive] = Black;
    colors[ImGuiCol_TitleBgCollapsed] = Gray;
    colors[ImGuiCol_CheckMark] = Green;
    colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 0.9f);
}