#pragma once
#include "imgui.h"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <thread>
#include <atomic>
#include <mutex>
#include "APIClient.h" 

namespace GameManagement {

    // Structure to hold the data we are editing
    struct EditSystem {
        std::string name;
        int romCount;
        char playthrough[128];
    };

    // --- STATE VARIABLES ---
    std::vector<EditSystem> activeSystems;
    char editCurrentGoals[256] = "";
    char editYearlyGoal[256] = "";
    
    char newSystemInput[128] = ""; 
    std::string systemErrorMsg = ""; 

    bool isDataLoaded = false;
    bool isDirty = false; 
    
    std::atomic<bool> isReGenerating{false};
    std::string reGenStatusMsg = "";
    std::mutex managementMutex;

    // --- ENFORCE RETRO RULE ---
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

    // --- SAVE UPDATED DATA TO CSV ---
    void SaveUpdatedProfile(const std::string& username) {
        std::ifstream file("profiles.csv");
        std::vector<std::string> lines;
        std::string line;

        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string savedUser;
            std::getline(ss, savedUser, '~');
            
            if (savedUser == username) {
                std::string systemsBlob = "";
                for (size_t i = 0; i < activeSystems.size(); ++i) {
                    systemsBlob += activeSystems[i].name + "|" + std::to_string(activeSystems[i].romCount) + "|" + std::string(activeSystems[i].playthrough);
                    if (i < activeSystems.size() - 1) systemsBlob += "^";
                }
                lines.push_back(username + "~" + systemsBlob + "~" + std::string(editCurrentGoals) + "~" + std::string(editYearlyGoal));
            } else {
                lines.push_back(line);
            }
        }
        file.close();

        std::ofstream outfile("profiles.csv", std::ios::trunc);
        for (const auto& l : lines) outfile << l << "\n";
        outfile.close();
        
        isDirty = false; 
    }

    // --- FETCH NEW STRATEGY ---
    void RunNewStrategyGen(std::string username) {
        std::string compiledGames = "";
        for (const auto& sys : activeSystems) {
            compiledGames += std::string(sys.playthrough) + " (" + sys.name + "), ";
        }

        std::string aiResponse = APIClient::FetchAIStrategy(compiledGames, editCurrentGoals, editYearlyGoal);
        
        std::string youtubePart = "\n3. Recommended Walkthroughs:\n";
        for (const auto& sys : activeSystems) {
            std::string vid = APIClient::FetchYouTubeWalkthrough(std::string(sys.playthrough), sys.name);
            if (!vid.empty()) youtubePart += vid;
        }

        std::lock_guard<std::mutex> lock(managementMutex);
        std::ofstream stratFile("strategy_" + username + ".txt");
        if (stratFile.is_open()) {
            stratFile << aiResponse << youtubePart;
            stratFile.close();
        }
        
        isReGenerating = false;
        reGenStatusMsg = "New Strategy Compiled & Saved!";
    }

    // --- MAIN RENDER ---
    void Render(float windowWidth, float windowHeight, unsigned int bgTexture, const std::string& username) {
        
        if (!isDataLoaded) {
            std::ifstream file("profiles.csv");
            std::string line;
            while (std::getline(file, line)) {
                std::stringstream ss(line);
                std::string u, blob, cur, yr;
                std::getline(ss, u, '~');
                if (u == username) {
                    std::getline(ss, blob, '~');
                    std::getline(ss, cur, '~');
                    std::getline(ss, yr, '~');
                    
                    #ifdef _WIN32
                        strcpy_s(editCurrentGoals, sizeof(editCurrentGoals), cur.c_str());
                        strcpy_s(editYearlyGoal, sizeof(editYearlyGoal), yr.c_str());
                    #else
                        strncpy(editCurrentGoals, cur.c_str(), sizeof(editCurrentGoals));
                        strncpy(editYearlyGoal, yr.c_str(), sizeof(editYearlyGoal));
                    #endif

                    activeSystems.clear(); 
                    newSystemInput[0] = '\0';
                    systemErrorMsg = "";

                    std::stringstream bs(blob);
                    std::string entry;
                    while (std::getline(bs, entry, '^')) {
                        std::stringstream es(entry);
                        std::string n, c, p;
                        std::getline(es, n, '|');
                        std::getline(es, c, '|');
                        std::getline(es, p, '|');
                        
                        if (!n.empty()) {
                            EditSystem sd;
                            sd.name = n;
                            try { sd.romCount = std::stoi(c); } catch(...) { sd.romCount = 0; }
                            #ifdef _WIN32
                                strcpy_s(sd.playthrough, sizeof(sd.playthrough), p.c_str());
                            #else
                                strncpy(sd.playthrough, p.c_str(), sizeof(sd.playthrough));
                            #endif
                            activeSystems.push_back(sd);
                        }
                    }
                    isDataLoaded = true;
                    break;
                }
            }
        }

        float currentScale = (windowHeight < 900.0f) ? 1.5f : 2.5f;
        float originalScale = ImGui::GetIO().FontGlobalScale;
        ImGui::GetIO().FontGlobalScale = currentScale;

        if (bgTexture) {
            ImGui::GetWindowDrawList()->AddImage(
                (void*)(intptr_t)bgTexture, 
                ImVec2(0,0), 
                ImVec2(windowWidth, windowHeight), 
                ImVec2(0,0), ImVec2(1,1), 
                ImColor(80, 80, 80, 255)
            );
        }

        ImGui::SetCursorPos(ImVec2(windowWidth * 0.05f, windowHeight * 0.05f));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 0.85f)); 
        ImGui::BeginChild("ManagementPanel", ImVec2(windowWidth * 0.9f, windowHeight * 0.9f), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

        ImGui::TextColored(ImVec4(0.8f, 0.1f, 0.1f, 1.0f), "GAME MANAGEMENT & DATA UPDATE");
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 10.0f));

        // --- SYSTEM UI ---
        ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "ADD NEW SYSTEM");
        ImGui::PushItemWidth(300.0f * (currentScale/2.0f));
        ImGui::InputText("##AddNewSystem", newSystemInput, IM_ARRAYSIZE(newSystemInput));
        ImGui::PopItemWidth();
        
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
        if (ImGui::Button("Add System") && strlen(newSystemInput) > 0) {
            std::string inputStr(newSystemInput);
            if (IsModernConsole(inputStr)) {
                systemErrorMsg = "'" + inputStr + "' is not considered retro!";
            } else {
                systemErrorMsg = ""; 
                EditSystem newSys;
                newSys.name = inputStr;
                newSys.romCount = 0;
                newSys.playthrough[0] = '\0';
                activeSystems.push_back(newSys);
                newSystemInput[0] = '\0'; 
                isDirty = true; // Flag the save button!
            }
        }
        ImGui::PopStyleColor();

        if (!systemErrorMsg.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", systemErrorMsg.c_str());
        }

        ImGui::Dummy(ImVec2(0, 20.0f));

        // Update Existing Systems Section
        ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "SYSTEM INFORMATION");
        
        int indexToDelete = -1; // Tracker for safe deletion

        for (int i = 0; i < activeSystems.size(); i++) {
            ImGui::PushID(i);
            
            ImGui::Text("System: %s", activeSystems[i].name.c_str());
            ImGui::SameLine();
            
            // --- REMOVE SYSTEM BUTTON ---
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
            if (ImGui::Button("Remove")) {
                indexToDelete = i;
            }
            ImGui::PopStyleColor();

            if (ImGui::InputInt("ROM Count", &activeSystems[i].romCount)) isDirty = true;
            if (ImGui::InputText("Active Playthrough", activeSystems[i].playthrough, 128)) isDirty = true;
            
            ImGui::PopID();
            ImGui::Separator();
        }

        // Safely execute the deletion outside the loop
        if (indexToDelete != -1) {
            activeSystems.erase(activeSystems.begin() + indexToDelete);
            isDirty = true;
        }

        ImGui::Dummy(ImVec2(0, 20));

        // Update Goals Section
        ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "STRATEGIC GOALS");
        if (ImGui::InputTextMultiline("Current Objective", editCurrentGoals, 256, ImVec2(0, 100))) isDirty = true;
        if (ImGui::InputText("Yearly Goal", editYearlyGoal, 256)) isDirty = true;

        ImGui::Dummy(ImVec2(0, 30));

        // Action Buttons
        if (isDirty) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
            if (ImGui::Button("Save Only (No Update)", ImVec2(450 * (currentScale/2.5f), 70 * (currentScale/2.5f)))) {
                SaveUpdatedProfile(username);
                reGenStatusMsg = "Data saved locally.";
            }
            ImGui::PopStyleColor();
            
            ImGui::SameLine(0, 20.0f);

            if (isReGenerating) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
                ImGui::Button("System is analyzing...", ImVec2(450 * (currentScale/2.5f), 70 * (currentScale/2.5f)));
                ImGui::PopStyleColor();
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 0.8f));
                if (ImGui::Button("Save & Regenerate Strategy", ImVec2(450 * (currentScale/2.5f), 70 * (currentScale/2.5f)))) {
                    SaveUpdatedProfile(username); 
                    isReGenerating = true;
                    reGenStatusMsg = "Contacting Groq AI and YouTube APIs...";
                    std::thread(RunNewStrategyGen, username).detach(); 
                }
                ImGui::PopStyleColor();
            }
        }
        
        if (!reGenStatusMsg.empty()) {
            ImGui::Dummy(ImVec2(0, 10));
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "%s", reGenStatusMsg.c_str());
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::GetIO().FontGlobalScale = originalScale; 
    }
}