#pragma once
#include "imgui.h"
#include <string>
#include <fstream> 
#include <vector>
#include <sstream>
#include <algorithm>

namespace InitialSetup {

    struct SystemRecord {
        std::string name;
        int romCount = 0;
        char playthrough[128] = "";
    };

    std::vector<SystemRecord> userSystems;
    char newSystemInput[128] = ""; 
    std::string systemErrorMsg = ""; 

    char currentGoals[256] = "";
    char yearlyGoal[256] = "";

    // --- NEW: Tracks if the user has clicked the generation button ---
    bool recommendationsGenerated = false;
    std::string generationErrorMsg = ""; 

    // --- HELPER 1: Console Age Validator ---
    bool IsModernConsole(std::string consoleInput) {
        std::transform(consoleInput.begin(), consoleInput.end(), consoleInput.begin(), ::tolower);
        consoleInput.erase(std::remove(consoleInput.begin(), consoleInput.end(), ' '), consoleInput.end());

        std::vector<std::string> modernConsoles = {
            "ps4", "playstation4", "ps5", "playstation5",
            "xboxone", "xboxseriesx", "xboxseriess",
            "wiiu", "nintendoswitch", "switch", "steamdeck", "rogally"
        };

        for (const auto& modern : modernConsoles) {
            if (consoleInput == modern) return true;
        }
        return false;
    }

    // --- HELPER 2: Comma Parser ---
    std::vector<std::string> SplitGames(const std::string& input) {
        std::vector<std::string> games;
        std::stringstream ss(input);
        std::string item;
        while (std::getline(ss, item, ',')) {
            size_t start = item.find_first_not_of(" \t");
            size_t end = item.find_last_not_of(" \t");
            if (start != std::string::npos && end != std::string::npos) {
                games.push_back(item.substr(start, end - start + 1));
            }
        }
        return games;
    }

    void Render(float windowWidth, float windowHeight, GLuint bgTexture, const std::string& currentUsername) {
        
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        if (bgTexture) {
            ImGui::GetWindowDrawList()->AddImage(
                (void*)(intptr_t)bgTexture, 
                viewport->WorkPos, 
                ImVec2(viewport->WorkPos.x + viewport->WorkSize.x, viewport->WorkPos.y + viewport->WorkSize.y)
            );
        }

        float currentScale, btnWidth, btnHeight, spacing;

        if (windowHeight < 900.0f) {
            currentScale = 1.6f;     
            btnWidth = 450.0f;       
            btnHeight = 85.0f;                       
            spacing = 25.0f;                           
        } else {
            currentScale = 2.6f;     
            btnWidth = windowWidth * 0.30f;   
            btnHeight = windowHeight * 0.10f; 
            spacing = 45.0f; 
        }

        float panelWidth = windowWidth * 0.85f; 
        float panelHeight = windowHeight * 0.85f;
        
        ImGui::SetCursorPosX((windowWidth - panelWidth) * 0.5f);
        ImGui::SetCursorPosY((windowHeight - panelHeight) * 0.5f);

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 0.85f)); 
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 8.0f)); 

        float originalScale = ImGui::GetIO().FontGlobalScale;
        ImGui::GetIO().FontGlobalScale = currentScale;

        ImGui::BeginChild("SetupPanel", ImVec2(panelWidth, panelHeight), true);

        ImGui::Dummy(ImVec2(0.0f, spacing * 0.5f)); 
        ImGui::TextColored(ImVec4(0.8f, 0.1f, 0.1f, 1.0f), "VAULT INITIAL SETUP FOR: %s", currentUsername.c_str());
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0.0f, spacing * 0.5f));

        float itemWidth = panelWidth * 0.8f;
        
        // --- 1. DYNAMIC SYSTEM INPUT & VALIDATION ---
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(Note: Retro systems are at least 15 years removed. E.g., 2011 or older)");
        ImGui::Text("Add a System:");
        
        ImGui::PushItemWidth(itemWidth * 0.6f);
        ImGui::InputText("##newSystem", newSystemInput, IM_ARRAYSIZE(newSystemInput));
        ImGui::PopItemWidth();
        
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 0.8f)); 
        if (ImGui::Button("Add System", ImVec2(0, 0)) && strlen(newSystemInput) > 0) {
            std::string inputStr(newSystemInput);
            
            if (IsModernConsole(inputStr)) {
                systemErrorMsg = "'" + inputStr + "' was released after 2011 and is not considered retro!";
            } else {
                systemErrorMsg = ""; 
                SystemRecord newSys;
                newSys.name = inputStr;
                userSystems.push_back(newSys);
                newSystemInput[0] = '\0'; 
                recommendationsGenerated = false; // Reset generation if they add a new system
            }
        }
        ImGui::PopStyleColor();

        if (!systemErrorMsg.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", systemErrorMsg.c_str());
        }

        ImGui::Dummy(ImVec2(0.0f, spacing * 0.5f));
        ImGui::Separator();

        // --- 2. DYNAMIC GENERATION LOOP ---
        bool allPlaythroughsFilled = true; 

        ImGui::PushItemWidth(itemWidth);
        for (size_t i = 0; i < userSystems.size(); ++i) {
            ImGui::PushID(i); 
            
            ImGui::Dummy(ImVec2(0.0f, spacing * 0.2f));
            ImGui::TextColored(ImVec4(0.8f, 0.1f, 0.1f, 1.0f), "SYSTEM: %s", userSystems[i].name.c_str());
            
            ImGui::Text("Game/ROM Count:");
            if (ImGui::InputInt("##romcount", &userSystems[i].romCount)) {
                if (userSystems[i].romCount < 0) userSystems[i].romCount = 0; 
            }
            
            ImGui::Text("Current Playthrough(s) [%s] (Separate with commas):", userSystems[i].name.c_str());
            if (ImGui::InputText("##playthrough", userSystems[i].playthrough, IM_ARRAYSIZE(userSystems[i].playthrough))) {
                recommendationsGenerated = false; // Reset if they edit their playthroughs
            }
            
            if (strlen(userSystems[i].playthrough) == 0) {
                allPlaythroughsFilled = false;
            }

            ImGui::PopID();
            ImGui::Dummy(ImVec2(0.0f, spacing * 0.5f));
            ImGui::Separator();
        }

        // --- 3. GOALS ---
        ImGui::Dummy(ImVec2(0.0f, spacing * 0.5f));
        ImGui::Text("Current Goals:");
        if (ImGui::InputTextMultiline("##currgoals", currentGoals, IM_ARRAYSIZE(currentGoals), ImVec2(itemWidth, btnHeight * 2.5f))) {
            recommendationsGenerated = false;
        }
        ImGui::Dummy(ImVec2(0.0f, spacing * 0.2f));

        ImGui::Text("Yearly Goal:");
        if (ImGui::InputText("##yearlygoal", yearlyGoal, IM_ARRAYSIZE(yearlyGoal))) {
            recommendationsGenerated = false;
        }
        ImGui::Dummy(ImVec2(0.0f, spacing * 0.5f));
        ImGui::Separator();

        // --- 4. AUTO-GENERATED RECOMMENDATIONS (GATED BY BUTTON) ---
        ImGui::Dummy(ImVec2(0.0f, spacing * 0.5f));
        ImGui::TextColored(ImVec4(0.8f, 0.1f, 0.1f, 1.0f), "SYSTEM RECOMMENDATIONS:");
        
        bool isFormComplete = (!userSystems.empty() && allPlaythroughsFilled && strlen(currentGoals) > 0 && strlen(yearlyGoal) > 0);

        if (!recommendationsGenerated) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            ImGui::TextWrapped("Please add at least one system, fill out its playthrough(s), and complete both goal sections below to generate your customized strategy.");
            ImGui::PopStyleColor();
            
            ImGui::Dummy(ImVec2(0.0f, spacing * 0.5f));
            
            // --- NEW: GENERATE STRATEGY BUTTON ---
            ImGui::SetCursorPosX((panelWidth - (btnWidth * 0.6f)) * 0.5f);
            if (ImGui::Button("Generate Strategy", ImVec2(btnWidth * 0.6f, btnHeight * 0.6f))) {
                if (isFormComplete) {
                    recommendationsGenerated = true;
                    generationErrorMsg = "";
                } else {
                    generationErrorMsg = "Error: Please fill out all systems, playthroughs, and goals before generating.";
                }
            }
            
            if (!generationErrorMsg.empty()) {
                ImGui::SetCursorPosX((panelWidth - ImGui::CalcTextSize(generationErrorMsg.c_str()).x) * 0.5f);
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", generationErrorMsg.c_str());
            }

        } 
        else {
            ImGui::TextWrapped("1. Playthrough Strategy:");
            for (const auto& sys : userSystems) {
                std::vector<std::string> games = SplitGames(sys.playthrough);
                for (const auto& game : games) {
                    std::string tip = "";
                    if (game.length() % 3 == 0) tip = "Focus on completing the main campaign first before getting distracted by side content.";
                    else if (game.length() % 3 == 1) tip = "Dedicate shorter, 45-minute focused sessions to master the mechanics without burnout.";
                    else tip = "Keep a quick physical log of your progress or where you left off to maintain momentum.";
                    
                    ImGui::TextWrapped("  - For %s [%s]: %s", game.c_str(), sys.name.c_str(), tip.c_str());
                }
            }
            ImGui::Dummy(ImVec2(0.0f, spacing * 0.2f));

            std::string rec2 = "After looking through your current and yearly goals, I believe to effectively reach these goals, you should treat your current goals as stepping stones. By establishing a consistent weekend routine targeting your immediate goals, you will effectively snowball your progress toward completing your larger yearly objective.";
            ImGui::TextWrapped("2. Goal Strategy: %s", rec2.c_str());
        }

        ImGui::PopItemWidth();
        ImGui::Dummy(ImVec2(0.0f, spacing));

        // --- 5. SAVE BUTTON ---
        ImGui::SetCursorPosX((panelWidth - btnWidth) * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 0.8f));        
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.2f, 0.2f, 0.9f)); 
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));  

        if (ImGui::Button("Save Profile & Enter Vault", ImVec2(btnWidth, btnHeight))) {
            std::ofstream outfile("profiles.csv", std::ios::app);
            if (outfile.is_open()) {
                std::string compiledSystems = "";
                for (size_t i = 0; i < userSystems.size(); ++i) {
                    compiledSystems += userSystems[i].name + "|" + std::to_string(userSystems[i].romCount) + "|" + std::string(userSystems[i].playthrough);
                    if (i < userSystems.size() - 1) compiledSystems += "^"; 
                }
                
                outfile << currentUsername << ","
                        << compiledSystems << ","
                        << currentGoals << ","
                        << yearlyGoal << "\n";
                outfile.close();
                
                userSystems.clear();
                currentGoals[0] = '\0';
                yearlyGoal[0] = '\0';
                systemErrorMsg = "";
                generationErrorMsg = "";
                recommendationsGenerated = false; // Reset the button state on save
            }
        }
        ImGui::PopStyleColor(3);

        ImGui::GetIO().FontGlobalScale = originalScale; 
        ImGui::PopStyleVar(2); 
        ImGui::PopStyleColor();
        ImGui::EndChild();
    }
}