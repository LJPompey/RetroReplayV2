#pragma once
#include <string>
#include <fstream>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <algorithm>

namespace APIClient {

    // Safely retrieves the API key and destroys hidden whitespace/carriage returns
    inline std::string GetAPIKey(const std::string& keyName) {
        std::ifstream file("secrets.txt");
        std::string line;
        if (file.is_open()) {
            while (std::getline(file, line)) {
                if (line.find(keyName + "=") == 0) {
                    std::string key = line.substr(keyName.length() + 1);
                    key.erase(0, key.find_first_not_of(" \t\r\n"));
                    key.erase(key.find_last_not_of(" \t\r\n") + 1);
                    return key;
                }
            }
        }
        return "";
    }

    // --- GPT-OSS 120B STRATEGY ENGINE ---
    inline std::string FetchAIStrategy(const std::string& compiledGames, const std::string& currentGoals, const std::string& yearlyGoal) {
        std::string apiKey = GetAPIKey("GROQ_API_KEY");
        if (apiKey.empty()) return "Error: GROQ_API_KEY not found in secrets.txt. Please create one at console.groq.com";

        std::string url = "https://api.groq.com/openai/v1/chat/completions";

        std::string prompt = "You are a top retro gaming enthusiast. The user is playing these specific games on these systems: " + compiledGames + ". " +
                             "Their current goals are: " + currentGoals + ". Their yearly goals are: " + yearlyGoal + ". " +
                             "Please format your response exactly like this:\n\n" +
                             "1. Playthrough Strategy:\n" +
                             "- For [Game Name] ([System]): [Provide a highly specific, time-efficient, 2-sentence strategy on how to beat this exact game efficiently without getting bogged down in side content.]\n" +
                             "(Repeat this format for every single game listed)\n\n" +
                             "2. Goal Strategy:\n" +
                             "[Provide a 3-sentence strategy dynamically connecting their specific games to their current and yearly goals.]\n\n" +
                             "Keep your response clean and professional. Do not use markdown bolding or asterisks, just plain text.";

    // Update the model string to the Safeguard 20B model (or latest model available)
        nlohmann::json requestBody = {
            {"model", "openai/gpt-oss-safeguard-20b"}, 
            {"messages", {{
                {"role", "user"},
                {"content", prompt}
            }}}
        };

        cpr::Response r = cpr::Post(cpr::Url{url},
                                    cpr::Header{
                                        {"Content-Type", "application/json"},
                                        {"Authorization", "Bearer " + apiKey}
                                    },
                                    cpr::Body{requestBody.dump()});

        if (r.status_code == 200) {
            try {
                auto jsonResponse = nlohmann::json::parse(r.text);
                return jsonResponse["choices"][0]["message"]["content"].get<std::string>();
            } catch (...) {
                return "Error: Failed to parse the response from Groq.";
            }
        } else {
            return "Groq API Error " + std::to_string(r.status_code) + "\nLog: " + r.text;
        }
    }

    // --- ZERO-API YOUTUBE WEB SCRAPER ---
    inline std::string FetchYouTubeWalkthrough(const std::string& game, const std::string& system) {
        std::string query = game + " " + system + " walkthrough 100%";
        std::string safeQuery = query;
        std::replace(safeQuery.begin(), safeQuery.end(), ' ', '+');
        
        std::string searchUrl = "https://www.youtube.com/results?search_query=" + safeQuery;

        // Disguise our C++ request as a normal Windows Chrome Browser
        cpr::Response r = cpr::Get(
            cpr::Url{searchUrl},
            cpr::Header{{"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36"}}
        );

        if (r.status_code == 200) {
            std::string html = r.text;
            
            size_t startPos = html.find("var ytInitialData = ");
            if (startPos != std::string::npos) {
                startPos += 20; 
                size_t endPos = html.find(";</script>", startPos);
                
                if (endPos != std::string::npos) {
                    std::string jsonStr = html.substr(startPos, endPos - startPos);
                    
                    try {
                        auto j = nlohmann::json::parse(jsonStr);
                        auto contents = j["contents"]["twoColumnSearchResultsRenderer"]["primaryContents"]["sectionListRenderer"]["contents"][0]["itemSectionRenderer"]["contents"];
                        
                        for (auto& item : contents) {
                            if (item.contains("videoRenderer")) {
                                auto video = item["videoRenderer"];
                                std::string videoId = video["videoId"];
                                std::string title = video["title"]["runs"][0]["text"];
                                std::string channel = video["ownerText"]["runs"][0]["text"];
                                
                                return "  - For " + game + ": '" + title + "' by " + channel + " [" + videoId + "]\n";
                            }
                        }
                    } catch (...) {}
                }
            }
        }
        
        return "  - For " + game + ": Search Ready [" + searchUrl + "]\n";
    }
}