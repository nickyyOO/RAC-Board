// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Mateusz Kłysz
//
// RAC-Board in Ratchet & Clank and Going Commando: the mod's entry. While Ratchet rides, the mod
// takes his update (rac1.player.update, rac2.player.update) and runs one frame of Jak II's target on the board (src/jak and src/board,
// ported from OpenGOAL) in his place. Off the board the game's own update runs, untouched.
//
// The adapter's parts (src/rac1):
//   context.*      settings, the board's clock, the mod's API
//   pad.*          the pad as Jak II reads it, the board's button
//   ride.*         getting on and off, each frame, drawing Ratchet and the board
//   interaction.*  health, hits on Ratchet, attacks on creatures
//   convert.hpp    units and axes between the two games
//   world/         the level's collision and grind rails for the board, the held item
//   fx/            sounds, particles and the trick combo line
//   anim/          Ratchet's riding clips and their secondary motion
#include "openrac/games/rac1.hpp"
#include "rac1/context.hpp"
#include "rac1/fx/effects.hpp"
#include "rac1/fx/records.hpp"
#include "rac1/camera.hpp"
#include "rac1/interaction.hpp"
#include "rac1/pad.hpp"
#include "rac1/ride.hpp"

namespace {

using namespace rac1;

// Going Commando's hero update (rac2.player.update), as newer SDKs' games/rac2.hpp name it: written
// here so the mod builds against older SDKs too. An OpenRAC without it refuses the hook.
struct Rac2HeroUpdate {
    static constexpr const char* id = "rac2.player.update";
    static constexpr const char* name = "Hero_Update";
    static constexpr const char* signature = "void()";
    using fn = void (*)();
};

// The hero's update: the board's frame while riding, the game's otherwise.
void hero_update(const openrac::Original<void()>& original) {
    // Under the pause menu (the port runs the level on under it) the board stands still, as the
    // game's world does.
    if (riding() && mod().play_status().phase == OPENRAC_PLAY_MENU) {
        effects().pause_sounds();
        return;
    }
    ctx().clock += frame_ticks();
    effects().warm();
    bool toggle = toggle_pressed();
    if (ride_level_changed()) {
        camera_stop(true);
        end_ride();
        effects().clear_particles();
    }
    // The board's particles live on after he gets off, until they fade.
    effects().update();
    camera_blend_tick();
    if (!riding()) {
        if (!(toggle && try_start_ride())) {
            note_on_foot();
            original();
            pose_out_tick();
            return;
        }
        toggle = false; // the press that got on is not also the one that gets off
    }
    const RideStep step = tick_ride(toggle);
    if (step != RideStep::taken) show_ride(step == RideStep::left);
    if (step == RideStep::taken) camera_stop(true); // the game's scene has its own camera
    if (step != RideStep::riding) {
        // Off the board: the game's update goes on from where the board (or the game) left Ratchet.
        end_ride();
        original();
        pose_out_tick();
    }
}

int load(openrac::Mod& m) {
    const bool rac1 = m.game() == "rac1" && (m.version() == "pal" || m.version() == "ntsc");
    const bool rac2 = m.game() == "rac2" && (m.version() == "pal" || m.version() == "ntsc" || m.version() == "ntsc2");
    if (!rac1 && !rac2) {
        m.fail("RAC-Board knows Ratchet & Clank's PAL and NTSC-U discs and Going Commando's only");
        return 1;
    }
    if (!m.has_api_12()) {
        m.fail("RAC-Board needs OpenRAC's mod interface 1.12 (update OpenRAC)");
        return 1;
    }
    ctx().mod = &m;
    ctx().rac2 = rac2;
    ctx().pal = m.version() == "pal";
    ctx().v200 = rac2 && m.version() == "ntsc2";
    load_settings(m);
    records().load(m.data_dir());
    if (m.has_world()) m.on_moby_hit(on_moby_hit);
    m.on_pad(on_pad);
    // The pause menu runs no hero update: the board's sounds held and brought back from here.
    m.on_frame([] {
        if (riding() && mod().play_status().phase == OPENRAC_PLAY_MENU) {
            effects().pause_sounds();
        } else {
            effects().resume_sounds();
        }
    });
    if (rac2) {
        m.hook<Rac2HeroUpdate>([](const auto& original) { hero_update(original); });
    } else {
        m.hook<openrac::rac1::Hero_Update>([](const auto& original) { hero_update(original); });
    }
    m.info("RAC-Board ready");
    return 0;
}

} // namespace

OPENRAC_MOD_CPP("rac-board", "0.3.0", load)
