// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Mateusz Kłysz
#include "rac1/fx/records.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>

namespace rac1 {

std::string Records::key(const std::string& game, int level) { return game + " " + std::to_string(level); }

void Records::parse(const std::string& text) {
    m_best.clear();
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream words(line);
        std::string game;
        int level = -1;
        long points = 0;
        if (!(words >> game >> level >> points) || level < 0 || points <= 0 || points > 0x7FFFFFFF) continue;
        int& best = m_best[key(game, level)];
        if (points > best) best = static_cast<int>(points);
    }
}

std::string Records::text() const {
    std::string out;
    for (const auto& [k, points] : m_best) out += k + " " + std::to_string(points) + "\n";
    return out;
}

int Records::best(const std::string& k) const {
    const auto it = m_best.find(k);
    return it != m_best.end() ? it->second : 0;
}

bool Records::offer(const std::string& k, int points) {
    if (points <= best(k)) return false;
    m_best[k] = points;
    return true;
}

void RecordFile::load(const std::string& dir) {
    m_path = dir.empty() ? std::string() : dir + "/records.txt";
    if (m_path.empty()) return;
    std::ifstream in(m_path);
    std::stringstream text;
    text << in.rdbuf();
    m_records.parse(text.str());
}

bool RecordFile::offer(const std::string& game, int level, int points) {
    if (level < 0 || !m_records.offer(Records::key(game, level), points)) return false;
    if (!m_path.empty()) {
        // Written beside and then renamed over, so a crash never leaves half a file.
        const std::string temp = m_path + ".new";
        {
            std::ofstream out(temp, std::ios::trunc);
            out << m_records.text();
        }
        std::remove(m_path.c_str());
        std::rename(temp.c_str(), m_path.c_str());
    }
    return true;
}

int RecordFile::best(const std::string& game, int level) const {
    return level < 0 ? 0 : m_records.best(Records::key(game, level));
}

RecordFile& records() {
    static RecordFile r;
    return r;
}

} // namespace rac1
