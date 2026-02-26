#pragma once
#include <string>
#include <thread>
#include <chrono>

namespace APIClient {

    // A fake Gemini fetcher that mimics the AI's exact formatting and response time
    inline std::string FetchGeminiStrategy(const std::string& compiledGames, const std::string& currentGoals, const std::string& yearlyGoal) {
        
        // Simulates the 2-3 second delay of an internet connection!
        std::this_thread::sleep_for(std::chrono::milliseconds(2500)); 

        std::string mockResponse = 
            "1. Playthrough Strategy:\n"
            "- For Sonic Riders (Playstation 2): Focus strictly on mastering the drifting mechanics in the main story mode to shave seconds off your race times before attempting side missions.\n"
            "- For T.M.N.T. (Playstation 2): Rely heavily on cooperative combo attacks and focus on clearing standard enemy waves quickly to conserve health for boss fights.\n"
            "- For God of War II (Playstation 2): Prioritize upgrading the Blades of Athena early to handle crowd control effectively, allowing you to breeze through the main narrative.\n\n"
            "2. Goal Strategy:\n"
            "Treating your current weekend gaming blocks as focused sprints will naturally chip away at your larger yearly objective. By setting strict 2-hour limits per game, you will maintain momentum across your entire Playstation 2 backlog without burning out on a single title.";

        return mockResponse;
    }

    // A fake YouTube fetcher
    inline std::string FetchYouTubeWalkthrough(const std::string& game, const std::string& system) {
        // Simulates a tiny network delay
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); 
        
        return "  - For " + game + ": 'Ultimate 100% Walkthrough - No Commentary' by RetroLongplays\n";
    }
}