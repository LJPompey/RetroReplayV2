#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Helper function to load an image into an OpenGL texture
bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height) {
    int image_width = 0;
    int image_height = 0;
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

int main() {
    // 1. Initialize GLFW
    if (!glfwInit()) return -1;

    // 2. Create OS Window
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Retro REPLAY V2", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // 3. Initialize Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    // Removed io.FontGlobalScale so we can control it dynamically per window size
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // 4. Load Assets
    GLuint bgTexture = 0, replayLogo = 0, consoleLogo = 0;
    int bgW, bgH, repW, repH, conW, conH;
    LoadTextureFromFile("assets/background_icons.jpg", &bgTexture, &bgW, &bgH);
    LoadTextureFromFile("assets/logo_retroreplay.png", &replayLogo, &repW, &repH);
    LoadTextureFromFile("assets/logo_consoles.png", &consoleLogo, &conW, &conH);

    // 5. Main Render Loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Setup Fullscreen ImGui Window over the OS Window
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

        // Draw Background (Matches window size)
        if (bgTexture) {
            ImGui::GetWindowDrawList()->AddImage(
                (void*)(intptr_t)bgTexture, 
                viewport->WorkPos, 
                ImVec2(viewport->WorkPos.x + viewport->WorkSize.x, viewport->WorkPos.y + viewport->WorkSize.y)
            );
        }

        // Draw Console Logo as a full-screen overlay
        if (consoleLogo) {
            ImGui::GetWindowDrawList()->AddImage(
                (void*)(intptr_t)consoleLogo, 
                viewport->WorkPos, 
                ImVec2(viewport->WorkPos.x + viewport->WorkSize.x, viewport->WorkPos.y + viewport->WorkSize.y)
            );
        }

        // Get dynamic window sizes
        float windowHeight = viewport->WorkSize.y;
        float trueWindowWidth = ImGui::GetWindowWidth();

        // 1. DYNAMIC LOGO SCALING (Applies to both)
        float scaledRepW = trueWindowWidth * 0.45f; 
        float scaledRepH = scaledRepW * ((float)repH / (float)repW);

        // Variables we will define based on screen size
        float logoY, btnWidth, btnHeight, buttonStartY, spacing;

        // 2. SEPARATE LOGIC: SMALL WINDOW VS FULLSCREEN
        if (windowHeight < 900.0f) {
            // --- SET TEXT SIZE FOR SMALL WINDOW ---
            ImGui::SetWindowFontScale(1.5f); 
            
            // --- SMALL WINDOW SETTINGS ---
            logoY = windowHeight * 0.20f;              // Adjust if needed
            btnWidth = 400.0f;                         
            btnHeight = 75.0f;                         
            buttonStartY = logoY + scaledRepH + -75.0f; // Adjust offset if needed
            spacing = 25.0f;                           
        } 
        else {
            // --- SET TEXT SIZE FOR FULLSCREEN ---
            ImGui::SetWindowFontScale(2.5f); // Scale up text for larger buttons
            
            // --- FULLSCREEN (LARGE WINDOW) SETTINGS ---
            logoY = windowHeight * 0.25f;       // Pushes logo up to 25% of the screen height
            btnWidth = trueWindowWidth * 0.25f; // Buttons take up 25% of screen width
            btnHeight = windowHeight * 0.08f;   // Buttons take up 8% of screen height
            buttonStartY = logoY + scaledRepH + -145.0f; 
            spacing = 40.0f; 
        }

        // Draw Retro REPLAY Logo (Perfectly Centered)
        if (replayLogo) {
            ImGui::SetCursorPosX((trueWindowWidth - scaledRepW) * 0.5f);
            ImGui::SetCursorPosY(logoY);
            ImGui::Image((void*)(intptr_t)replayLogo, ImVec2(scaledRepW, scaledRepH));
        }

        // Apply "Clear Red" Styling
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 0.8f));        
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.2f, 0.2f, 0.9f)); 
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));  

        // Draw First Button (Perfectly Centered)
        ImGui::SetCursorPosX((trueWindowWidth - btnWidth) * 0.5f);
        ImGui::SetCursorPosY(buttonStartY);
        if (ImGui::Button("Get Started", ImVec2(btnWidth, btnHeight))) {
            std::cout << "Get Started Clicked!" << std::endl;
        }

        // Draw Second Button (Perfectly Centered)
        ImGui::SetCursorPosX((trueWindowWidth - btnWidth) * 0.5f);
        ImGui::SetCursorPosY(buttonStartY + btnHeight + spacing);
        if (ImGui::Button("Welcome Back", ImVec2(btnWidth, btnHeight))) {
            std::cout << "Welcome Back Clicked!" << std::endl;
        }

        ImGui::PopStyleColor(3);
        
        // Reset font scale so it doesn't leak into other parts of the app later
        ImGui::SetWindowFontScale(1.0f); 

        ImGui::End();

        // ----------------------------------------------------
        // RENDERING
        // ----------------------------------------------------
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}