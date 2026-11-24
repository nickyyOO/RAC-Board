// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Mateusz Kłysz
// The best combo on each planet (rac1/fx/records.*): only a higher score is kept, bad lines in the
// file are skipped, and the text reads back the same.
#include "check.hpp"
#include "rac1/fx/records.hpp"

using namespace rac1;

int main() {
    Records r;
    r.parse("rac1 3 1200\nbroken line\nrac1 -1 50\nrac1 4 0\nrac1 3 900\nrac2 3 70\n");
    CHECK(r.best(Records::key("rac1", 3)) == 1200);
    CHECK(r.best(Records::key("rac1", 4)) == 0);
    CHECK(r.best(Records::key("rac2", 3)) == 70);
    CHECK(!r.offer(Records::key("rac1", 3), 1200));
    CHECK(r.offer(Records::key("rac1", 3), 1201));
    CHECK(r.offer(Records::key("rac1", 7), 10));
    Records back;
    back.parse(r.text());
    CHECK(back.best(Records::key("rac1", 3)) == 1201);
    CHECK(back.best(Records::key("rac1", 7)) == 10);
    CHECK(back.text() == r.text());
    TEST_MAIN_END();
}
