#pragma once

namespace td {

// Wave N's enemy count grows by growthRatePerWave off wave N-1's, rounded down
// (GDD §5). A naive floor(count * (1 + rate)) can hit a fixed point and never
// escalate again once count * rate < 1 -- e.g. floor(5 * 1.10) == 5, forever,
// regardless of how many waves pass (this was a real bug: base_enemy_count 5 with
// growth_rate_per_wave 0.10 never grew). Since GDD §5 explicitly wants these numbers
// config-driven/tunable, guard against any future base-count/rate combination
// hitting the same trap by guaranteeing at least +1 per wave.
int nextWaveEnemyCount(int previousCount, float growthRatePerWave);

} // namespace td
