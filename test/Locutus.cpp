#include "BWTest.h"
#include "AttackNearestModule.h"
#include "DoNothingModule.h"
#include "LocutusBotModule.h"
#include <limits>

TEST(Locutus, RunOne)
{
    BWTest test;
    test.maps = Maps::Get("aiide");
    test.opponentRace = BWAPI::Races::Protoss;
    test.opponentModule = []()
    {
        return new Locutus::LocutusBotModule();
    };
    test.onStartOpponent = []()
    {
        std::cout << "Locutus strategy: " << Locutus::LocutusBotModule::getStrategyName() << std::endl;

        std::cout.setstate(std::ios_base::failbit);
    };
    test.onEndMine = [&](bool won)
    {
        std::ostringstream replayName;
        replayName << "Locutus_" << test.map->shortname();
        if (!won)
        {
            replayName << "_LOSS";
        }
        replayName << "_" << test.randomSeed;
        test.replayName = replayName.str();
    };
    test.run();
}

TEST(Locutus, RunTwenty)
{
    int count = 0;
    int lost = 0;
    while (count < 20)
    {
        BWTest test;
        test.maps = Maps::Get("aiide");
        test.opponentRace = BWAPI::Races::Protoss;
        test.opponentModule = []()
        {
            return new Locutus::LocutusBotModule();
        };
        test.onStartOpponent = []()
        {
            std::cout << "Locutus strategy: " << Locutus::LocutusBotModule::getStrategyName() << std::endl;

            std::cout.setstate(std::ios_base::failbit);
        };
        test.onEndMine = [&](bool won)
        {
            std::ostringstream replayName;
            replayName << "Locutus_" << test.map->shortname();
            if (!won)
            {
                replayName << "_LOSS";
                lost++;
            }
            replayName << "_" << test.randomSeed;
            test.replayName = replayName.str();

            count++;
            std::cout << "---------------------------------------------" << std::endl;
            std::cout << "STATUS AFTER " << count << " GAME" << (count == 1 ? "" : "S") << ": "
                      << (count - lost) << " won; " << lost << " lost" << std::endl;
            std::cout << "---------------------------------------------" << std::endl;
        };
        test.expectWin = false;
        test.run();
    }
}

TEST(Locutus, 4GateGoon)
{
    BWTest test;
    test.opponentRace = BWAPI::Races::Protoss;
    test.opponentModule = []()
    {
        Locutus::LocutusBotModule::setStrategy("4GateGoon");
        return new Locutus::LocutusBotModule();
    };

    test.run();
}

TEST(Locutus, GasSteal4GateGoon)
{
    BWTest test;
    test.opponentRace = BWAPI::Races::Protoss;
    test.opponentModule = []()
    {
        Locutus::LocutusBotModule::setStrategy("4GateGoon");
        Locutus::LocutusBotModule::forceGasSteal();
        return new Locutus::LocutusBotModule();
    };

    test.run();
}

TEST(Locutus, ZealotDropRoadkillRegression)
{
    BWTest test;
    test.maps = Maps::Get("Roadkill");
    test.randomSeed = 40203;
    test.opponentRace = BWAPI::Races::Protoss;
    test.opponentModule = [seed = test.randomSeed]()
    {
        Locutus::LocutusBotModule::setRandomSeed(seed);
        Locutus::LocutusBotModule::setStrategy("ZealotDrop");
        return new Locutus::LocutusBotModule();
    };

    test.run();
}

TEST(Locutus, ZealotDropRoadkillDropDefenseRegression)
{
    BWTest test;
    test.maps = Maps::Get("Roadkill");
    test.randomSeed = 40203;
    test.opponentRace = BWAPI::Races::Protoss;
    test.opponentModule = [seed = test.randomSeed]()
    {
        Locutus::LocutusBotModule::setRandomSeed(seed);
        Locutus::LocutusBotModule::setStrategy("ZealotDrop");
        return new Locutus::LocutusBotModule();
    };
    test.frameLimit = 24 * 60 * 8;
    test.expectWin = false;
    test.writeReplay = false;

    test.onEndMine = [&](bool won)
    {
        EXPECT_GT(BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Forge) +
                  BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Photon_Cannon), 0);
        EXPECT_GE(BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Probe), 12);
        EXPECT_GE(BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Dragoon), 3);
    };

    test.run();
}

TEST(Combat, PrioritizesShuttleOverGroundThreatNearHome)
{
    BWTest test;
    test.map = Maps::GetOne("Python");
    test.opponentRace = BWAPI::Races::Protoss;
    test.opponentModule = []()
    {
        return new DoNothingModule();
    };
    test.frameLimit = 72;
    test.timeLimit = 10;
    test.expectWin = false;
    test.writeReplay = false;

    test.myInitialUnits = {
            UnitTypeAndPosition(BWAPI::UnitTypes::Protoss_Dragoon, BWAPI::TilePosition(46, 118)),
    };

    test.opponentInitialUnits = {
            UnitTypeAndPosition(BWAPI::UnitTypes::Protoss_Shuttle, BWAPI::TilePosition(48, 118)),
            UnitTypeAndPosition(BWAPI::UnitTypes::Protoss_Zealot, BWAPI::TilePosition(48, 119)),
    };

    bool sawDragoon = false;
    bool attackedShuttle = false;
    int zealotDamage = 0;
    test.onFrameMine = [&]()
    {
        BWAPI::Unit dragoon = nullptr;
        for (auto &unit : BWAPI::Broodwar->self()->getUnits())
        {
            if (unit->exists() && unit->getType() == BWAPI::UnitTypes::Protoss_Dragoon)
            {
                dragoon = unit;
                break;
            }
        }

        BWAPI::Unit shuttle = nullptr;
        BWAPI::Unit zealot = nullptr;
        for (auto &unit : BWAPI::Broodwar->enemy()->getUnits())
        {
            if (!unit->exists() || !unit->isVisible()) continue;
            if (unit->getType() == BWAPI::UnitTypes::Protoss_Shuttle) shuttle = unit;
            if (unit->getType() == BWAPI::UnitTypes::Protoss_Zealot) zealot = unit;
        }
        if (!dragoon || !shuttle || !zealot) return;

        sawDragoon = true;
        if (dragoon->getOrderTarget() == shuttle) attackedShuttle = true;
        zealotDamage = std::max(
                zealotDamage,
                (zealot->getType().maxHitPoints() + zealot->getType().maxShields())
                - (zealot->getHitPoints() + zealot->getShields()));
    };

    test.onEndMine = [&](bool won)
    {
        EXPECT_TRUE(sawDragoon);
        EXPECT_TRUE(attackedShuttle);
        EXPECT_EQ(zealotDamage, 0);
    };

    test.run();
}

TEST(Combat, ProbesSurviveMeleeHarassNearHome)
{
    BWTest test;
    test.map = Maps::GetOne("Python");
    test.opponentRace = BWAPI::Races::Protoss;
    test.opponentModule = []()
    {
        return new AttackNearestModule();
    };
    test.frameLimit = 144;
    test.timeLimit = 10;
    test.expectWin = false;
    test.writeReplay = false;

    test.opponentInitialUnits = {
            UnitTypeAndPosition(BWAPI::UnitTypes::Protoss_Zealot, BWAPI::TilePosition(47, 118)),
    };

    int minProbeCount = std::numeric_limits<int>::max();
    int maxProbeDamage = 0;
    test.onFrameMine = [&]()
    {
        int probeCount = 0;
        int probeDamage = 0;
        for (auto &unit : BWAPI::Broodwar->self()->getUnits())
        {
            if (!unit->exists() || unit->getType() != BWAPI::UnitTypes::Protoss_Probe) continue;
            probeCount++;
            probeDamage += (unit->getType().maxHitPoints() + unit->getType().maxShields())
                           - (unit->getHitPoints() + unit->getShields());
        }

        minProbeCount = std::min(minProbeCount, probeCount);
        maxProbeDamage = std::max(maxProbeDamage, probeDamage);
    };

    test.onEndMine = [&](bool won)
    {
        EXPECT_GE(minProbeCount, 4);
        EXPECT_LT(maxProbeDamage, 60);
    };

    test.run();
}
