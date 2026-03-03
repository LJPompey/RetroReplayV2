#pragma once
#include "imgui.h"
#include <string>
#include <fstream> 
#include <vector>
#include <sstream>
#include <algorithm>
#include <thread>
#include <atomic>
#include <mutex>
#include <cstdlib> 
#include "APIClient.h"

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

    bool recommendationsGenerated = false;
    std::string generationErrorMsg = ""; 

    std::atomic<bool> isGenerating{false}; 
    std::mutex strategyMutex;              
    std::string finalStrategyText = "";    

    bool IsModernConsole(std::string consoleInput) {
        std::transform(consoleInput.begin(), consoleInput.end(), consoleInput.begin(), ::tolower);
        consoleInput.erase(std::remove(consoleInput.begin(), consoleInput.end(), ' '), consoleInput.end());

        std::vector<std::string> modernConsoles = {
            "ps4", "playstation4", "ps5", "playstation5",
            "xboxone", "xboxseriesx", "xboxseriess",
            "wiiu", "nintendoswitch", "switch", "steamdeck", 
            "rogally", "nintendoswitch2", "switch2", "nintendoswitchlite",
            "nintendoswitcholed", "switchlite", "switcholed", "nintendowiiu",
            "lenovolegiongo", "asusrogally", "lenovolegiongos", "msiclawa1m", 
            "razeredge", "amazonluna", "gefrocenow", "googlestadia",
            "applevisionpro", "htcvivexrelite", "quest2", "metaquest3", 
            "metaquest3s", "metaquestpro", "pico4", "playsattionvr",
            "playstationvr2", "oculusrift", "nintendo3dsxl", "nintendo2ds",
            "newnintendo2dsxl", "newnintendo3ds", "newnitendo3dsxl", "playstationvita2000",
            "new3dsxl", "3dsxl", "2ds", "new2dsxl",
            "new3ds", "vita", "psvita", "steamdecklcd", 
            "lcdsteamdeck", "steamdecklcd", "oledsteamdeck", "aynthor", 
            "rogallyx", "xboxrogally" 
        };

        for (const auto& modern : modernConsoles) {
            if (consoleInput == modern) return true;
        }
        return false;
    }

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

    void FetchStrategiesFromCloud(std::vector<SystemRecord> systems, std::string currGoals, std::string yrGoals) {
        std::string compiledGames = "";
        for (const auto& sys : systems) {
            std::vector<std::string> games = SplitGames(sys.playthrough);
            for (const auto& game : games) {
                compiledGames += game + " (" + sys.name + "), ";
            }
        }

        std::string aiResponse = APIClient::FetchAIStrategy(compiledGames, currGoals, yrGoals);

        std::string youtubeResponse = "\n3. Recommended Walkthroughs:\n";
        bool foundAnyVideo = false;
        
        for (const auto& sys : systems) {
            std::vector<std::string> games = SplitGames(sys.playthrough);
            for (const auto& game : games) {
                std::string vid = APIClient::FetchYouTubeWalkthrough(game, sys.name);
                if (!vid.empty()) {
                    youtubeResponse += vid;
                    foundAnyVideo = true;
                }
            }
        }

        std::string finalOutput = aiResponse;
        if (foundAnyVideo) {
            finalOutput += youtubeResponse;
        }

        std::lock_guard<std::mutex> lock(strategyMutex);
        finalStrategyText = finalOutput;
        isGenerating = false; 
    }

    // Now returns an INT: 0 = Stay, 1 = Go to Vault, 2 = Logout
    int Render(float windowWidth, float windowHeight, unsigned int bgTexture, const std::string& currentUsername) {
        
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        if (bgTexture) {
            ImGui::GetWindowDrawList()->AddImage(
                (void*)(intptr_t)bgTexture, 
                viewport->WorkPos, 
                ImVec2(viewport->WorkPos.x + viewport->WorkSize.x, viewport->WorkPos.y + viewport->WorkSize.y)
            );
        }

        float currentScale, btnWidth, btnHeight, spacing;
        int actionState = 0; // 0 = Do nothing

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

        float originalScale = ImGui::GetIO().FontGlobalScale;
        ImGui::GetIO().FontGlobalScale = currentScale;

        // --- NEW: TOP-LEFT LOGOUT BUTTON ---
        ImGui::SetCursorPos(ImVec2(30.0f, 30.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 0.8f));
        if (ImGui::Button("<- Logout", ImVec2(btnWidth * 0.4f, btnHeight * 0.6f))) {
            actionState = 2; // Signal main.cpp to log out
        }
        ImGui::PopStyleColor();

        float panelWidth = windowWidth * 0.85f; 
        float panelHeight = windowHeight * 0.85f;
        
        ImGui::SetCursorPosX((windowWidth - panelWidth) * 0.5f);
        ImGui::SetCursorPosY((windowHeight - panelHeight) * 0.5f);

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 0.85f)); 
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 8.0f)); 

        ImGui::BeginChild("SetupPanel", ImVec2(panelWidth, panelHeight), true);

        ImGui::Dummy(ImVec2(0.0f, spacing * 0.5f)); 
        ImGui::TextColored(ImVec4(0.8f, 0.1f, 0.1f, 1.0f), "VAULT INITIAL SETUP FOR: %s", currentUsername.c_str());
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0.0f, spacing * 0.5f));

        float itemWidth = panelWidth * 0.8f;
        
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
                recommendationsGenerated = false; 
            }
        }
        ImGui::PopStyleColor();

        if (!systemErrorMsg.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", systemErrorMsg.c_str());
        }

        ImGui::Dummy(ImVec2(0.0f, spacing * 0.5f));
        ImGui::Separator();

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
                recommendationsGenerated = false; 
            }
            
            if (strlen(userSystems[i].playthrough) == 0) {
                allPlaythroughsFilled = false;
            }

            ImGui::PopID();
            ImGui::Dummy(ImVec2(0.0f, spacing * 0.5f));
            ImGui::Separator();
        }

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

        ImGui::Dummy(ImVec2(0.0f, spacing * 0.5f));
        ImGui::TextColored(ImVec4(0.8f, 0.1f, 0.1f, 1.0f), "SYSTEM RECOMMENDATIONS:");
        
        bool isFormComplete = (!userSystems.empty() && allPlaythroughsFilled && strlen(currentGoals) > 0 && strlen(yearlyGoal) > 0);

        if (!recommendationsGenerated) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            ImGui::TextWrapped("Please add at least one system, fill out its playthrough(s), and complete both goal sections below to generate your customized strategy.");
            ImGui::PopStyleColor();
            
            ImGui::Dummy(ImVec2(0.0f, spacing * 0.5f));
            
            ImGui::SetCursorPosX((panelWidth - (btnWidth * 0.6f)) * 0.5f);
            
            if (isGenerating) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
                ImGui::Button("Analyzing data...", ImVec2(btnWidth * 0.6f, btnHeight * 0.6f));
                ImGui::PopStyleColor();
            } 
            else if (ImGui::Button("Generate AI Strategy", ImVec2(btnWidth * 0.6f, btnHeight * 0.6f))) {
                if (isFormComplete) {
                    recommendationsGenerated = true;
                    generationErrorMsg = "";
                    isGenerating = true; 
                    
                    std::lock_guard<std::mutex> lock(strategyMutex);
                    finalStrategyText = "Booting AI link... Please wait a few seconds while strategies are compiled.";
                    
                    std::thread(FetchStrategiesFromCloud, userSystems, std::string(currentGoals), std::string(yearlyGoal)).detach();
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
            std::lock_guard<std::mutex> lock(strategyMutex);
            
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.05f, 0.05f, 0.7f));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.4f, 0.4f, 0.4f, 0.5f));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
            
            ImGui::BeginChild("StrategyResultsBox", ImVec2(itemWidth, panelHeight * 0.45f), true, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_HorizontalScrollbar);
            
            if (isGenerating) {
                ImGui::Dummy(ImVec2(0.0f, spacing));
                ImGui::SetCursorPosX((itemWidth - ImGui::CalcTextSize("Booting Secure AI Link...").x) * 0.5f);
                ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.8f, 1.0f), "Booting Secure AI Link...");
            } 
            else {
                std::istringstream stream(finalStrategyText);
                std::string line;
                
                while (std::getline(stream, line)) {
                    if (line.empty()) {
                        ImGui::Dummy(ImVec2(0.0f, 5.0f));
                        continue;
                    }
                    
                    if (line.find("1. Playthrough") == 0 || line.find("2. Goal") == 0 || line.find("3. Recommended") == 0) {
                        ImGui::Dummy(ImVec2(0.0f, 5.0f));
                        ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.2f, 1.0f), "%s", line.c_str()); 
                        ImGui::Separator();
                        ImGui::Dummy(ImVec2(0.0f, 5.0f));
                    }
                    else if (line.find("- For") == 0 || line.find("  - For") == 0) {
                        ImGui::Bullet();
                        
                        size_t bracketStart = line.find("[");
                        size_t bracketEnd = line.find("]");
                        
                        if (bracketStart != std::string::npos && bracketEnd != std::string::npos) {
                            std::string textPart = line.substr(0, bracketStart);
                            std::string linkData = line.substr(bracketStart + 1, bracketEnd - bracketStart - 1);
                            
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                            ImGui::Text("%s", textPart.c_str());
                            ImGui::PopStyleColor();
                            
                            ImGui::SameLine();
                            
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 0.8f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.2f, 0.2f, 0.9f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
                            
                            std::string btnLabel = "▶ Watch##" + linkData; 
                            
                            if (ImGui::Button(btnLabel.c_str())) {
                                std::string url = (linkData.find("http") == 0) ? linkData : "https://www.youtube.com/watch?v=" + linkData;
                                
                                #if defined(_WIN32)
                                    system(("start \"\" \"" + url + "\"").c_str());
                                #elif defined(__APPLE__)
                                    system(("open \"" + url + "\"").c_str());
                                #else
                                    system(("xdg-open \"" + url + "\"").c_str());
                                #endif
                            }
                            ImGui::PopStyleColor(3);
                        } else {
                            ImGui::TextWrapped("%s", line.c_str());
                        }
                    }
                    else {
                        ImGui::TextWrapped("%s", line.c_str());
                    }
                }
            }
            ImGui::EndChild();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(2);
        }

        ImGui::PopItemWidth();
        ImGui::Dummy(ImVec2(0.0f, spacing));

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
                
                outfile << currentUsername << "~"
                        << compiledSystems << "~"
                        << currentGoals << "~"
                        << yearlyGoal << "\n";
                outfile.close();

                std::ofstream stratFile("strategy_" + currentUsername + ".txt");
                if (stratFile.is_open()) {
                    stratFile << finalStrategyText;
                    stratFile.close();
                }
                
                userSystems.clear();
                currentGoals[0] = '\0';
                yearlyGoal[0] = '\0';
                systemErrorMsg = "";
                generationErrorMsg = "";
                recommendationsGenerated = false; 
                
                actionState = 1; // Signal main.cpp to enter Vault
            }
        }
        ImGui::PopStyleColor(3);

        ImGui::PopStyleVar(2); 
        ImGui::PopStyleColor();
        ImGui::EndChild(); 
        
        // Reset scale before returning
        ImGui::GetIO().FontGlobalScale = originalScale; 
        
        return actionState; 
    }
}