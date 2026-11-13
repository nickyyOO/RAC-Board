// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Mateusz Kłysz
// Setting trick_button "x" (src/rac1/cross_trick.hpp): when Cross held in the air does a trick.
#include "check.hpp"
#include "rac1/cross_trick.hpp"

using rac1::CrossTrick;

int main() {
    const uint8_t mid = 128, right = 255, up = 0, small = 100;

    // Cross held in the air, then the stick pushed sideways: a trick, kept with the stick let go.
    CrossTrick c;
    c.reset(mid, mid);
    CHECK(!c.step(true, false, mid, mid)); // on the ground: the jump
    CHECK(!c.step(true, true, mid, mid));  // in the air, stick centred: a plain high jump
    CHECK(c.step(true, true, right, mid));
    CHECK(c.step(true, true, mid, mid));
    CHECK(!c.step(false, true, mid, mid)); // Cross let go: over
    CHECK(!c.step(true, true, mid, mid));  // Cross again, stick still: nothing

    // Stick held forward from the ground (steering) into the air: no flip until it moves again.
    c.reset(mid, mid);
    CHECK(!c.step(true, false, mid, up));
    CHECK(!c.step(true, true, mid, up));
    CHECK(!c.step(true, true, mid, up));
    CHECK(c.step(true, true, right, up)); // pushed sideways: a spin
    CHECK(!c.step(true, false, right, up)); // landed: over

    // Forward let go and pushed again in the air: a flip.
    c.reset(mid, mid);
    CHECK(!c.step(true, false, mid, up));
    CHECK(!c.step(true, true, mid, small));
    CHECK(c.step(true, true, mid, up));

    // A stick inside the dead zones does nothing.
    c.reset(mid, mid);
    CHECK(!c.step(true, true, small, small));

    // A ride begun with the stick pushed: that push counts as old.
    c.reset(right, mid);
    CHECK(!c.step(true, true, right, mid));

    // No Cross: the stick only steers.
    c.reset(mid, mid);
    CHECK(!c.step(false, true, right, mid));
    CHECK(!c.step(true, true, right, mid)); // Cross pressed with the stick already sideways
    TEST_MAIN_END();
}
