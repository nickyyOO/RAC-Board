// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Mateusz Kłysz
#include "rac1/manual.hpp"

#include <algorithm>
#include <cmath>
#include <string>

#include "board/board.hpp"
#include "rac1/context.hpp"

namespace rac1 {

namespace {

// Points for each second on the tail, and the least time that counts as a manual.
constexpr float kPointsPerSecond = 300.0f;
constexpr float kLeastTime = 0.5f;
// The balance: how fast a lean grows by itself, how hard the stick pushes back, how the swing is
// damped, and the board's sway, which grows the longer he stays up.
constexpr float kTip = 2.2f;
constexpr float kStick = 4.5f;
constexpr float kDamp = 1.5f;
constexpr float kSway = 1.1f;
constexpr float kSwayGrowth = 0.12f; // more a second
// How far the nose goes up (radians), and how much more the balance tips it.
constexpr float kNoseUp = 0.22f;
constexpr float kLeanTilt = 0.12f;
constexpr float kTiltEase = 12.0f; // a second
// The ground flickers under the board on bumps: a landing needs this long in the air before it, and
// the manual ends in a jump only once he is off the ground this long.
constexpr float kLandingAir = 0.25f;
constexpr float kOffGround = 0.12f;

bool on_surface(const jak::Target& t) { return (t.control.status & jak::cstat::on_surface) != 0; }

} // namespace

Manual& manual() {
    static Manual m;
    return m;
}

bool Manual::hold(const jak::Target& t, bool grab) const {
    // On, or landing into one: the landing's flush comes before this frame's update.
    return m_active || (grab && on_surface(t) && m_air >= kLandingAir);
}

void Manual::update(jak::Target& t, bool grab, float stick, float dt) {
    const bool ground = on_surface(t);
    if (!m_active && grab && ground && m_air >= kLandingAir) {
        // Landed holding the button: up on the tail, leaning a little one way or the other.
        static int count = 0;
        m_active = true;
        m_time = 0.0f;
        m_speed = 0.0f;
        m_balance = (count++ & 1) ? 0.12f : -0.12f;
        trace("manual");
    }
    m_air = ground ? 0.0f : m_air + dt;
    if (m_active) {
        if (m_air >= kOffGround) {
            // A jump or a drop from it: the manual is one more trick of the combo in the air.
            end(t, false);
        } else if (!grab) {
            end(t, true);
        } else {
            m_time += dt;
            const float sway = kSway * (1.0f + kSwayGrowth * m_time) *
                               (0.6f * std::sin(1.7f * m_time + 0.4f) + 0.4f * std::sin(3.3f * m_time + 1.3f));
            const float accel = kTip * m_balance + sway - kStick * stick;
            m_speed += accel * dt;
            m_speed *= std::max(0.0f, 1.0f - kDamp * dt);
            m_balance += m_speed * dt;
            if (std::fabs(m_balance) >= 1.0f) {
                trace("manual bail after " + std::to_string(m_time) + " s");
                m_active = false;
                m_balance = 0.0f;
                jak::drop_trick_list(t);
            }
        }
    }
    const float target = m_active ? kNoseUp + kLeanTilt * m_balance : 0.0f;
    m_tilt += (target - m_tilt) * std::min(1.0f, kTiltEase * dt);
}

void Manual::end(jak::Target& t, bool flush) {
    m_active = false;
    if (m_time >= kLeastTime) {
        jak::add_to_trick_list(t, jak::BoardTricks::board_manual, kPointsPerSecond * m_time);
    }
    trace("manual " + std::to_string(m_time) + " s");
    m_balance = 0.0f;
    m_time = 0.0f;
    if (flush) jak::flush_trick_list(t);
}

void Manual::finish(jak::Target& t) {
    if (m_active) end(t, true);
    m_air = 0.0f;
    m_tilt = 0.0f;
}

} // namespace rac1
