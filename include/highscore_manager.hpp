#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <fstream>

struct HighScoreEntry {
    int score;
    bool aiUsed;
    bool obstaclesEnabled;
};

class HighScoreManager {
public:
    HighScoreManager() = default;
    explicit HighScoreManager(const std::string& path) : m_path(path) {}

    void setPath(const std::string& p) { m_path = p; }
    void load();
    void save() const;
    bool update(int score, bool aiUsed, bool obstaclesEnabled);
    const std::vector<HighScoreEntry>& getEntries() const { return m_entries; }

private:
    std::vector<HighScoreEntry> m_entries;
    std::string m_path;
};