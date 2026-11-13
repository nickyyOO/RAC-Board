// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Mateusz Kłysz
// Setting trick_button "x": Cross held in the air does the spins and flips.
#pragma once

#include <cstdint>
#include <cstdlib>

namespace rac1 {

// When the board's R1 (spins, flips) is held with the trick on Cross. The trick starts when the
// left stick moves into a trick dead zone (64 sideways, 96 up or down, as target-board-spin-check
// reads it) while Cross is held in the air. A stick already pushed at take-off (steering) starts
// nothing until it moves again. Once started, it lasts until Cross is let go or the board lands.
struct CrossTrick {
    bool on = false;
    bool side = false; // the stick in the sideways zone last frame
    bool fore = false; // the stick in the up/down zone last frame

    // A new ride: the stick as it is now counts as already pushed.
    void reset(uint8_t lx, uint8_t ly) {
        on = false;
        side = std::abs(128 - static_cast<int>(lx)) >= 64;
        fore = std::abs(128 - static_cast<int>(ly)) >= 96;
    }

    bool step(bool cross, bool in_air, uint8_t lx, uint8_t ly) {
        const bool s = std::abs(128 - static_cast<int>(lx)) >= 64;
        const bool f = std::abs(128 - static_cast<int>(ly)) >= 96;
        const bool moved = (s && !side) || (f && !fore);
        side = s;
        fore = f;
        if (!cross || !in_air) on = false;
        else if (moved) on = true;
        return on;
    }
};

} // namespace rac1
