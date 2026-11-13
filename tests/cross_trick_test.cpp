// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Mateusz Kłysz
// Setting trick_button "x" (src/rac1/cross_trick.hpp): a second Cross press in the air does tricks.
#include "check.hpp"
#include "rac1/cross_trick.hpp"

using rac1::CrossTrick;

int main() {
    // The jump's press on the ground, held into the air: a plain jump, Jak sees Cross.
    CrossTrick c;
    c.reset(false);
    CHECK(!c.step(true, false, false));
    CHECK(!c.step(true, true, true));
    CHECK(!c.hide_cross());

    // Let go in the air and press again: the trick, hidden from Jak (no kick), while held.
    CHECK(!c.step(false, true, true));
    CHECK(c.step(true, true, true));
    CHECK(c.hide_cross());
    CHECK(c.step(true, true, true));
    CHECK(!c.step(false, true, true)); // let go: over
    CHECK(!c.hide_cross());

    // Held through the landing: no trick on the ground, and Cross stays hidden (no jump) until let go.
    CHECK(c.step(true, true, true));
    CHECK(!c.step(true, false, false));
    CHECK(c.hide_cross());
    CHECK(!c.step(false, false, false));
    CHECK(!c.hide_cross());
    CHECK(!c.step(true, false, false)); // a fresh press on the ground: the jump

    // A press the caller does not allow (just before the landing, or grinding): a plain Cross.
    c.reset(false);
    CHECK(!c.step(true, true, false));
    CHECK(!c.hide_cross());

    // A ride begun with Cross held: not a press.
    c.reset(true);
    CHECK(!c.step(true, true, true));
    CHECK(!c.hide_cross());
    TEST_MAIN_END();
}
