// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Mateusz Kłysz
// Setting trick_button "x": a second Cross press in the air, held, does the spins and flips.
#pragma once

namespace rac1 {

// The jump takes the first Cross press, on the ground. Let go and press Cross again in the air, and
// while it is held it is the board's R1 (spins, flips) instead of Cross: Jak's kick in the air and
// the jump it would buffer for the landing do not see it. The caller says if this frame's press may
// start a trick (`may_own`): not shortly before the landing, so a jump pressed early still comes,
// and not while grinding, where Cross jumps off the rail.
struct CrossTrick {
    bool prev = false;  // Cross held last frame
    bool owned = false; // this Cross hold is the trick's, until Cross is let go

    // A new ride: a Cross already held is not a press.
    void reset(bool cross) {
        prev = cross;
        owned = false;
    }

    // One frame. Returns whether the board's R1 is held; hide_cross() then says if Jak sees Cross.
    bool step(bool cross, bool in_air, bool may_own) {
        const bool press = cross && !prev;
        prev = cross;
        if (!cross) owned = false;
        else if (press && in_air && may_own) owned = true;
        return owned && in_air;
    }
    // Cross is the trick's (kept from Jak even after the landing, until let go, so it does not jump).
    bool hide_cross() const { return owned; }
};

} // namespace rac1
