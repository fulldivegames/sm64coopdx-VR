#include "game/vr_speedrun.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <filesystem>
int main(int argc, char** argv) {
    assert(argc == 2); // unique, previously absent export path supplied by runner
    vr_speedrun_basic(3);
    assert(vr_speedrun_total() == 3 && vr_speedrun_count() == 0);
    vr_speedrun_split(10);
    assert(vr_speedrun_running());
    assert(vr_speedrun_elapsed(15) == 5);
    vr_speedrun_split(15);
    assert(vr_speedrun_count() == 1);
    vr_speedrun_pause(20);
    assert(vr_speedrun_elapsed(100) == 10);
    vr_speedrun_split(100);
    assert(vr_speedrun_count() == 1); // no splitting a paused timer
    vr_speedrun_pause(100);
    vr_speedrun_split(102);
    vr_speedrun_split(105);
    assert(!vr_speedrun_running() && vr_speedrun_elapsed(200) == 15);
    assert(vr_speedrun_count() == 3);
    vr_speedrun_split(201);
    assert(vr_speedrun_count() == 3); // finished means finished until reset
    assert(vr_speedrun_import_lss("tests/fixtures/speedrun.lss"));
    assert(vr_speedrun_total() == 2 && vr_speedrun_count() == 0);
    assert(std::strcmp(vr_speedrun_name(), "Bob-omb & Battlefield") == 0);
    assert(std::abs(vr_speedrun_best() - 61.25) < 0.0001);
    assert(!vr_speedrun_load("tests/fixtures/speedrun-invalid.json"));
    assert(vr_speedrun_total() == 2); // invalid import is transactional
    assert(vr_speedrun_set_name(0, "First Star"));
    vr_speedrun_configure(16);
    assert(vr_speedrun_total() == 16 && vr_speedrun_count() == 0);
    assert(std::strcmp(vr_speedrun_segment_name(0), "First Star") == 0);
    assert(std::strcmp(vr_speedrun_segment_name(15), "Split 16") == 0);
    vr_speedrun_split(300);
    for (int i = 0; i < 16; ++i) {
        vr_speedrun_split(301 + i);
        assert(vr_speedrun_count() == (unsigned int)i + 1);
        assert(vr_speedrun_segment_time(i) == i + 1);
        assert(vr_speedrun_running() == (i < 15));
    }
    assert(vr_speedrun_segment_time(0) == 1); // completed first split remains available
    assert(!vr_speedrun_set_name(16, "Out of range"));
    vr_speedrun_configure(2);
    assert(std::strcmp(vr_speedrun_segment_name(0), "First Star") == 0);
    assert(vr_speedrun_save(argv[1]));
    assert(!vr_speedrun_save(argv[1])); // never silently overwrite a setup
    vr_speedrun_basic(120);
    assert(vr_speedrun_load(argv[1]) && vr_speedrun_total() == 2);
    vr_speedrun_reset();
    assert(!vr_speedrun_running() && vr_speedrun_elapsed(500) == 0);
    const auto localDir = std::filesystem::u8path(std::string(argv[1]) + ".state");
    assert(!std::filesystem::exists(localDir));
    std::filesystem::create_directory(localDir);
    const auto setupPath = (localDir / "setup.json").u8string();
    vr_speedrun_initialize(setupPath.c_str(), 4);
    assert(vr_speedrun_set_name(0, "Persistent Name"));
    assert(vr_speedrun_save_local());
    assert(vr_speedrun_set_name(0, "Updated Name"));
    assert(vr_speedrun_save_local()); // atomically replace only app-owned local setup
    vr_speedrun_basic(1);
    assert(vr_speedrun_load((localDir / "local.json").u8string().c_str()));
    assert(vr_speedrun_total() == 4);
    assert(std::strcmp(vr_speedrun_segment_name(0), "Updated Name") == 0);
    std::puts("PASS: 16 named splits, retained results, configure/reset, local persistence, LSS/JSON, no overwrite");
}
