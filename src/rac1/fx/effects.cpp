// SPDX-License-Identifier: MIT AND ISC
// Copyright (c) 2026 Mateusz Kłysz
// The combo line follows render-combo of OpenGOAL's PC port (target-board.gc; jak-project, ISC
// License, Copyright (c) 2020-2026 OpenGOAL Team; THIRD_PARTY.md), and the trick names are its
// English texts (trick-board-*).
#include "rac1/fx/effects.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <vector>

#include "rac1/context.hpp"
#include "rac1/convert.hpp"
#include "rac1/fx/records.hpp"

namespace rac1 {

namespace {

// The names in board-tricks order (text ids trick-board-none and on).
constexpr const char* kTrickNames[] = {"",          "Spin",       "Boost",      "Flip",     "Rail Grind",
                                       "Rail Jump", "Nosegrab",   "Method",     "Board Spin", "Board Flip",
                                       "Noseflip",  "Kickflip",   "Jump",       "Duck Jump",  "Quick Jump",
                                       "",          "",           "",           "",         "Manual"};
constexpr int kTrickNameCount = sizeof(kTrickNames) / sizeof(kTrickNames[0]);

// How long the last combo stays on screen once it is over.
constexpr float kComboShown = 3.0f;

// GOAL's pitch-mod: 1024 an octave.
float pitch_ratio(float pitch) {
    const float r = std::exp2(pitch / 1024.0f);
    return r < 0.25f ? 0.25f : (r > 4.0f ? 4.0f : r);
}

} // namespace

Effects& effects() {
    static Effects e;
    return e;
}

int Effects::sound(const char* name) {
    auto it = m_sounds.find(name);
    if (it != m_sounds.end()) return it->second;
    const int s = mod().sound_load(std::string("assets/sounds/") + name + ".wav");
    m_sounds[name] = s;
    return s;
}

// The mod's own sounds (assets/sounds) and the effects' meshes.
constexpr const char* kSoundFiles[] = {"board-bank",  "board-boost",     "board-boots",  "board-bounce",
                                       "board-glance", "board-spin-loop", "board-steady", "board-turn",
                                       "board-wind",   "get-burned",      "swim-enter"};
constexpr const char* kFxMeshes[] = {"assets/fx/compass.glb", "assets/fx/glow.glb", "assets/fx/streak.glb",
                                     "assets/fx/ring.glb",    "assets/fx/puff.glb", "assets/fx/burst.glb"};

void Effects::warm() {
    if (m_warm) return;
    // A prop is made only in a level: all of them in one frame, so the port reads the level again
    // once, here, and keeps the meshes for every level after.
    // The board's model first, then the effects'.
    const char* meshes[1 + std::size(kFxMeshes)] = {board_mesh()};
    std::copy(std::begin(kFxMeshes), std::end(kFxMeshes), meshes + 1);
    int ids[std::size(meshes)];
    for (std::size_t i = 0; i < std::size(meshes); ++i) {
        ids[i] = mod().prop_create(meshes[i]);
        if (ids[i] < 0) {
            for (std::size_t j = 0; j < i; ++j) mod().prop_remove(ids[j]);
            return; // not in a level yet (or no props free): the next frame tries again
        }
    }
    for (const int id : ids) mod().prop_remove(id);
    for (const char* name : kSoundFiles) sound(name);
    m_warm = true;
}

namespace {

// Jak's board sounds that Ratchet has his own of, the game's (entries of his class sound table, as
// his own moves play them): his jump and double jump for the board's, his wrench swing's whoosh for
// the tricks', and for the rail his landing on one and his grind's loop.
struct HeroSound {
    const char* name;
    int sound;
};
constexpr HeroSound kHeroSounds[] = {
    {"board-jump", 26},     {"board-q-jump", 26},   {"board-k-jump", 36}, {"board-kick", 27},
    {"board-kickflip", 27}, {"board-noseflip", 27}, {"board-flip", 27},   {"board-method", 27},
    {"board-nosegrab", 27}, {"board-boardspin", 53},
};
constexpr int kHeroRailLand = 13;
constexpr int kHeroGrindLoop = 0;

// Jak's jingles for a trick and its points: none in Ratchet's game, so none here.
// Left out: the trick's own cues, and the bank's hiss on a turn or a spin (user decision).
bool silent(const char* name) {
    return std::strcmp(name, "do-trick") == 0 || std::strcmp(name, "trick-score") == 0 ||
           std::strcmp(name, "board-bank") == 0;
}

int hero_sound(const char* name) {
    for (const HeroSound& h : kHeroSounds) {
        if (std::strcmp(h.name, name) == 0) return h.sound;
    }
    return -1;
}

} // namespace

uint32_t Effects::sound_play(const char* name, float volume, float pitch) {
    if (silent(name)) return 0;
    if (const int h = hero_sound(name); h >= 0) {
        mod().hero_sound_play(h);
        return 0;
    }
    const int s = sound(name);
    if (s < 0) return 0;
    // A "-loop" sound (the spin's) loops in Jak II's bank until the board stops it.
    const size_t n = std::strlen(name);
    const bool loop = n > 5 && std::strcmp(name + n - 5, "-loop") == 0;
    const uint32_t voice = mod().sound_play(s, volume, pitch_ratio(pitch), loop);
    if (voice == 0) return 0;
    const uint32_t id = m_next++;
    m_voices[id] = voice;
    m_levels[id] = {volume, pitch_ratio(pitch)};
    if (m_paused) mod().sound_set(voice, 0.0f, pitch_ratio(pitch));
    if (!loop) m_once.insert(id);
    return id;
}

void Effects::sound_loop(const char* name, uint32_t id, float volume, float pitch) {
    if (silent(name)) return;
    // The rail: Ratchet's own landing on one, then his grind's loop until it stops.
    if (std::strcmp(name, "board-rail") == 0) {
        if (m_hero_voices.count(id) == 0) {
            mod().hero_sound_play(kHeroRailLand);
            m_hero_voices[id] = mod().hero_sound_play(kHeroGrindLoop, true);
        }
        return;
    }
    auto it = m_voices.find(id);
    if (it != m_voices.end()) {
        m_levels[id] = {volume, pitch_ratio(pitch)};
        if (mod().sound_set(it->second, m_paused ? 0.0f : volume, pitch_ratio(pitch))) return;
    }
    // A one-shot that ended, or was stopped under the pause, stays over: started again as a loop it
    // played on for good once the board dropped its id (the spin's loop after a pause).
    if (m_once.count(id) != 0) return;
    const int s = sound(name);
    if (s < 0) return;
    const uint32_t voice = mod().sound_play(s, m_paused ? 0.0f : volume, pitch_ratio(pitch), true);
    if (voice != 0) {
        m_voices[id] = voice;
        m_levels[id] = {volume, pitch_ratio(pitch)};
    }
}

void Effects::sound_stop(uint32_t id) {
    if (auto h = m_hero_voices.find(id); h != m_hero_voices.end()) {
        if (h->second >= 0) mod().hero_sound_stop(h->second);
        m_hero_voices.erase(h);
    }
    m_once.erase(id);
    auto it = m_voices.find(id);
    m_levels.erase(id);
    if (it == m_voices.end()) return;
    mod().sound_stop(it->second);
    m_voices.erase(it);
}

// Under the pause menu every voice is held silent where it is, and comes back as it was after.
void Effects::pause_sounds() {
    if (m_paused) return;
    m_paused = true;
    trace("pause: " + std::to_string(m_voices.size()) + " voices held");
    for (const auto& v : m_voices) {
        const auto l = m_levels.find(v.first);
        mod().sound_set(v.second, 0.0f, l != m_levels.end() ? l->second.second : 1.0f);
    }
}

void Effects::resume_sounds() {
    if (!m_paused) return;
    m_paused = false;
    for (const auto& v : m_voices) {
        const auto l = m_levels.find(v.first);
        if (l != m_levels.end()) mod().sound_set(v.second, l->second.first, l->second.second);
    }
}

void Effects::stop_sounds() {
    for (const auto& v : m_voices) mod().sound_stop(v.second);
    m_voices.clear();
    m_levels.clear();
    m_paused = false;
    m_once.clear();
    for (const auto& v : m_hero_voices) {
        if (v.second >= 0) mod().hero_sound_stop(v.second);
    }
    m_hero_voices.clear();
}

void Effects::particles(const char* group, const jak::Vec& at, float rate) {
    float p[3];
    to_game_point(at, p);
    m_fx.launch(group, p, yaw, rate);
}

void Effects::shock(const float at[3]) { m_fx.shock(at); }

void Effects::clear_particles() { m_fx.clear(mod()); }

// add-trick-to-combo!: a new combo starts over, then the trick and its points go on the list.
void Effects::trick(jak::BoardTricks t, float points) {
    if (!m_in_combo) {
        m_in_combo = true;
        m_bailed = false;
        m_count = 0;
        m_points = 0.0f;
        m_record.clear();
        m_new_record = false;
    }
    if (m_count < 16) {
        m_tricks[m_count++] = t;
        m_points += points;
    }
    show_combo();
}

// end-combo!: the line stays, as the PC port leaves it, until the next combo (or a while).
void Effects::combo_end() {
    m_in_combo = false;
    m_shown = kComboShown;
    // The planet's best combo: a new one is kept and called out, else the best stays in view.
    const int level = mod().play_status().level;
    const int points = static_cast<int>(m_points);
    m_new_record = points > 0 && records().offer(std::string(mod().game()), level, points);
    const int best = records().best(std::string(mod().game()), level);
    m_record = m_new_record ? std::string("New record!") : best > 0 ? "Best " + std::to_string(best) : std::string();
    if (m_new_record) trace("new record " + std::to_string(points) + " on level " + std::to_string(level));
    draw_combo();
}

// The manual's bail: the combo is lost, its line says so and no points are kept.
void Effects::combo_bail() {
    if (!m_in_combo) m_line = "Manual"; // a manual straight after a flushed landing
    m_in_combo = false;
    m_shown = kComboShown;
    m_score.clear();
    m_record = "Bail!";
    m_new_record = false;
    m_bailed = true;
    draw_combo();
}

void Effects::manual_meter(bool on, float balance) {
    m_meter = on;
    m_balance = balance;
}

// render-combo: each trick once, in the order done, " x N" when done more than once, then the points.
void Effects::show_combo() {
    if (!settings().trick_display || m_count == 0) return;
    std::string line;
    bool printed[kTrickNameCount] = {};
    for (int i = 0; i < m_count; ++i) {
        const int t = static_cast<int>(m_tricks[i]);
        if (t <= 0 || t >= kTrickNameCount || printed[t]) continue;
        printed[t] = true;
        int times = 0;
        for (int j = 0; j < m_count; ++j) times += m_tricks[j] == m_tricks[i];
        if (!line.empty()) line += " + ";
        line += kTrickNames[t];
        if (times > 1) line += " x " + std::to_string(times);
    }
    // The HUD's line holds 63 characters: a long combo keeps its last tricks.
    constexpr std::size_t kMost = 60;
    if (line.size() > kMost) {
        std::size_t cut = line.find(" + ", line.size() - kMost + 4);
        line = "... " + (cut == std::string::npos ? line.substr(line.size() - kMost + 4) : line.substr(cut + 1));
    }
    m_line = line;
    m_score = m_points > 0.0f ? std::to_string(static_cast<int>(m_points)) : std::string();
    draw_combo();
}

// The combo in the game's HUD, in its own font, low in the middle of the screen as Jak II has it:
// the tricks, the points under them, fading out at the end of the time the line stays.
void Effects::draw_combo() {
    const bool meter = settings().trick_display && m_meter;
    const bool shown = meter || (settings().trick_display && (m_in_combo || m_shown > 0.0f) && !m_line.empty());
    if (!shown) {
        if (m_drawn) mod().hud_text_set({});
        m_drawn = false;
        return;
    }
    const float fade = m_in_combo ? 1.0f : std::min(1.0f, m_shown / 0.5f);
    const auto a = static_cast<uint8_t>(255.0f * fade + 0.5f);
    std::vector<openrac_hud_text> lines;
    const auto add = [&](const std::string& text, float y, float scale, uint8_t r, uint8_t g, uint8_t b) {
        if (text.empty()) return;
        openrac_hud_text l{};
        std::snprintf(l.text, sizeof l.text, "%s", text.c_str());
        l.x = 0.5f;
        l.y = y;
        l.scale = scale;
        l.rgba[0] = r, l.rgba[1] = g, l.rgba[2] = b, l.rgba[3] = a;
        l.flags = OPENRAC_HUD_SHADOW;
        lines.push_back(l);
    };
    if (meter) {
        // The manual's balance: the mark slides to the edge he falls off at, green to red.
        constexpr int kSlots = 13;
        const float b = std::clamp(m_balance, -1.0f, 1.0f);
        const int at = static_cast<int>(std::lround((b + 1.0f) * 0.5f * (kSlots - 1)));
        std::string bar = "[";
        for (int i = 0; i < kSlots; ++i) bar += i == at ? 'O' : (i == kSlots / 2 ? '|' : '-');
        bar += "]";
        const float d = std::fabs(b);
        const auto lerp = [](float a, float c, float k) { return static_cast<uint8_t>(a + (c - a) * k + 0.5f); };
        openrac_hud_text l{};
        std::snprintf(l.text, sizeof l.text, "%s", bar.c_str());
        l.x = 0.5f;
        l.y = 0.70f;
        l.scale = 0.9f;
        l.rgba[0] = lerp(120, 255, d), l.rgba[1] = lerp(230, 70, d), l.rgba[2] = lerp(120, 50, d), l.rgba[3] = 255;
        l.flags = OPENRAC_HUD_SHADOW;
        lines.push_back(l);
    }
    if (m_in_combo || m_shown > 0.0f) {
        add(m_line, 0.76f, 0.8f, 220, 220, 220);
        add(m_score, 0.83f, 1.25f, 255, 190, 40);
        if (m_bailed) {
            add(m_record, 0.89f, 1.0f, 255, 80, 60);
        } else if (m_new_record) {
            add(m_record, 0.89f, 0.9f, 120, 230, 255);
        } else {
            add(m_record, 0.89f, 0.7f, 170, 170, 170);
        }
    }
    m_drawn = mod().hud_text_set(lines);
}

void Effects::update() {
    m_fx.haze = settings().heat_haze && mod().has_camera();
    m_fx.update(mod(), frame_seconds());
    if (m_shown > 0.0f) m_shown = std::max(0.0f, m_shown - frame_seconds());
    if (m_drawn || m_in_combo || m_shown > 0.0f || m_meter) draw_combo();
}

} // namespace rac1
