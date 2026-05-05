#include "WeakPvPModule.h"
#include <iostream>

using namespace BWAPI;

void WeakPvPModule::onStart()
{
    myStart = Broodwar->self()->getStartLocation();
    Broodwar->sendText("WeakPvP: GLHF!");
}

void WeakPvPModule::onEnd(bool isWinner)
{
    std::cout << "WeakPvP: " << (isWinner ? "Win" : "Loss") << std::endl;
}

void WeakPvPModule::onFrame()
{
    if (Broodwar->isReplay() || Broodwar->isPaused() || !Broodwar->self()) return;

    int frame = Broodwar->getFrameCount();
    int supplyUsed = Broodwar->self()->supplyUsed();
    int supplyTotal = Broodwar->self()->supplyTotal();

    // --- Worker management ---
    for (auto &u : Broodwar->self()->getUnits())
    {
        if (!u->exists()) continue;
        if (!u->getType().isWorker()) continue;
        if (u->isIdle())
        {
            if (u->isCarryingGas() || u->isCarryingMinerals())
                u->returnCargo();
            else
            {
                Unit mineral = u->getClosestUnit(Filter::IsMineralField);
                if (mineral) u->gather(mineral);
            }
        }
    }

    int minerals = Broodwar->self()->minerals();
    int probes = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Probe);
    int pylons = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Pylon);
    int gateways = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Gateway);
    int zealots = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Zealot);

    // --- Build order ---
    // Train probes until 16
    int targetProbes = 16;
    if (probes < targetProbes && supplyUsed < supplyTotal)
    {
        for (auto &u : Broodwar->self()->getUnits())
        {
            if (!u->exists()) continue;
            if (u->getType() != UnitTypes::Protoss_Nexus) continue;
            if (!u->isCompleted() || !u->isIdle()) continue;
            if (minerals < 50) break;
            if (supplyUsed >= supplyTotal) break;
            if (u->train(UnitTypes::Protoss_Probe))
            {
                supplyUsed += 2;
                probes++;
                if (probes >= targetProbes) break;
            }
        }
    }

    // Pylon: build at 8 supply
    if (pylons == 0 && supplyUsed >= 16 && minerals >= 100)
    {
        TilePosition buildPos = Broodwar->getBuildLocation(UnitTypes::Protoss_Pylon, myStart);
        Unit builder = nullptr;
        for (auto &u : Broodwar->self()->getUnits())
        {
            if (!u->exists()) continue;
            if (u->getType().isWorker() && !u->isConstructing())
            {
                builder = u;
                break;
            }
        }
        if (builder && buildPos.isValid())
        {
            builder->build(UnitTypes::Protoss_Pylon, buildPos);
            pylonBuilt++;
        }
    }

    // Gateway: build at 10 supply, second at 14
    int targetGates = (supplyUsed >= 28) ? 2 : 1;
    if (gateways < targetGates && pylons > 0 && minerals >= 150)
    {
        TilePosition buildPos = Broodwar->getBuildLocation(UnitTypes::Protoss_Gateway, myStart);
        Unit builder = nullptr;
        for (auto &u : Broodwar->self()->getUnits())
        {
            if (!u->exists()) continue;
            if (u->getType().isWorker() && !u->isConstructing())
            {
                builder = u;
                break;
            }
        }
        if (builder && buildPos.isValid())
        {
            builder->build(UnitTypes::Protoss_Gateway, buildPos);
            gatewayBuilt++;
        }
    }

    // Train Zealots from idle Gateways (disabled for testing)
    if (false && supplyTotal - supplyUsed >= 4 && minerals >= 100)
    {
        for (auto &u : Broodwar->self()->getUnits())
        {
            if (!u->exists()) continue;
            if (u->getType() != UnitTypes::Protoss_Gateway) continue;
            if (!u->isCompleted() || !u->isIdle()) continue;
            if (minerals < 100) break;
            if (supplyTotal - Broodwar->self()->supplyUsed() < 4) break;
            if (u->train(UnitTypes::Protoss_Zealot))
            {
                minerals -= 100;
                zealots++;
            }
        }
    }

    // --- Attack logic ---
    // Attack with 8+ Zealots, or attack after 7 minutes regardless
    bool shouldAttack = (zealots >= 8) || (frame > 24 * 60 * 7);
    if (shouldAttack && !attacked)
    {
        attacked = true;
    }

    if (attacked)
    {
        // Find enemy start or a known building
        BWAPI::Position attackTarget = BWAPI::Position(Broodwar->self()->getStartLocation());
        if (Broodwar->enemy())
        {
            for (auto &u : Broodwar->enemy()->getUnits())
            {
                if (!u->exists()) continue;
                if (u->getType().isBuilding())
                {
                    attackTarget = u->getPosition();
                    break;
                }
            }
            // Try enemy start location
            auto starts = Broodwar->getStartLocations();
            for (auto &s : starts)
            {
                if (s != myStart)
                {
                    attackTarget = BWAPI::Position(s);
                    break;
                }
            }
        }

        for (auto &u : Broodwar->self()->getUnits())
        {
            if (!u->exists()) continue;
            if (u->getType() != UnitTypes::Protoss_Zealot) continue;
            if (u->isIdle() || u->getOrder() == Orders::PlayerGuard)
            {
                u->attack(attackTarget, true);
            }
        }
    }
}
