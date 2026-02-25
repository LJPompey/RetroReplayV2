#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream> 
#include <string>  
#include <cctype>  
#include <ctime>   
#include <sstream> 
#include <vector> 
#include "InitialSetup.h" // Includes our new Setup form!

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// ----------------------------------------------------
// STATE MACHINE SETUP
// ----------------------------------------------------
enum class AppState {
    WelcomeScreen,
    GetStarted,
    WelcomeBack,
    ForgotPassword,
    InitialSetup
};

AppState currentState = AppState::WelcomeScreen;

// Buffers for Registration
char regUsername[128] = "";
char regPassword[128] = "";
bool showConfirmation = false;
std::string registrationError = ""; 

// Buffers for Login
char loginUsername[128] = "";
char loginPassword[128] = "";
std::string loginError = "";
std::string loginSuccessMsg = "";

// Buffers for Password Reset
char resetUsername[128] = "";
char resetPassword[128] = "";
std::string resetError = "";
std::string resetSuccessMsg = "";

// --- NEW: TRACKS THE LOGGED-IN USER ---
std::string activeUser = ""; 

// Helper function to load an image
bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height) {
    int image_width = 0, image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL) return false;

    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_texture = image_texture;
    *out_width = image_width;
    *out_height = image_height;
    return true;
}

// ----------------------------------------------------
// VALIDATION & SECURITY FUNCTIONS
// ----------------------------------------------------
bool IsValidUsername(const std::string& uname) {
    if (uname.empty() || uname.length() > 12) return false;
    if (uname.find(',') != std::string::npos) return false; 
    return true;
}

bool IsValidPassword(const std::string& pass) {
    if (pass.length() < 10) return false;
    if (pass.find(',') != std::string::npos) return false; 

    bool hasUpper = false, hasDigit = false, hasSpecial = false;
    for (char c : pass) {
        if (std::isupper(c)) hasUpper = true;
        if (std::isdigit(c)) hasDigit = true;
        if (std::ispunct(c)) hasSpecial = true;
    }
    return hasUpper && hasDigit && hasSpecial;
}

unsigned long HashPassword(const std::string& str) {
    unsigned long hash = 5381;
    for (char c : str) {
        hash = ((hash << 5) + hash) + c; 
    }
    return hash;
}

bool UserExists(const std::string& targetUser) {
    std::ifstream file("credentials.txt");
    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string uName;
        std::getline(ss, uName, ','); 
        if (uName == targetUser) return true;
    }
    return false;
}

bool ValidateLogin(const std::string& targetUser, const std::string& targetPass) {
    unsigned long targetHash = HashPassword(targetPass);
    std::string hashStrTarget = std::to_string(targetHash);
    
    std::ifstream file("credentials.txt");
    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string uName, hashStr;
        std::getline(ss, uName, ',');
        std::getline(ss, hashStr, ',');
        
        if (uName == targetUser && hashStr == hashStrTarget) {
            return true;
        }
    }
    return false;
}

bool UpdatePassword(const std::string& targetUser, const std::string& newPass) {
    std::vector<std::string> lines;
    std::ifstream infile("credentials.txt");
    bool userFound = false;

    if (infile.is_open()) {
        std::string line;
        while (std::getline(infile, line)) {
            std::stringstream ss(line);
            std::string uName;
            std::getline(ss, uName, ',');
            
            if (uName == targetUser) {
                unsigned long newHashedPass = HashPassword(newPass);
                std::time_t timestamp = std::time(nullptr);
                lines.push_back(uName + "," + std::to_string(newHashedPass) + "," + std::to_string(timestamp));
                userFound = true;
            } else {
                lines.push_back(line);
            }
        }
        infile.close();
    }

    if (!userFound) return false;

    std::ofstream outfile("credentials.txt", std::ios::trunc); 
    if (outfile.is_open()) {
        for (const auto& l : lines) {
            outfile << l << "\n";
        }
        outfile.close();
        return true;
    }
    return false;
}

int main() {
    if (!glfwInit()) return -1;

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Retro REPLAY V2", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); 

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    GLuint bgTexture = 0, replayLogo = 0, consoleLogo = 0;
    int bgW, bgH, repW, repH, conW, conH;
    LoadTextureFromFile("assets/background_icons.jpg", &bgTexture, &bgW, &bgH);
    LoadTextureFromFile("assets/logo_retroreplay.png", &replayLogo, &repW, &repH);
    LoadTextureFromFile("assets/logo_consoles.png", &consoleLogo, &conW, &conH);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | 
                                        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
                                        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | 
                                        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | 
                                        ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Welcome Screen", nullptr, window_flags);
        ImGui::PopStyleVar();

        if (bgTexture) ImGui::GetWindowDrawList()->AddImage((void*)(intptr_t)bgTexture, viewport->WorkPos, ImVec2(viewport->WorkPos.x + viewport->WorkSize.x, viewport->WorkPos.y + viewport->WorkSize.y));
        if (consoleLogo) ImGui::GetWindowDrawList()->AddImage((void*)(intptr_t)consoleLogo, viewport->WorkPos, ImVec2(viewport->WorkPos.x + viewport->WorkSize.x, viewport->WorkPos.y + viewport->WorkSize.y));

        float windowHeight = viewport->WorkSize.y;
        float trueWindowWidth = ImGui::GetWindowWidth();

        float scaledRepW = trueWindowWidth * 0.45f; 
        float scaledRepH = scaledRepW * ((float)repH / (float)repW);
        
        float logoY, btnWidth, btnHeight, buttonStartY, spacing, guideOffset, errorOffset;

        if (windowHeight < 900.0f) {
            ImGui::SetWindowFontScale(1.5f); 
            logoY = windowHeight * 0.20f;              
            btnWidth = 400.0f;                         
            btnHeight = 75.0f;                         
            buttonStartY = logoY + scaledRepH + -75.0f; 
            spacing = 25.0f;                           
            
            guideOffset = btnHeight * 0.4f;
            errorOffset = btnHeight * 2.5f; 
        } 
        else {
            ImGui::SetWindowFontScale(2.5f); 
            logoY = windowHeight * 0.25f;       
            btnWidth = trueWindowWidth * 0.25f; 
            btnHeight = windowHeight * 0.08f;   
            buttonStartY = logoY + scaledRepH + -145.0f; 
            spacing = 40.0f; 
            
            guideOffset = btnHeight * 0.4f;
            errorOffset = btnHeight * 3.0f; 
        }

        if (replayLogo) {
            ImGui::SetCursorPosX((trueWindowWidth - scaledRepW) * 0.5f);
            ImGui::SetCursorPosY(logoY);
            ImGui::Image((void*)(intptr_t)replayLogo, ImVec2(scaledRepW, scaledRepH));
        }

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 0.8f));        
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.2f, 0.2f, 0.9f)); 
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));  

        // ----------------------------------------------------
        // STATE: WELCOME SCREEN
        // ----------------------------------------------------
        if (currentState == AppState::WelcomeScreen) {
            ImGui::SetCursorPosX((trueWindowWidth - btnWidth) * 0.5f);
            ImGui::SetCursorPosY(buttonStartY);
            if (ImGui::Button("Get Started", ImVec2(btnWidth, btnHeight))) {
                registrationError = ""; 
                currentState = AppState::GetStarted; 
            }

            ImGui::SetCursorPosX((trueWindowWidth - btnWidth) * 0.5f);
            ImGui::SetCursorPosY(buttonStartY + btnHeight + spacing);
            if (ImGui::Button("Welcome Back", ImVec2(btnWidth, btnHeight))) {
                loginError = "";
                loginSuccessMsg = "";
                currentState = AppState::WelcomeBack; 
            }
        }
        // ----------------------------------------------------
        // STATE: GET STARTED (Registration)
        // ----------------------------------------------------
        else if (currentState == AppState::GetStarted) {
            if (!showConfirmation) {
                ImGui::PushItemWidth(btnWidth); 

                if (!registrationError.empty()) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f)); 
                    ImGui::SetCursorPosX((trueWindowWidth - ImGui::CalcTextSize(registrationError.c_str()).x) * 0.5f);
                    ImGui::SetCursorPosY(buttonStartY - errorOffset); 
                    ImGui::Text("%s", registrationError.c_str());
                    ImGui::PopStyleColor();
                }

                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.8f, 0.1f, 0.1f, 0.8f));       
                ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(1.0f, 0.2f, 0.2f, 0.9f));
                ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));          

                ImGui::SetCursorPosX((trueWindowWidth - btnWidth) * 0.5f);
                ImGui::SetCursorPosY(buttonStartY - guideOffset);
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Username (Max 12 characters)");

                ImGui::SetCursorPosX((trueWindowWidth - btnWidth) * 0.5f);
                ImGui::SetCursorPosY(buttonStartY);
                ImGui::InputTextWithHint("##username", "Create Username", regUsername, IM_ARRAYSIZE(regUsername));

                ImGui::SetCursorPosX((trueWindowWidth - btnWidth) * 0.5f);
                ImGui::SetCursorPosY(buttonStartY + (btnHeight * 0.8f) - guideOffset);
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Password (Min 10 chars, 1 Upper, 1 Num, 1 Spec)");

                ImGui::SetCursorPosX((trueWindowWidth - btnWidth) * 0.5f);
                ImGui::SetCursorPosY(buttonStartY + (btnHeight * 0.8f)); 
                ImGui::InputTextWithHint("##password", "Create Password", regPassword, IM_ARRAYSIZE(regPassword), ImGuiInputTextFlags_Password);
                
                ImGui::PopStyleColor(4); 
                ImGui::PopItemWidth();

                ImGui::SetCursorPosX((trueWindowWidth - btnWidth) * 0.5f);
                ImGui::SetCursorPosY(buttonStartY + (btnHeight * 1.1f) + spacing);
                if (ImGui::Button("Create Account", ImVec2(btnWidth, btnHeight))) {
                    std::string uName(regUsername);
                    std::string pWord(regPassword);

                    if (!IsValidUsername(uName)) {
                        registrationError = "Invalid Username! Must be 1-12 characters.";
                    } else if (!IsValidPassword(pWord)) {
                        registrationError = "Invalid Password! Check the requirements.";
                    } else if (UserExists(uName)) {
                        registrationError = "Username already exists in the system!";
                    } else {
                        unsigned long hashedPass = HashPassword(pWord);
                        std::time_t timestamp = std::time(nullptr);
                        
                        std::ofstream outfile("credentials.txt", std::ios::app); 
                        if (outfile.is_open()) {
                            outfile << uName << "," << hashedPass << "," << timestamp << "\n";
                            outfile.close();
                            registrationError = ""; 
                            showConfirmation = true;
                        } else {
                            registrationError = "System Error: Could not write to credentials file.";
                        }
                    }
                }

                ImGui::SetCursorPosX((trueWindowWidth - btnWidth) * 0.5f);
                ImGui::SetCursorPosY(buttonStartY + (btnHeight * 2.3f) + (spacing * 2));
                if (ImGui::Button("<- Back", ImVec2(btnWidth, btnHeight))) {
                    registrationError = ""; 
                    currentState = AppState::WelcomeScreen;
                }
            } 
            else {
                const char* successMsg = "Account saved successfully!";
                ImGui::SetCursorPosX((trueWindowWidth - ImGui::CalcTextSize(successMsg).x) * 0.5f);
                ImGui::SetCursorPosY(buttonStartY);
                ImGui::Text("%s", successMsg);

                ImGui::SetCursorPosX((trueWindowWidth - btnWidth) * 0.5f);
                ImGui::SetCursorPosY(buttonStartY + btnHeight + spacing);
                if (ImGui::Button("Continue to Initial Setup", ImVec2(btnWidth, btnHeight))) {
                    // --- FIXED: SAVE THE ACTIVE USER HERE ---
                    activeUser = std::string(regUsername);
                    
                    showConfirmation = false;
                    regUsername[0] = '\0';
                    regPassword[0] = '\0';
                    currentState = AppState::InitialSetup;
                }
            }
        }
        // ----------------------------------------------------
        // STATE: WELCOME BACK (Login)
        // ----------------------------------------------------
        else if (currentState == AppState::WelcomeBack) {
            if (loginSuccessMsg.empty()) {
                ImGui::PushItemWidth(btnWidth); 

                if (!loginError.empty()) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f)); 
                    ImGui::SetCursorPosX((trueWindowWidth - ImGui::CalcTextSize(loginError.c_str()).x) * 0.5f);
                    ImGui::SetCursorPosY(buttonStartY - errorOffset); 
                    ImGui::Text("%s", loginError.c_str());
                    ImGui::PopStyleColor();
                }

                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.8f, 0.1f, 0.1f, 0.8f));       
                ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(1.0f, 0.2f, 0.2f, 0.9f));
                ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));          

                ImGui::SetCursorPosX((trueWindowWidth - btnWidth) * 0.5f);
                ImGui::SetCursorPosY(buttonStartY - guideOffset);
                std::string currentTypedUser(loginUsername);
                if (!currentTypedUser.empty() && UserExists(currentTypedUser)) {
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Username found in system.");
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Enter Username");
                }

                ImGui::SetCursorPosX((trueWindowWidth - btnWidth) * 0.5f);
                ImGui::SetCursorPosY(buttonStartY);
                ImGui::InputTextWithHint("##loginuser", "Username", loginUsername, IM_ARRAYSIZE(loginUsername));

                ImGui::SetCursorPosX((trueWindowWidth - btnWidth) * 0.5f);
                ImGui::SetCursorPosY(buttonStartY + (btnHeight * 0.8f) - guideOffset);
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Enter Password");

                ImGui::SetCursorPosX((trueWindowWidth - btnWidth) * 0.5f);
                ImGui::SetCursorPosY(buttonStartY + (btnHeight * 0.8f)); 
                ImGui::InputTextWithHint("##loginpass", "Password", loginPassword, IM_ARRAYSIZE(loginPassword), ImGuiInputTextFlags_Password);
                
                ImGui::PopStyleColor(4); 
                ImGui::PopItemWidth();

                ImGui::SetCursorPosX((trueWindowWidth - btnWidth) * 0.5f);
                ImGui::SetCursorPosY(buttonStartY + (btnHeight * 1.1f) + spacing);
                if (ImGui::Button("Login", ImVec2(btnWidth, btnHeight))) {
                    std::string uName(loginUsername);
                    std::string pWord(loginPassword);

                    if (ValidateLogin(uName, pWord)) {
                        loginError = "";
                        loginSuccessMsg = "Login Successful! Welcome back, " + uName + ".";
                    } else {
                        loginError = "Invalid Username or Password.";
                    }
                }

                float smallBtnWidth = btnWidth * 0.48f; 
                float smallBtnHeight = btnHeight * 0.6f;
                float smallSpacing = btnWidth * 0.04f;  
                float totalGroupWidth = (smallBtnWidth * 2) + smallSpacing;
                
                float startX = (trueWindowWidth - totalGroupWidth) * 0.5f;
                float currentY = buttonStartY + (btnHeight * 2.2f) + (spacing * 1.5f);

                float currentScale = (windowHeight < 900.0f) ? 1.5f : 2.5f;
                ImGui::SetWindowFontScale(currentScale * 0.6f); 

                ImGui::SetCursorPosX(startX);
                ImGui::SetCursorPosY(currentY);
                if (ImGui::Button("Forgot Password", ImVec2(smallBtnWidth, smallBtnHeight))) {
                    resetError = "";
                    resetSuccessMsg = "";
                    resetUsername[0] = '\0';
                    resetPassword[0] = '\0';
                    currentState = AppState::ForgotPassword;
                }

                ImGui::SameLine(0.0f, smallSpacing);
                
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 0.8f)); 
                if (ImGui::Button("Forgot Username", ImVec2(smallBtnWidth, smallBtnHeight))) {
                    std::ofstream outfile("credentials.txt", std::ios::trunc); 
                    if (outfile.is_open()) outfile.close();
                    
                    loginUsername[0] = '\0';
                    loginPassword[0] = '\0';
                    regUsername[0] = '\0';
                    regPassword[0] = '\0';
                    registrationError = "";
                    currentState = AppState::GetStarted;
                }
                ImGui::PopStyleColor();

                ImGui::SetWindowFontScale(currentScale);

                ImGui::SetCursorPosX((trueWindowWidth - btnWidth) * 0.5f);
                ImGui::SetCursorPosY(currentY + smallBtnHeight + spacing);
                if (ImGui::Button("<- Back", ImVec2(btnWidth, btnHeight))) {
                    loginError = "";
                    currentState = AppState::WelcomeScreen;
                }
            }
            else {
                ImGui::SetCursorPosX((trueWindowWidth - ImGui::CalcTextSize(loginSuccessMsg.c_str()).x) * 0.5f);
                ImGui::SetCursorPosY(buttonStartY);
                ImGui::Text("%s", loginSuccessMsg.c_str());

                ImGui::SetCursorPosX((trueWindowWidth - btnWidth) * 0.5f);
                ImGui::SetCursorPosY(buttonStartY + btnHeight + spacing);
                if (ImGui::Button("Enter Vault", ImVec2(btnWidth, btnHeight))) {
                    // --- FIXED: SAVE THE ACTIVE USER HERE ---
                    activeUser = std::string(loginUsername);
                    
                    loginSuccessMsg = "";
                    loginUsername[0] = '\0';
                    loginPassword[0] = '\0';
                    currentState = AppState::InitialSetup; 
                }
            }
        }
        // ----------------------------------------------------
        // STATE: FORGOT PASSWORD (Reset logic)
        // ----------------------------------------------------
        else if (currentState == AppState::ForgotPassword) {
            
            if (resetSuccessMsg.empty()) {
                ImGui::PushItemWidth(btnWidth); 

                if (!resetError.empty()) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f)); 
                    ImGui::SetCursorPosX((trueWindowWidth - ImGui::CalcTextSize(resetError.c_str()).x) * 0.5f);
                    ImGui::SetCursorPosY(buttonStartY - errorOffset); 
                    ImGui::Text("%s", resetError.c_str());
                    ImGui::PopStyleColor();
                }

                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.8f, 0.1f, 0.1f, 0.8f));       
                ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(1.0f, 0.2f, 0.2f, 0.9f));
                ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));          

                ImGui::SetCursorPosX((trueWindowWidth - btnWidth) * 0.5f);
                ImGui::SetCursorPosY(buttonStartY - guideOffset);
                std::string currentResetUser(resetUsername);
                if (!currentResetUser.empty() && UserExists(currentResetUser)) {
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Account verified.");
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Enter Username for Reset");
                }

                ImGui::SetCursorPosX((trueWindowWidth - btnWidth) * 0.5f);
                ImGui::SetCursorPosY(buttonStartY);
                ImGui::InputTextWithHint("##resetuser", "Username", resetUsername, IM_ARRAYSIZE(resetUsername));

                ImGui::SetCursorPosX((trueWindowWidth - btnWidth) * 0.5f);
                ImGui::SetCursorPosY(buttonStartY + (btnHeight * 0.8f) - guideOffset);
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "New Password (Min 10, 1 Upper, 1 Num, 1 Spec)");

                ImGui::SetCursorPosX((trueWindowWidth - btnWidth) * 0.5f);
                ImGui::SetCursorPosY(buttonStartY + (btnHeight * 0.8f)); 
                ImGui::InputTextWithHint("##resetpass", "New Password", resetPassword, IM_ARRAYSIZE(resetPassword), ImGuiInputTextFlags_Password);
                
                ImGui::PopStyleColor(4); 
                ImGui::PopItemWidth();

                ImGui::SetCursorPosX((trueWindowWidth - btnWidth) * 0.5f);
                ImGui::SetCursorPosY(buttonStartY + (btnHeight * 1.6f) + spacing);
                if (ImGui::Button("Reset Password", ImVec2(btnWidth, btnHeight))) {
                    std::string targetUser(resetUsername);
                    std::string targetPass(resetPassword);

                    if (!UserExists(targetUser)) {
                        resetError = "Username not found in system.";
                    } else if (!IsValidPassword(targetPass)) {
                        resetError = "Invalid Password! Check the requirements.";
                    } else {
                        if (UpdatePassword(targetUser, targetPass)) {
                            resetError = "";
                            resetSuccessMsg = "Password successfully reset!";
                        } else {
                            resetError = "System Error: Failed to update password.";
                        }
                    }
                }

                ImGui::SetCursorPosX((trueWindowWidth - btnWidth) * 0.5f);
                ImGui::SetCursorPosY(buttonStartY + (btnHeight * 2.6f) + (spacing * 2));
                if (ImGui::Button("<- Back to Login", ImVec2(btnWidth, btnHeight))) {
                    resetError = "";
                    currentState = AppState::WelcomeBack;
                }
            } 
            else {
                ImGui::SetCursorPosX((trueWindowWidth - ImGui::CalcTextSize(resetSuccessMsg.c_str()).x) * 0.5f);
                ImGui::SetCursorPosY(buttonStartY);
                ImGui::Text("%s", resetSuccessMsg.c_str());

                ImGui::SetCursorPosX((trueWindowWidth - btnWidth) * 0.5f);
                ImGui::SetCursorPosY(buttonStartY + btnHeight + spacing);
                if (ImGui::Button("Return to Login", ImVec2(btnWidth, btnHeight))) {
                    resetSuccessMsg = "";
                    resetUsername[0] = '\0';
                    resetPassword[0] = '\0';
                    currentState = AppState::WelcomeBack; 
                }
            }
        }
        // ----------------------------------------------------
        // STATE: INITIAL SETUP
        // ----------------------------------------------------
        else if (currentState == AppState::InitialSetup) {
            // --- FIXED: FUNCTION NOW HAS ALL 4 ARGUMENTS ---
            InitialSetup::Render(trueWindowWidth, windowHeight, bgTexture, activeUser);
        }

        ImGui::PopStyleColor(3); 
        ImGui::SetWindowFontScale(1.0f); 
        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}