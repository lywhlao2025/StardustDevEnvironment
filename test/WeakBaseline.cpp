#include "BWTest.h"
#include "DoNothingModule.h"
#include "UAlbertaBotModule.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <vector>
#include <cstdlib>

TEST(EasyBaseline, RunTwenty_DoNothing)
{
    int count = 0;
    int lost = 0;
    int targetGames = 20;
    if (const char *envGames = std::getenv("EASY_BASELINE_GAMES"))
    {
        targetGames = std::max(1, std::atoi(envGames));
    }
    std::vector<long long> durationsMs;
    durationsMs.reserve(targetGames);

    while (count < targetGames)
    {
        BWTest test;
        test.maps = Maps::Get("aiide");
        test.opponentRace = BWAPI::Races::Protoss;
        test.opponentModule = []()
        {
            return new DoNothingModule();
        };
        // short-game configuration for weak baseline throughput
        test.frameLimit = 16000;
        test.timeLimit = 180;

        auto start = std::chrono::steady_clock::now();
        test.onEndMine = [&](bool won)
        {
            auto end = std::chrono::steady_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            durationsMs.push_back(ms);

            std::ostringstream replayName;
            replayName << "Easy_DoNothing_" << test.map->shortname();
            if (!won)
            {
                replayName << "_LOSS";
                lost++;
            }
            replayName << "_" << test.randomSeed;
            test.replayName = replayName.str();

            count++;
            std::cout << "---------------------------------------------" << std::endl;
            std::cout << "EASY BASELINE STATUS AFTER " << count << " GAME" << (count == 1 ? "" : "S") << ": "
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
        std::cout << "EASY_BASELINE_P95_MS=" << durationsMs[idx] << std::endl;
        std::cout << "EASY_BASELINE_WINRATE=" << (100.0 * (targetGames - lost) / (double)targetGames) << std::endl;
    }
}

TEST(WeakBaseline, SteamhammerRunThirty)
{
    int count = 0;
    int lost = 0;
    std::vector<long long> durationsMs;
    durationsMs.reserve(30);

    while (count < 30)
    {
        BWTest test;
        test.opponentRace = BWAPI::Races::Zerg;
        test.maps = Maps::Get("aiide");
        test.opponentModule = []()
        {
            return new UAlbertaBot::UAlbertaBotModule();
        };

        auto start = std::chrono::steady_clock::now();
        test.onEndMine = [&](bool won)
        {
            auto end = std::chrono::steady_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            durationsMs.push_back(ms);

            std::ostringstream replayName;
            replayName << "WeakBaseline_Steamhammer_" << test.map->shortname();
            if (!won)
            {
                replayName << "_LOSS";
                lost++;
            }
            replayName << "_" << test.randomSeed;
            test.replayName = replayName.str();

            count++;
            std::cout << "WB30_GAME index=" << count
                      << " won=" << (won ? 1 : 0)
                      << " ms=" << ms
                      << " map=" << test.map->shortname()
                      << " seed=" << test.randomSeed
                      << std::endl;
        };

        test.expectWin = false;
        test.run();
    }

    std::sort(durationsMs.begin(), durationsMs.end());
    size_t idx = static_cast<size_t>(std::ceil(durationsMs.size() * 0.95)) - 1;
    idx = std::min(idx, durationsMs.size() - 1);
    std::cout << "WB30_SUMMARY wins=" << (30 - lost)
              << " losses=" << lost
              << " winrate=" << (100.0 * (30 - lost) / 30.0)
              << " p95_ms=" << durationsMs[idx]
              << std::endl;
}
