// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Mateusz Kłysz
// The manual: land holding the grab button and he rides on the board's tail, keeping the combo
// going into the next jump. The left stick keeps his balance; past the edge he bails and the combo
// is lost. Let go of the button and the combo's points are his.
#pragma once

#include "jak/target.hpp"

namespace rac1 {

class Manual {
  public:
    // One frame of the ride, after the board's: starts, balances and ends the manual.
    // `grab` the grab button held, `stick` the left stick's x (-1 left to 1 right), `dt` seconds.
    void update(jak::Target& t, bool grab, float stick, float dt);
    // The ride is over: a manual still on gives its points.
    void finish(jak::Target& t);
    // Whether the board's landing and jumps keep the trick list now (Interaction::hold_combo).
    bool hold(const jak::Target& t, bool grab) const;

    bool active() const { return m_active; }
    // The balance: 0 level, -1 or 1 the edge.
    float balance() const { return m_balance; }
    // How far the board's nose is up (radians), drawn as it eases in and out.
    float tilt() const { return m_tilt; }

  private:
    void end(jak::Target& t, bool flush);

    bool m_active = false;
    float m_air = 0.0f; // seconds off the ground
    float m_balance = 0.0f;
    float m_speed = 0.0f; // the balance's speed (a second)
    float m_time = 0.0f;  // seconds in this manual
    float m_tilt = 0.0f;
};

Manual& manual();

} // namespace rac1
