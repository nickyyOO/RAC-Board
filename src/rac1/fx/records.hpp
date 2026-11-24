// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Mateusz Kłysz
// The best combo on each planet: its points, kept in a small text file in the mod's data folder.
#pragma once

#include <map>
#include <string>

namespace rac1 {

class Records {
  public:
    // A planet's key: the game and its level number, for example "rac1 3".
    static std::string key(const std::string& game, int level);

    // The file's text: one "<game> <level> <points>" line per planet. Bad lines are skipped.
    void parse(const std::string& text);
    std::string text() const;

    // The best points on a planet, 0 for none yet.
    int best(const std::string& key) const;
    // A finished combo: true (and kept) when it beats the planet's best.
    bool offer(const std::string& key, int points);

  private:
    std::map<std::string, int> m_best;
};

// The mod's records, read from and written to its data folder (empty: kept in memory only).
class RecordFile {
  public:
    void load(const std::string& dir);
    // A finished combo on a level: true when it is a new record (then written to the file).
    bool offer(const std::string& game, int level, int points);
    int best(const std::string& game, int level) const;

  private:
    std::string m_path;
    Records m_records;
};

RecordFile& records();

} // namespace rac1
