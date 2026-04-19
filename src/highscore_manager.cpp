#include "highscore_manager.hpp"
#include <spdlog/spdlog.h>
#include <sstream>

void HighScoreManager::save() const {
    if (m_path.empty()) {
        spdlog::warn("Highscore path is empty, skipping save.");
        return;
    }
    std::ofstream file(m_path);
    if (file.is_open()) {
        for (const auto& entry : m_entries) {
            file << "Score: " << entry.score << "\n"
                 << "AI_Used: " << (entry.aiUsed ? "Yes" : "No") << "\n"
                 << "Obstacles: " << (entry.obstaclesEnabled ? "Yes" : "No") << "\n";
        }
        file.close();
    }
}

void HighScoreManager::load() {
    m_entries.clear();
    if (m_path.empty()) return;
    std::ifstream file(m_path);
    if (file.is_open()) {
        std::string line, label, value;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            
            HighScoreEntry entry;
            std::stringstream ss(line);
            ss >> label >> entry.score;
            
            if (std::getline(file, line)) {
                std::stringstream ssAI(line);
                ssAI >> label >> value;
                entry.aiUsed = (value == "Yes");
            }
            if (std::getline(file, line)) {
                std::stringstream ssObs(line);
                ssObs >> label >> value;
                entry.obstaclesEnabled = (value == "Yes");
            }
            m_entries.push_back(entry);
        }
        file.close();
    }
}

bool HighScoreManager::update(int score, bool aiUsed, bool obstaclesEnabled) {
    if (score <= 0) return false;
    bool qualifies = m_entries.size() < 5 || score > m_entries.back().score;
    if (qualifies) {
        m_entries.push_back({score, aiUsed, obstaclesEnabled});
        std::sort(m_entries.begin(), m_entries.end(), [](const HighScoreEntry& a, const HighScoreEntry& b) {
            return a.score > b.score;
        });
        if (m_entries.size() > 5) m_entries.resize(5);
        save();
        return true;
    }
    return false;
}