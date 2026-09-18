// Regression coverage for GDD §5's wave escalation curve ("every wave spawns 10%
// more enemies, rounded down") and per-wave enemy health growth. No framework
// dependency, same reasoning as flow_field_test.cpp.
#include <cmath>
#include <iostream>

#include "core/WaveEscalation.hpp"

namespace {

int failures = 0;

void check(bool condition, const char* description) {
    if (!condition) {
        std::cerr << "FAILED: " << description << "\n";
        ++failures;
    } else {
        std::cout << "ok: " << description << "\n";
    }
}

// This is the exact bug report: with the originally-configured base_enemy_count (5)
// and growth_rate_per_wave (0.10), floor(5 * 1.10) == floor(5.5) == 5 -- a fixed
// point. The wave count never grew, no matter how many waves passed.
void testOldConfigWouldHaveStalledWithoutTheGuard() {
    const int next = td::nextWaveEnemyCount(5, 0.10f);
    check(next > 5, "old config (base 5, rate 0.10): count still increases despite floor(5*1.10)==5");
    check(next == 6, "old config (base 5, rate 0.10): guard guarantees exactly +1 when floor() would stall");
}

// The currently-configured rate (see data/waves.json) shouldn't need the +1 guard at
// all from this starting point -- floor() growth alone should already exceed it.
void testCurrentConfigGrowsPastTheGuardFloor() {
    const int next = td::nextWaveEnemyCount(5, 0.25f);
    check(next == 6, "current config (base 5, rate 0.25): floor(5*1.25)==6 matches the guard, not overridden by it");
}

// Simulates the actual multi-wave recurrence (same call pattern as
// WaveDirector::startWave) for a long run and asserts it never plateaus, at any
// count -- not just the specific base/rate pair that originally broke.
void testEscalationNeverPlateausOverManyWaves() {
    int count = 5; // base_enemy_count
    bool everStalled = false;
    for (int wave = 2; wave <= 100; ++wave) {
        const int next = td::nextWaveEnemyCount(count, 0.25f);
        if (next <= count) {
            everStalled = true;
            break;
        }
        count = next;
    }
    check(!everStalled, "escalation: count strictly increases every wave across a 100-wave run");
    check(count > 100, "escalation: after 100 waves the count has grown well past its starting value");
}

void testWaveOneHealthIsExactlyBaseHealth() {
    const float health = td::escalatedEnemyHealth(10.0f, 0.15f, 1);
    check(health == 10.0f, "health escalation: wave 1 gets exactly the configured base health, no growth applied yet");
}

void testHealthCompoundsExponentiallyByWave() {
    const float wave3 = td::escalatedEnemyHealth(10.0f, 0.15f, 3);
    // wave 3 -> exponent 2: 10 * 1.15^2 == 13.225
    check(std::fabs(wave3 - 13.225f) < 0.001f, "health escalation: wave 3 matches base * (1+rate)^(wave-1)");
}

void testHealthNeverDecreasesAcrossWaves() {
    float previous = td::escalatedEnemyHealth(10.0f, 0.15f, 1);
    bool everDecreased = false;
    for (int wave = 2; wave <= 100; ++wave) {
        const float health = td::escalatedEnemyHealth(10.0f, 0.15f, wave);
        if (health < previous) {
            everDecreased = true;
            break;
        }
        previous = health;
    }
    check(!everDecreased, "health escalation: health is non-decreasing across a 100-wave run");
    check(previous > 10.0f * 100.0f, "health escalation: after 100 waves health has grown by more than 100x");
}

void testZeroGrowthRateHoldsHealthConstant() {
    // A 0% growth rate is a legitimate config value (health escalation off) and
    // shouldn't be treated as a bug the way the count's floor()-based growth was --
    // there's no fixed-point trap here to guard against (see WaveEscalation.hpp).
    check(td::escalatedEnemyHealth(10.0f, 0.0f, 50) == 10.0f,
          "health escalation: a 0% growth rate holds health at the base value indefinitely, by design");
}

} // namespace

int main() {
    testOldConfigWouldHaveStalledWithoutTheGuard();
    testCurrentConfigGrowsPastTheGuardFloor();
    testEscalationNeverPlateausOverManyWaves();
    testWaveOneHealthIsExactlyBaseHealth();
    testHealthCompoundsExponentiallyByWave();
    testHealthNeverDecreasesAcrossWaves();
    testZeroGrowthRateHoldsHealthConstant();

    if (failures > 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "All checks passed\n";
    return 0;
}
