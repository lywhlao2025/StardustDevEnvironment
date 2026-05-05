#include "BWTest.h"
#include "WeakPvPModule.h"

#include <algorithm>
#include <chrono>
#include <vector>

TEST(WeakPvP, RunTen)
{
    int count = 0;
    int lost = 0;
    std::vector<long long> durationsMs;

    while (count < 10)
    {
        BWTest test;
        test.maps = Maps::Get("aiide");
        test.opponentRace = BWAPI::Races::Protoss;
        test.opponentModule = []()
        {
            return new WeakPvPModule();
        };

        auto start = std::chrono::steady_clock::now();
        test.onEndMine = [&](bool won)
        {
            auto end = std::chrono::steady_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            durationsMs.push_back(ms);

            std::ostringstream replayName;
            replayName << "WeakPvP_" << test.map->shortname();
            if (!won)
            {
                replayName << "_LOSS";
                lost++;
            }
            replayName << "_" << test.randomSeed;
            test.replayName = replayName.str();

            count++;
            std::cout << "---------------------------------------------" << std::endl;
            std::cout << "WEAK PVP STATUS AFTER " << count << " GAME" << (count == 1 ? "" : "S") << ": "
                      << (count - lost) << " won; " << lost << " lost"
                      << "; last_ms=" << ms
                      << std::endl;
            std::cout << "---------------------------------------------" << std::endl;
        };

        test.expectWin = false;
        test.run();
    }

    if (!durationsMs.empty())
    {
        std::sort(durationsMs.begin(), durationsMs.end());
        size_t idx = static_cast<size_t>(std::ceil(durationsMs.size() * 0.95)) - 1;
        idx = std::min(idx, durationsMs.size() - 1);
        std::cout << "WEAK_PVP_P95_MS=" << durationsMs[idx] << std::endl;
        std::cout << "WEAK_PVP_WINRATE=" << (100.0 * (10 - lost) / 10.0) << std::endl;
    }
}
