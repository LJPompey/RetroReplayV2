#pragma once
#include "imgui.h"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdlib>

namespace VaultDashboard {

    struct SystemData {
        std::string name;
        int romCount;
        std::string playthrough;
    };

    struct UserProfile {
        std::string username;
        std::vector<SystemData> systems;
        std::string currentGoals;
        std::string yearlyGoal;
        std::string savedStrategy; 
    };

    UserProfile activeUser;
    bool isProfileLoaded = false;
    int activeTab = 0; 

    // Used to clear out the user's data when they log out
    void ClearSession() {
        isProfileLoaded = false;
        activeUser = UserProfile();
        activeTab = 0;
    }

    void LoadUserData(const std::string& targetUsername) {
        std::ifstream file("profiles.csv");
        std::string line;
        
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string savedUser, systemsBlob, currGoals, yrGoals;
            
            std::getline(ss, savedUser, '~');
            if (savedUser == targetUsername) {
                activeUser.username = savedUser;
                
                std::getline(ss, systemsBlob, '~');
                std::getline(ss, currGoals, '~');
                std::getline(ss, yrGoals, '~');
                
                activeUser.currentGoals = currGoals;
                activeUser.yearlyGoal = yrGoals;
                
                activeUser.systems.clear();
                std::stringstream blobStream(systemsBlob);
                std::string sysEntry;
                while (std::getline(blobStream, sysEntry, '^')) {
                    std::stringstream entryStream(sysEntry);
                    std::string name, countStr, games;
                    
                    std::getline(entryStream, name, '|');
                    std::getline(entryStream, countStr, '|');
                    std::getline(entryStream, games, '|');
                    
                    SystemData sd;
                    sd.name = name;
                    
                    try {
                        sd.romCount = countStr.empty() ? 0 : std::stoi(countStr);
                    } catch (...) {
                        sd.romCount = 0; 
                    }
                    
                    sd.playthrough = games;
                    activeUser.systems.push_back(sd);
                }
                
                std::ifstream stratFile("strategy_" + targetUsername + ".txt");
                if (stratFile.is_open()) {
                    std::stringstream buffer;
                    buffer << stratFile.rdbuf();
                    activeUser.savedStrategy = buffer.str();
                    stratFile.close();
                } else {
                    activeUser.savedStrategy = "No saved strategy found. Please generate one in the Setup Phase.";
                }

                isProfileLoaded = true;
                break;
            }
        }
    }

    // Now returns a bool: TRUE if the user clicked Logout
    bool Render(float windowWidth, float windowHeight, unsigned int bgTexture, const std::string& currentUsername) {
        if (!isProfileLoaded) {
            LoadUserData(currentUsername);
        }

        // --- NEW: DYNAMIC SCALING FOR DASHBOARD ---
        float currentScale = (windowHeight < 900.0f) ? 1.5f : 2.5f;
        float originalScale = ImGui::GetIO().FontGlobalScale;
        ImGui::GetIO().FontGlobalScale = currentScale;

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        
        if (bgTexture) {
            ImGui::GetWindowDrawList()->AddImage(
                (void*)(intptr_t)bgTexture, 
                viewport->WorkPos, 
                ImVec2(viewport->WorkPos.x + viewport->WorkSize.x, viewport->WorkPos.y + viewport->WorkSize.y),
                ImVec2(0, 0), ImVec2(1, 1), 
                ImColor(80, 80, 80, 255) 
            );
        }

        float sidebarWidth = windowWidth * 0.18f; 
        float contentWidth = windowWidth - sidebarWidth;

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.08f, 0.85f)); 
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        // ==========================================
        // LEFT SIDEBAR NAVIGATION
        // ==========================================
        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::BeginChild("Sidebar", ImVec2(sidebarWidth, windowHeight), false);
        
        ImGui::Dummy(ImVec2(0.0f, 20.0f));
        ImGui::SetCursorPosX(sidebarWidth * 0.1f);
        ImGui::TextColored(ImVec4(0.8f, 0.1f, 0.1f, 1.0f), "RETRO REPLAY");
        ImGui::SetCursorPosX(sidebarWidth * 0.1f);
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "VAULT: %s", activeUser.username.c_str());
        
        ImGui::Dummy(ImVec2(0.0f, 40.0f));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0.0f, 20.0f));

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); 
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.1f, 0.1f, 0.3f)); 
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.1f, 0.1f, 0.6f));
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.1f, 0.5f)); 

        ImVec2 btnSize = ImVec2(sidebarWidth, 50.0f * currentScale);
        if (ImGui::Button("  [ Home Page ]", btnSize)) activeTab = 0;

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        // --- NEW: LOGOUT BUTTON (Anchored to bottom of sidebar) ---
        ImGui::SetCursorPosY(windowHeight - (80.0f * currentScale));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 0.4f)); 
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
        if (ImGui::Button("  [ Logout ]", btnSize)) {
            ClearSession();
            ImGui::PopStyleColor(3);
            ImGui::EndChild();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
            ImGui::GetIO().FontGlobalScale = originalScale;
            return true; // Send Logout Signal!
        }
        ImGui::PopStyleColor(3);

        ImGui::EndChild();

        // ==========================================
        // RIGHT MAIN CONTENT AREA (THE HOME PAGE)
        // ==========================================
        ImGui::SameLine(0, 0); 
        
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.12f, 0.12f, 0.75f)); 
        ImGui::BeginChild("MainContent", ImVec2(contentWidth, windowHeight), false);
        
        ImGui::Dummy(ImVec2(40.0f, 40.0f)); 
        
        if (activeTab == 0) {
            float innerWidth = contentWidth - 80.0f; 
            
            ImGui::SetCursorPosX(40.0f);
            ImGui::TextColored(ImVec4(0.8f, 0.1f, 0.1f, 1.0f), "COMMAND CENTER");
            ImGui::SetCursorPosX(40.0f);
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0.0f, 20.0f));

            ImGui::SetCursorPosX(40.0f);
            ImGui::BeginChild("TopRow", ImVec2(innerWidth, windowHeight * 0.35f), false);
            
            ImGui::BeginChild("Playthroughs", ImVec2(innerWidth * 0.48f, 0), true);
            ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "ACTIVE PLAYTHROUGHS");
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0.0f, 10.0f));
            for (const auto& sys : activeUser.systems) {
                ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.2f, 1.0f), "[ %s ]", sys.name.c_str());
                ImGui::TextWrapped("%s", sys.playthrough.c_str());
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
            }
            ImGui::EndChild();
            
            ImGui::SameLine(0, innerWidth * 0.04f); 
            
            ImGui::BeginChild("Goals", ImVec2(innerWidth * 0.48f, 0), true);
            ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "STRATEGIC GOALS");
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0.0f, 10.0f));
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Current Objective:");
            ImGui::TextWrapped("%s", activeUser.currentGoals.c_str());
            ImGui::Dummy(ImVec2(0.0f, 15.0f));
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Yearly Goal:");
            ImGui::TextWrapped("%s", activeUser.yearlyGoal.c_str());
            ImGui::EndChild();
            
            ImGui::EndChild(); 
            
            ImGui::Dummy(ImVec2(0.0f, 20.0f));

            ImGui::SetCursorPosX(40.0f);
            ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "SAVED AI STRATEGY & WALKTHROUGHS");
            ImGui::SetCursorPosX(40.0f);
            
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.05f, 0.05f, 0.9f));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
            
            ImGui::BeginChild("SavedStrategyBox", ImVec2(innerWidth, windowHeight * 0.45f), true, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_HorizontalScrollbar);
            
            std::istringstream stream(activeUser.savedStrategy);
            std::string line;
            
            while (std::getline(stream, line)) {
                if (line.empty()) { ImGui::Dummy(ImVec2(0.0f, 5.0f)); continue; }
                
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
            ImGui::EndChild();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
        }

        ImGui::EndChild();
        ImGui::PopStyleColor(); 

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(); 
        
        // Always return Font scale to normal before exiting frame
        ImGui::GetIO().FontGlobalScale = originalScale;
        return false; 
    }
}