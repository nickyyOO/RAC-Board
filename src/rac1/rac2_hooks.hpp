// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Mateusz Kłysz
// The Going Commando hook points the mod uses, as newer SDKs name them in games/rac2.hpp. They are
// written here so the mod still builds with SDK 1.12. An OpenRAC without them refuses the hook or
// finds no function.
#pragma once

#include <cstdint>

namespace rac1::rac2 {

// rac2.player.update: the hero's whole update each tick.
struct Hero_Update {
    static constexpr const char* id = "rac2.player.update";
    static constexpr const char* name = "Hero_Update";
    static constexpr const char* signature = "void()";
    using fn = void (*)();
};

// rac2.player.take_damage: takes a hit's points off his health at once.
struct Hero_TakeDamage {
    static constexpr const char* id = "rac2.player.take_damage";
    static constexpr const char* name = "Hero_TakeDamage";
    static constexpr const char* signature = "void(i32)";
    using fn = void (*)(int32_t);
};

// rac2.combat.get_hit: the hit waiting for a moby (its entry in the hit ring), 0 for none.
struct Moby_GetHitMessage {
    static constexpr const char* id = "rac2.combat.get_hit";
    static constexpr const char* name = "Moby_GetHitMessage";
    static constexpr const char* signature = "u32(u32,u32,i32)";
    using fn = uint32_t (*)(uint32_t, uint32_t, int32_t);
};

} // namespace rac1::rac2
