Retro REPLAY 🎮
Retro REPLAY is a persistent, multi-window C++ desktop application designed to act as a "Command Center" for retro gamers. It allows users to track their game backlogs, manage active playthroughs, and generate personalized, AI-driven gaming strategies and YouTube walkthrough recommendations.

🚀 Overview
Managing a massive retro game collection can be overwhelming. Retro REPLAY solves this by providing a localized, secure vault to track progress across multiple classic consoles.

Rather than just logging data, the application actively assists the user. By integrating with cloud-based AI and YouTube APIs, the software analyzes a user's current game rotation and overarching yearly goals to generate targeted advice, strategies, and video guides on the fly.

✨ Key Features
Custom State-Machine Architecture: Smooth, crash-free navigation between Welcome, Login, Setup, and Dashboard screens without memory leaks.

Secure Authentication System: Includes user registration, login validation, and password recovery, utilizing localized password hashing for security.

AI Strategy Generation: Connects to external REST APIs via std::thread to prevent UI freezing, returning actionable gameplay advice and YouTube walkthrough links based on the user's current goals.

Dynamic Data Management (CRUD): A robust "Edit Mode" utilizing dirty flags (isDirty) to track UI changes. Users can add, update, or remove systems and goals, instantly rewriting their localized .csv database and triggering new AI recommendations only when necessary.

Responsive UI/UX: Built with Dear ImGui, featuring custom scaling algorithms that dynamically adjust font sizes, button dimensions, and geometry to support both 1080p monitors and smaller laptop displays (<900p).

Input Validation: Enforces a "Retro-Only" rule, actively preventing users from logging modern consoles (e.g., PS5, Xbox Series X, Nintendo Switch) into their classic vault.

🛠️ Tech Stack & Concepts
Language: C++11 / C++14

GUI Framework: Dear ImGui

Graphics API: OpenGL3 / GLFW

Image Loading: stb_image

Concurrency: std::thread, std::atomic, std::mutex

Data Persistence: Flat-file database management (profiles.csv, credentials.txt)

📂 Project Structure
main.cpp - Initializes GLFW/OpenGL, handles the core rendering loop, and manages the application state machine.

InitialSetup.h - Captures new user data, enforces console rules, and runs the initial AI strategy generation.

VaultDashboard.h - The core application hub, managing navigation between the Home Page and Data Management tabs, and formatting the AI text output.

GameManagement.h - Handles dynamic CRUD operations, active buffer tracking, UI updates, and safe vector deletion.

APIClient.h - Manages the HTTP requests to external AI and YouTube APIs.

⚙️ Installation & Build Instructions
Prerequisites
A C++ compiler (MSVC, GCC, or Clang)

CMake (Recommended for generating build files)

OpenGL & GLFW libraries installed on your system

📥 Download & Play (For Gamers)
You don't need to be a programmer to use the Retro REPLAY Vault! If you just want to manage your backlog and get AI strategies, follow these steps:

Download the App: Navigate to the Releases section on the right side of this GitHub page and download the latest RetroREPLAY_Windows.zip (or your OS equivalent).

Extract the Files: Right-click the downloaded .zip file and select "Extract All...".

Keep the Folder Intact: Ensure the assets/ folder (which contains the backgrounds and logos) stays in the exact same folder as the main executable file.

Launch: Double-click RetroREPLAY.exe to launch the application.

Create your Vault: Click "Get Started" to create your secure offline profile and start logging your retro collection!

(Note: Your profile data, passwords, and strategies are saved entirely locally on your machine in the profiles.csv and credentials.txt files that generate automatically upon your first launch.)

💻 Build Instructions (For Developers)
If you want to view the source code, modify the UI, or compile the project yourself, follow these steps:

Prerequisites
A C++11 (or higher) compatible compiler (MSVC, GCC, or Clang)

CMake (Recommended for generating build files)

OpenGL & GLFW libraries installed on your system

Setup & Compilation
Clone the repository:

Bash
git clone https://github.com/YourUsername/RetroREPLAY.git
cd RetroREPLAY
Add API Keys:

Locate the APIClient.h file in the source code.

Insert your specific AI and YouTube Data API keys into the designated placeholder strings. (Never commit your active API keys to a public repository!)

Build the Project:

Generate the build files using CMake and compile the executable according to your specific IDE or compiler setup.

Run:

Ensure the assets/ folder is placed in the same directory as your newly compiled executable so the textures render correctly.