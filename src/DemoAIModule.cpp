#include "DemoAIModule.h"
#include <iostream>
#include <limits>
#include <cmath>
#include <string>
#include <algorithm>
#include <vector>

#include "Log.h"
#include "CherryVis.h"

using namespace BWAPI;
using namespace Filter;

namespace
{
    // ===== 内部工具函数区域 =====
    // 这些函数不直接暴露给模块外部，主要用于 onFrame 的决策支持。
    const int kScoutIntervalFrames = 24;
    const int kFramesPerSecond = 24;

    enum ThreatBits
    {
        Threat_None = 0,
        Threat_FastExpand = 1 << 0,
        Threat_Proxy = 1 << 1,
        Threat_TechRush = 1 << 2,
        Threat_Cloak = 1 << 3,
        Threat_Air = 1 << 4
    };

    enum ResponseBits
    {
        Resp_None = 0,
        Resp_Scout = 1 << 0,
        Resp_Defend = 1 << 1,
        Resp_DelayExpand = 1 << 2,
        Resp_IncreaseProd = 1 << 3,
        Resp_TechShift = 1 << 4,
        Resp_Counter = 1 << 5
    };

    struct BuildMilestone
    {
        UnitType type;
        int count;
        int frame; // trigger after this frame
    };

    // PvP early-game timing template (median timings from replay stats)
    const BuildMilestone kPvPMilestones[] = {
        {UnitTypes::Protoss_Pylon, 1, 48 * kFramesPerSecond},
        {UnitTypes::Protoss_Gateway, 1, 76 * kFramesPerSecond},
        {UnitTypes::Protoss_Assimilator, 1, 105 * kFramesPerSecond},
        {UnitTypes::Protoss_Cybernetics_Core, 1, 157 * kFramesPerSecond},
        // Extreme-defense: early Forge + double Cannon, then detection chain
        {UnitTypes::Protoss_Forge, 1, 185 * kFramesPerSecond},
        {UnitTypes::Protoss_Photon_Cannon, 2, 230 * kFramesPerSecond},
        {UnitTypes::Protoss_Robotics_Facility, 1, 260 * kFramesPerSecond},
        {UnitTypes::Protoss_Observatory, 1, 300 * kFramesPerSecond}
    };

    // 统计我方某类单位数量；completedOnly=true 时仅统计已完成单位。
    int countUnits(UnitType type, bool completedOnly = false)
    {
        if (!Broodwar->self()) return 0;
        if (completedOnly) return Broodwar->self()->completedUnitCount(type);
        return Broodwar->self()->allUnitCount(type);
    }

    // 找到离目标建造点最近的可用 Probe；可排除指定工人（例如侦查工人）。
    Unit findWorkerClosestTo(TilePosition target, Unit exclude = nullptr)
    {
        Unit best = nullptr;
        double bestDist = std::numeric_limits<double>::max();
        for (auto &u : Broodwar->self()->getUnits())
        {
            if (!u->exists()) continue;
            if (!u->getType().isWorker()) continue;
            if (u == exclude) continue;
            if (u->isConstructing()) continue;

            double d = u->getPosition().getDistance(Position(target));
            if (d < bestDist)
            {
                bestDist = d;
                best = u;
            }
        }
        return best;
    }

    // 尝试在 near 附近下建筑：检查资源、找建造工人、找合法地块并发出 build 指令。
    bool tryBuild(UnitType type, TilePosition near, Unit builder = nullptr)
    {
        if (Broodwar->self()->minerals() < type.mineralPrice()) return false;
        if (Broodwar->self()->gas() < type.gasPrice()) return false;

        // Do not spam failed build commands every frame. Repeated failed build
        // attempts pull many Probes off minerals and were starving Zealot
        // production on some maps. Keep this cooldown per game: GoogleTest runs
        // multiple games in one process, while BWAPI frame count resets to 0.
        static int lastAttemptFrame[256] = {};
        static int lastSeenFrame = -1;
        int typeId = type.getID();
        int frame = Broodwar->getFrameCount();
        if (frame < lastSeenFrame)
        {
            std::fill(std::begin(lastAttemptFrame), std::end(lastAttemptFrame), -1000000);
        }
        lastSeenFrame = frame;
        if (typeId >= 0 && typeId < 256 && frame - lastAttemptFrame[typeId] < 48) return false;
        if (!builder) builder = findWorkerClosestTo(near);
        if (!builder) return false;

        TilePosition buildPos = Broodwar->getBuildLocation(type, near);
        bool needsPower = type.getRace() == Races::Protoss && type.isBuilding() &&
                          type != UnitTypes::Protoss_Pylon && type != UnitTypes::Protoss_Nexus &&
                          type != UnitTypes::Protoss_Assimilator;
        auto isGoodBuildPos = [&](TilePosition p) {
            if (!p || !p.isValid()) return false;
            if (!Broodwar->canBuildHere(p, type, builder, false)) return false;
            if (needsPower && !Broodwar->hasPower(p, type)) return false;
            return true;
        };
        if (!isGoodBuildPos(buildPos))
        {
            buildPos = TilePositions::Invalid;
            for (int r = 1; r <= 80 && !buildPos; ++r)
            {
                for (int dx = -r; dx <= r && !buildPos; ++dx)
                {
                    for (int dy = -r; dy <= r && !buildPos; ++dy)
                    {
                        if (std::abs(dx) != r && std::abs(dy) != r) continue;
                        TilePosition p(near.x + dx, near.y + dy);
                        if (!p.isValid()) continue;
                        if (isGoodBuildPos(p))
                        {
                            buildPos = p;
                        }
                    }
                }
            }
        }
        if (!buildPos) return false;

        bool issued = builder->build(type, buildPos);
        if (issued && typeId >= 0 && typeId < 256) lastAttemptFrame[typeId] = frame;
        return issued;
    }

    // 根据节奏模板补齐关键建筑（用于 PvP 早期时间点约束）。
    void applyBuildMilestones(const BuildMilestone *milestones, int count, TilePosition near)
    {
        if (!Broodwar->self()) return;
        int frame = Broodwar->getFrameCount();
        for (int i = 0; i < count; ++i)
        {
            const auto &m = milestones[i];
            if (frame < m.frame) continue;

            int current = countUnits(m.type);
            int inProgress = Broodwar->self()->incompleteUnitCount(m.type);
            if (current + inProgress >= m.count) continue;

            tryBuild(m.type, near);
        }
    }

    // 找到一个空闲且已完成的 Nexus，用于持续补 Probe。
    Unit findIdleNexus()
    {
        for (auto &u : Broodwar->self()->getUnits())
        {
            if (!u->exists()) continue;
            if (u->getType() == UnitTypes::Protoss_Nexus && u->isCompleted() && u->isIdle())
            {
                return u;
            }
        }
        return nullptr;
    }

    // 找到空闲且已完成的 Cybernetics Core，用于升级（如 Dragoon 射程）。
    Unit findIdleCore()
    {
        for (auto &u : Broodwar->self()->getUnits())
        {
            if (!u->exists()) continue;
            if (u->getType() == UnitTypes::Protoss_Cybernetics_Core && u->isCompleted() && !u->isUpgrading() && u->isIdle())
            {
                return u;
            }
        }
        return nullptr;
    }

    // 统计敌方可见单位中某个类型的数量（用于敌情判定）。
    int countEnemy(UnitType type)
    {
        int count = 0;
        if (!Broodwar->enemy()) return 0;
        for (auto &u : Broodwar->enemy()->getUnits())
        {
            if (!u->exists()) continue;
            if (u->getType() == type) count++;
        }
        return count;
    }

    double weaponDps(UnitType t, bool vsAir)
    {
        WeaponType w = vsAir ? t.airWeapon() : t.groundWeapon();
        if (w == WeaponTypes::None) return 0.0;
        double dmg = w.damageAmount() * w.damageFactor();
        double cooldown = std::max(1, w.damageCooldown());
        return dmg / cooldown;
    }

    double unitCombatValue(Unit u)
    {
        if (!u || !u->exists()) return 0.0;
        UnitType t = u->getType();
        if (t.isWorker()) return 0.35;
        if (t.isBuilding()) return 0.1;

        double base = 1.0;
        if (t == UnitTypes::Protoss_Zealot) base = 1.5;
        else if (t == UnitTypes::Protoss_Dragoon) base = 2.2;
        else if (t == UnitTypes::Protoss_High_Templar) base = 2.4;
        else if (t == UnitTypes::Protoss_Dark_Templar) base = 2.6;
        else if (t.isFlyer()) base = 1.7;

        double dps = weaponDps(t, false) + weaponDps(t, true);
        double range = std::max(t.groundWeapon().maxRange(), t.airWeapon().maxRange());
        double rangeBonus = range > 0 ? (range / 256.0) : 0.0; // 1 tile = 32, 256 ~ 8 tiles

        double maxHp = t.maxHitPoints() + t.maxShields();
        double hp = u->getHitPoints() + u->getShields();
        double ratio = (maxHp > 0) ? (hp / maxHp) : 1.0;

        double power = base + dps * 1.8 + rangeBonus * 0.8;
        return power * (0.45 + 0.55 * ratio);
    }

    double computeLocalPower(const Unitset &units, const Position &center, int radius)
    {
        double power = 0.0;
        for (auto &u : units)
        {
            if (!u->exists()) continue;
            if (!u->getType().canAttack() && !u->getType().isWorker()) continue;
            double dist = u->getDistance(center);
            if (dist > radius) continue;
            double falloff = 1.0 - std::min(0.6, dist / std::max(1.0, (double)radius));
            power += unitCombatValue(u) * falloff;
        }
        return power;
    }

    bool isEnemyProxyNear(const Position &home)
    {
        if (!Broodwar->enemy()) return false;
        for (auto &u : Broodwar->enemy()->getUnits())
        {
            if (!u->exists()) continue;
            if (!u->getType().isBuilding()) continue;
            if (u->getDistance(home) < 1200) return true;
        }
        return false;
    }

    int computeThreatMask(int enemyNexus, int enemyGateways, int enemyZealots, int enemyDragoons, int enemyStargates, int enemyRobotics, int enemyCores, int enemyCitadel, int enemyTemplarArchives, int enemyDT, int enemyObserver, const Position &home)
    {
        int frame = Broodwar->getFrameCount();
        int mask = Threat_None;

        if (enemyNexus >= 2 && frame < 24 * 60 * 8)
        {
            mask |= Threat_FastExpand;
        }
        if (isEnemyProxyNear(home) || (enemyGateways >= 2 && frame < 24 * 60 * 5))
        {
            mask |= Threat_Proxy;
        }
        if ((enemyCores >= 1 && enemyGateways >= 1 && frame < 24 * 60 * 6) || enemyCitadel > 0 || enemyTemplarArchives > 0 || (enemyRobotics > 0 && frame < 24 * 60 * 7))
        {
            mask |= Threat_TechRush;
        }
        if (enemyDT > 0 || enemyTemplarArchives > 0)
        {
            mask |= Threat_Cloak;
        }
        if (enemyStargates > 0 || enemyObserver > 0)
        {
            mask |= Threat_Air;
        }
        if (enemyDragoons >= 4 && frame < 24 * 60 * 7)
        {
            mask |= Threat_TechRush;
        }
        if (enemyZealots >= 4 && frame < 24 * 60 * 6)
        {
            mask |= Threat_Proxy;
        }
        return mask;
    }

    int mapThreatToResponse(int mask)
    {
        int resp = Resp_None;
        if (mask & (Threat_Proxy | Threat_TechRush | Threat_Cloak | Threat_Air)) resp |= Resp_Defend;
        if (mask & (Threat_Cloak | Threat_TechRush | Threat_Air)) resp |= Resp_TechShift;
        if (mask & Threat_FastExpand) resp |= Resp_IncreaseProd;
        if (mask & Threat_Proxy) resp |= Resp_Scout;
        if (mask & Threat_TechRush) resp |= Resp_DelayExpand;
        return resp;
    }

    int pendingSupplyFromPylons()
    {
        if (!Broodwar->self()) return 0;
        int incomplete = Broodwar->self()->incompleteUnitCount(UnitTypes::Protoss_Pylon);
        return incomplete * 8;
    }

    bool enemyPresenceNearHome(const Position &home, int radius)
    {
        if (!Broodwar->enemy()) return false;
        for (auto &u : Broodwar->enemy()->getUnits())
        {
            if (!u->exists()) continue;
            if (u->getType().isWorker()) continue;
            if (u->getDistance(home) <= radius) return true;
        }
        return false;
    }

    bool isMeleeThreatOnWorkers(Unit enemy)
    {
        if (!enemy || !enemy->exists() || !Broodwar->self()) return false;
        if (enemy->isFlying()) return false;
        if (!enemy->getType().canAttack()) return false;
        if (enemy->getType().groundWeapon() == WeaponTypes::None) return false;
        if (enemy->getType().groundWeapon().maxRange() > 40) return false;

        for (auto &u : Broodwar->self()->getUnits())
        {
            if (!u->exists()) continue;
            if (u->getType() != UnitTypes::Protoss_Probe) continue;
            if (enemy->getDistance(u) <= 128) return true;
        }
        return false;
    }

    // 为单个作战单位选择目标：优先高价值/低血/就近。
    Unit pickCombatTarget(Unit unit)
    {
        Unit best = nullptr;
        double bestScore = std::numeric_limits<double>::max();
        for (auto &enemy : Broodwar->enemy()->getUnits())
        {
            if (!enemy->exists()) continue;
            if (!enemy->isVisible()) continue;
            double dist = unit->getDistance(enemy);
            bool threat = enemy->getType().canAttack();
            bool transport = enemy->getType() == UnitTypes::Protoss_Shuttle ||
                             enemy->getType() == UnitTypes::Terran_Dropship;
            bool meleeThreatOnWorkers = isMeleeThreatOnWorkers(enemy);
            double value = 1.0;
            if (enemy->getType().isWorker()) value = 0.8;
            if (enemy->getType().isBuilding()) value = 0.6;
            if (enemy->getType() == UnitTypes::Protoss_Dragoon) value = 2.0;
            if (enemy->getType() == UnitTypes::Protoss_Zealot) value = 1.4;
            if (enemy->getType() == UnitTypes::Protoss_High_Templar) value = 2.2;
            if (enemy->getType() == UnitTypes::Protoss_Dark_Templar) value = 2.4;
            if (transport) value = 3.2;
            if (meleeThreatOnWorkers) value = std::max(value, 5.5);

            double hpRatio = 1.0;
            double maxHp = enemy->getType().maxHitPoints() + enemy->getType().maxShields();
            if (maxHp > 0)
            {
                hpRatio = (enemy->getHitPoints() + enemy->getShields()) / maxHp;
            }
            double lowHpBonus = (hpRatio < 0.35) ? -64.0 : 0.0;
            double threatPenalty = transport ? -96.0 : (threat ? 0.0 : 48.0);
            if (meleeThreatOnWorkers) threatPenalty -= 96.0;

            double score = dist + threatPenalty - (value * 40.0) + lowHpBonus;
            if (score < bestScore)
            {
                bestScore = score;
                best = enemy;
            }
        }
        return best;
    }

    // 计算集火目标：围绕中心点、优先高威胁与低血
    [[maybe_unused]] Unit pickFocusTarget(const Position &center, int radius)
    {
        Unit best = nullptr;
        double bestScore = std::numeric_limits<double>::max();
        for (auto &enemy : Broodwar->enemy()->getUnits())
        {
            if (!enemy->exists()) continue;
            if (!enemy->isVisible()) continue;
            double dist = enemy->getDistance(center);
            if (dist > radius) continue;

            bool transport = enemy->getType() == UnitTypes::Protoss_Shuttle ||
                             enemy->getType() == UnitTypes::Terran_Dropship;
            bool meleeThreatOnWorkers = isMeleeThreatOnWorkers(enemy);
            double value = 1.0;
            if (enemy->getType().isWorker()) value = 0.7;
            if (enemy->getType().isBuilding()) value = 0.5;
            if (enemy->getType() == UnitTypes::Protoss_Dragoon) value = 2.2;
            if (enemy->getType() == UnitTypes::Protoss_Zealot) value = 1.5;
            if (enemy->getType() == UnitTypes::Protoss_High_Templar) value = 2.6;
            if (enemy->getType() == UnitTypes::Protoss_Dark_Templar) value = 2.8;
            if (transport) value = 3.4;
            if (meleeThreatOnWorkers) value = std::max(value, 5.8);

            double maxHp = enemy->getType().maxHitPoints() + enemy->getType().maxShields();
            double hpRatio = (maxHp > 0) ? (enemy->getHitPoints() + enemy->getShields()) / maxHp : 1.0;
            double lowHpBonus = (hpRatio < 0.4) ? -96.0 : 0.0;
            double threatBonus = transport ? -120.0 : (enemy->getType().canAttack() ? -32.0 : 12.0);
            if (meleeThreatOnWorkers) threatBonus -= 128.0;

            double score = dist - (value * 55.0) + lowHpBonus + threatBonus;
            if (score < bestScore)
            {
                bestScore = score;
                best = enemy;
            }
        }
        return best;
    }

    Unit closestMineralTo(Unit worker)
    {
        Unit best = nullptr;
        double bestDist = std::numeric_limits<double>::max();
        for (auto &m : Broodwar->getMinerals())
        {
            if (!m || !m->exists()) continue;
            double d = worker->getDistance(m);
            if (d < bestDist)
            {
                bestDist = d;
                best = m;
            }
        }
        if (!best && worker)
        {
            best = worker->getClosestUnit(IsMineralField);
        }
        return best;
    }

    Unit closestCompletedAssimilatorTo(Unit worker)
    {
        Unit best = nullptr;
        double bestDist = std::numeric_limits<double>::max();
        for (auto &u : Broodwar->self()->getUnits())
        {
            if (!u->exists()) continue;
            if (u->getType() != UnitTypes::Protoss_Assimilator || !u->isCompleted()) continue;
            double d = worker->getDistance(u);
            if (d < bestDist)
            {
                bestDist = d;
                best = u;
            }
        }
        return best;
    }

    // 维持基础采矿循环：非侦查工人空闲时自动回收/采矿。
    void keepMining(Unit scout)
    {
        for (auto &u : Broodwar->self()->getUnits())
        {
            if (!u->exists()) continue;
            if (!u->getType().isWorker()) continue;
            if (u == scout) continue;
            if (!u->isIdle()) continue;

            if (u->isCarryingGas() || u->isCarryingMinerals())
            {
                u->returnCargo();
            }
            else
            {
                Unit mineral = closestMineralTo(u);
                if (mineral) u->gather(mineral);
            }
        }
    }

    // 维持采气人数。PvP 早期可主动降气，把工人拉回矿线，避免矿不够出 Core/Dragoon/防守建筑。
    void manageGas(Unit scout, int desiredGasWorkers)
    {
        int assimilators = 0;
        for (auto &u : Broodwar->self()->getUnits())
        {
            if (!u->exists()) continue;
            if (u->getType() == UnitTypes::Protoss_Assimilator && u->isCompleted()) assimilators++;
        }
        if (assimilators == 0) return;

        int gasWorkers = 0;
        std::vector<Unit> assignedGasWorkers;
        for (auto &u : Broodwar->self()->getUnits())
        {
            if (!u->exists()) continue;
            if (!u->getType().isWorker()) continue;
            if (u->isGatheringGas() || u->isCarryingGas())
            {
                gasWorkers++;
                assignedGasWorkers.push_back(u);
            }
        }

        desiredGasWorkers = std::max(0, std::min(desiredGasWorkers, 3 * assimilators));
        if (gasWorkers > desiredGasWorkers)
        {
            for (auto &u : assignedGasWorkers)
            {
                if (!u->exists()) continue;
                if (u == scout) continue;
                if (u->isConstructing()) continue;
                if (u->isCarryingGas())
                {
                    u->returnCargo();
                    gasWorkers--;
                    if (gasWorkers <= desiredGasWorkers) break;
                    continue;
                }

                Unit mineral = closestMineralTo(u);
                if (mineral && u->gather(mineral))
                {
                    gasWorkers--;
                    if (gasWorkers <= desiredGasWorkers) break;
                }
            }
        }

        if (gasWorkers >= desiredGasWorkers) return;

        for (auto &u : Broodwar->self()->getUnits())
        {
            if (!u->exists()) continue;
            if (!u->getType().isWorker()) continue;
            if (u == scout) continue;
            if (u->isConstructing()) continue;
            if (!u->isIdle() && !u->isGatheringMinerals()) continue;

            Unit refinery = closestCompletedAssimilatorTo(u);
            if (refinery)
            {
                u->gather(refinery);
                gasWorkers++;
                if (gasWorkers >= desiredGasWorkers) break;
            }
        }
    }

    // 根据敌方起点计算一个前置锚点（当前版本主要用于预留扩展策略位置）。
    TilePosition pickProxyAnchor(const TilePosition &enemyStart)
    {
        if (!enemyStart) return TilePositions::Invalid;
        TilePosition candidate(enemyStart.x + 6, enemyStart.y + 6);
        if (Broodwar->isBuildable(candidate)) return candidate;
        return enemyStart;
    }

    int completedCount(UnitType type)
    {
        return Broodwar->self() ? Broodwar->self()->completedUnitCount(type) : 0;
    }

    TilePosition firstCompletedPylonTile()
    {
        if (!Broodwar->self()) return TilePositions::Invalid;
        for (auto &u : Broodwar->self()->getUnits())
        {
            if (!u->exists()) continue;
            if (u->getType() == UnitTypes::Protoss_Pylon && u->isCompleted()) return u->getTilePosition();
        }
        return TilePositions::Invalid;
    }

    int desiredProbeCount()
    {
        int completedNexus = std::max(1, completedCount(UnitTypes::Protoss_Nexus));
        int completedGas = completedCount(UnitTypes::Protoss_Assimilator);
        int desired = completedNexus * 18 + completedGas * 3;
        if (completedNexus <= 1) desired = std::max(desired, 24);
        return std::min(50, desired);
    }

    bool hasNexusNear(TilePosition tile)
    {
        if (!tile) return true;
        Position pos(tile);
        for (auto &u : Broodwar->self()->getUnits())
        {
            if (!u->exists()) continue;
            if (u->getType() != UnitTypes::Protoss_Nexus) continue;
            if (u->getDistance(pos) < 640) return true;
        }
        return false;
    }

    TilePosition chooseExpansionLocation(TilePosition myStart, TilePosition enemyStart)
    {
        TilePosition best = TilePositions::Invalid;
        double bestDist = std::numeric_limits<double>::max();
        for (auto &start : Broodwar->getStartLocations())
        {
            if (!start || start == myStart || (enemyStart && start == enemyStart)) continue;
            if (hasNexusNear(start)) continue;
            TilePosition buildPos = Broodwar->getBuildLocation(UnitTypes::Protoss_Nexus, start);
            if (!buildPos) continue;
            double dist = Position(myStart).getDistance(Position(buildPos));
            if (dist < bestDist)
            {
                bestDist = dist;
                best = buildPos;
            }
        }
        return best;
    }

    Position tileCenter(TilePosition tile)
    {
        return Position(tile.x * 32 + 16, tile.y * 32 + 16);
    }

    Position startLocationCenter(TilePosition tile)
    {
        // Start locations are resource-depot top-left tiles; send units near the depot center
        // instead of the exact top-left build tile, which can be unwalkable in OpenBW pathing.
        return Position(tile.x * 32 + 64, tile.y * 32 + 48);
    }

    // 将一个世界坐标映射到最近的开局点（用于从敌方建筑反推敌方起点）。
    TilePosition closestStartLocation(const Position &pos)
    {
        TilePosition best = TilePositions::Invalid;
        double bestDist = std::numeric_limits<double>::max();
        for (auto &start : Broodwar->getStartLocations())
        {
            double d = Position(start).getDistance(pos);
            if (d < bestDist)
            {
                bestDist = d;
                best = start;
            }
        }
        return best;
    }
}

// 游戏开始时初始化：日志、可视化、以及策略状态变量。
void DemoAIModule::onStart()
{
    Log::initialize();
    CherryVis::initialize();

    Log::Get() << "Started game on " << Broodwar->mapName() << " against " << Broodwar->enemy()->getName();

    myStart = Broodwar->self()->getStartLocation();
    enemyStart = TilePositions::Invalid;
    proxyAnchor = TilePositions::Invalid;
    scoutIndex = 0;
    lastScoutFrame = 0;
    proxyPylonStarted = false;
    proxyPylonCompleted = false;
    scoutProbeId = -1;

    threatMask = 0;
    responseMask = 0;
    supplyBlockedFrames = 0;
    idleGatewayFrames = 0;
    idleCoreFrames = 0;
    idleNexusFrames = 0;
    workerCountAt5 = workerCountAt8 = workerCountAt10 = workerCountAt12 = -1;
    armyCountAt5 = armyCountAt6 = armyCountAt7 = armyCountAt8 = -1;
    gatewayCountAt5 = gatewayCountAt6 = gatewayCountAt7 = gatewayCountAt8 = -1;
    firstProxyFrame = firstTechFrame = firstExpandFrame = -1;
    lastFrameCount = Broodwar->getFrameCount();
    lastEnemyPressureFrame = -1;
    lastEnemyBuildingTile = TilePositions::Invalid;
    armySearchIndex = 0;
    lastArmySearchFrame = 0;
    focusTargetId = -1;
    focusTargetFrame = 0;
    retreatUntilFrame = 0;
    committedAttack = false;
}

void DemoAIModule::onEnd(bool isWinner)
{
    auto frameToSec = [](int frame) { return frame / 24; };
    auto frameToMin = [](int frame) { return frame / (24.0 * 60.0); };

    Log::Get() << "DIAG workers@5/8/10/12: "
               << workerCountAt5 << "/" << workerCountAt8 << "/" << workerCountAt10 << "/" << workerCountAt12;
    Log::Get() << "DIAG army@5/6/7/8: "
               << armyCountAt5 << "/" << armyCountAt6 << "/" << armyCountAt7 << "/" << armyCountAt8
               << " gates@5/6/7/8: "
               << gatewayCountAt5 << "/" << gatewayCountAt6 << "/" << gatewayCountAt7 << "/" << gatewayCountAt8;
    Log::Get() << "DIAG final supply=" << Broodwar->self()->supplyUsed() << "/" << Broodwar->self()->supplyTotal()
               << " probes=" << Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Probe)
               << " zealots=" << Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Zealot)
               << " dragoons=" << Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Dragoon)
               << " gates=" << Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Gateway)
               << " minerals=" << Broodwar->self()->minerals()
               << " gas=" << Broodwar->self()->gas();
    Log::Get() << "DIAG supplyBlockedSec=" << frameToSec(supplyBlockedFrames)
               << " idleFrames(Gateway/Core/Nexus)=" << idleGatewayFrames << "/" << idleCoreFrames << "/" << idleNexusFrames;

    Log::Get() << "DIAG firstScout proxy/tech/expand(min)="
               << (firstProxyFrame >= 0 ? frameToMin(firstProxyFrame) : -1.0) << "/"
               << (firstTechFrame >= 0 ? frameToMin(firstTechFrame) : -1.0) << "/"
               << (firstExpandFrame >= 0 ? frameToMin(firstExpandFrame) : -1.0);

    std::string failReason = "Win";
    if (!isWinner)
    {
        if (supplyBlockedFrames > 24 * 60) failReason = "SupplyBlocked";
        else if (workerCountAt8 >= 0 && workerCountAt8 < 16) failReason = "LowEcon";
        else if (threatMask & Threat_Proxy) failReason = "ProxyPressure";
        else if (threatMask & Threat_TechRush) failReason = "TechRush";
        else if (threatMask & Threat_FastExpand) failReason = "OutExpanded";
        else failReason = "OutFought";
    }
    Log::Get() << "DIAG result=" << (isWinner ? "Win" : "Loss") << " reason=" << failReason;

    CherryVis::gameEnd();
}

// 主循环（每帧调用一次）：
// 1) 信息更新（敌方起点/侦查）
// 2) 经济与建造（采矿、气矿、补给、科技、产能）
// 3) 训练与升级（Probe / Zealot / Dragoon / Range）
// 4) 战斗决策（守家 or 前压）与基础微操
void DemoAIModule::onFrame()
{
    if (Broodwar->isReplay() || Broodwar->isPaused() || !Broodwar->self()) return;

    int frame = Broodwar->getFrameCount();
    int frameDelta = frame - lastFrameCount;
    if (frameDelta < 0) frameDelta = 0;
    lastFrameCount = frame;

    // Diagnostics: supply block + idle production
    int supplyUsed = Broodwar->self()->supplyUsed();
    int supplyTotal = Broodwar->self()->supplyTotal();
    if ((supplyTotal - supplyUsed) <= 0)
    {
        supplyBlockedFrames += frameDelta;
    }

    for (auto &u : Broodwar->self()->getUnits())
    {
        if (!u->exists() || !u->isCompleted()) continue;
        if (u->getType() == UnitTypes::Protoss_Gateway && u->isIdle()) idleGatewayFrames += frameDelta;
        if (u->getType() == UnitTypes::Protoss_Cybernetics_Core && u->isIdle()) idleCoreFrames += frameDelta;
        if (u->getType() == UnitTypes::Protoss_Nexus && u->isIdle()) idleNexusFrames += frameDelta;
    }

    if (workerCountAt5 < 0 && frame >= 5 * 60 * 24) workerCountAt5 = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Probe);
    if (workerCountAt8 < 0 && frame >= 8 * 60 * 24) workerCountAt8 = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Probe);
    if (workerCountAt10 < 0 && frame >= 10 * 60 * 24) workerCountAt10 = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Probe);
    if (workerCountAt12 < 0 && frame >= 12 * 60 * 24) workerCountAt12 = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Probe);
    int currentArmyCount = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Zealot) +
                           Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Dragoon);
    int currentGatewayCount = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Gateway);
    if (armyCountAt5 < 0 && frame >= 5 * 60 * 24)
    {
        armyCountAt5 = currentArmyCount;
        gatewayCountAt5 = currentGatewayCount;
    }
    if (armyCountAt6 < 0 && frame >= 6 * 60 * 24)
    {
        armyCountAt6 = currentArmyCount;
        gatewayCountAt6 = currentGatewayCount;
    }
    if (armyCountAt7 < 0 && frame >= 7 * 60 * 24)
    {
        armyCountAt7 = currentArmyCount;
        gatewayCountAt7 = currentGatewayCount;
    }
    if (armyCountAt8 < 0 && frame >= 8 * 60 * 24)
    {
        armyCountAt8 = currentArmyCount;
        gatewayCountAt8 = currentGatewayCount;
    }

    // Detect enemy start if known
    if (!enemyStart)
    {
        TilePosition known = Broodwar->enemy()->getStartLocation();
        bool knownIsRealStart = false;
        for (auto &start : Broodwar->getStartLocations())
        {
            if (known && known == start && known != myStart)
            {
                knownIsRealStart = true;
                break;
            }
        }
        if (knownIsRealStart) enemyStart = known;
    }

    // On 2-player maps the enemy start is deterministic even if BWAPI has not
    // revealed it yet. Lock this immediately so scouts/army do not wander or
    // attack an imprecise fallback point until the frame limit.
    if (!enemyStart)
    {
        TilePosition onlyOtherStart = TilePositions::Invalid;
        int otherStarts = 0;
        for (auto &start : Broodwar->getStartLocations())
        {
            if (!start || start == myStart) continue;
            onlyOtherStart = start;
            otherStarts++;
        }
        if (otherStarts == 1) enemyStart = onlyOtherStart;
    }

    if (!enemyStart)
    {
        for (auto &u : Broodwar->enemy()->getUnits())
        {
            if (!u->exists()) continue;
            if (!u->getType().isBuilding()) continue;
            lastEnemyBuildingTile = u->getTilePosition();
            enemyStart = closestStartLocation(u->getPosition());
            break;
        }
    }

    if (enemyStart && !proxyAnchor)
    {
        proxyAnchor = pickProxyAnchor(enemyStart);
    }

    // Manage scouting
    BWAPI::Unit scoutProbe = (scoutProbeId >= 0) ? Broodwar->getUnit(scoutProbeId) : nullptr;
    if (!scoutProbe || !scoutProbe->exists() || scoutProbe->getPlayer() != Broodwar->self())
    {
        // pick a worker as scout
        scoutProbe = findWorkerClosestTo(myStart);
        scoutProbeId = scoutProbe ? scoutProbe->getID() : -1;
    }

    if (scoutProbe && enemyStart == TilePositions::Invalid)
    {
        if (Broodwar->getFrameCount() - lastScoutFrame >= kScoutIntervalFrames)
        {
            lastScoutFrame = Broodwar->getFrameCount();

            auto starts = Broodwar->getStartLocations();
            if (!starts.empty())
            {
                int attempts = 0;
                while (attempts < (int)starts.size())
                {
                    TilePosition target = starts[scoutIndex % starts.size()];
                    scoutIndex++;
                    attempts++;
                    if (target == myStart) continue;
                    if (!Broodwar->isExplored(target) || Broodwar->isVisible(target))
                    {
                        scoutProbe->move(startLocationCenter(target));
                        break;
                    }
                }
            }
        }
    }

    // Proxy pylon once enemy is found. PvP needs a proactive cheese option against Locutus pressure.
    if (enemyStart && !proxyPylonStarted && scoutProbe && Broodwar->self()->minerals() >= 100)
    {
        TilePosition proxyPos = Broodwar->getBuildLocation(UnitTypes::Protoss_Pylon, proxyAnchor);
        if (proxyPos.isValid() && scoutProbe->build(UnitTypes::Protoss_Pylon, proxyPos))
        {
            proxyPylonStarted = true;
        }
    }

    if (proxyPylonStarted && !proxyPylonCompleted && enemyStart)
    {
        for (auto &u : Broodwar->self()->getUnits())
        {
            if (!u->exists()) continue;
            if (u->getType() != UnitTypes::Protoss_Pylon) continue;
            if (!u->isCompleted()) continue;
            if (u->getDistance(Position(enemyStart)) < 1200)
            {
                proxyPylonCompleted = true;
                break;
            }
        }
    }

    // Basic worker mining
    keepMining(scoutProbe);

    // ===== 经济与建造阶段 =====
    // Build order + supply
    int pylons = countUnits(UnitTypes::Protoss_Pylon);
    int gateways = countUnits(UnitTypes::Protoss_Gateway);
    int cores = countUnits(UnitTypes::Protoss_Cybernetics_Core);
    int assimilators = countUnits(UnitTypes::Protoss_Assimilator);
    int forges = countUnits(UnitTypes::Protoss_Forge);
    int cannons = countUnits(UnitTypes::Protoss_Photon_Cannon);
    int completedCannons = countUnits(UnitTypes::Protoss_Photon_Cannon, true);
    int robotics = countUnits(UnitTypes::Protoss_Robotics_Facility);
    int observatories = countUnits(UnitTypes::Protoss_Observatory);
    int observers = countUnits(UnitTypes::Protoss_Observer);
    int dragoons = countUnits(UnitTypes::Protoss_Dragoon);
    int zealots = countUnits(UnitTypes::Protoss_Zealot);
    int completedCores = countUnits(UnitTypes::Protoss_Cybernetics_Core, true);

    int enemyZealots = countEnemy(UnitTypes::Protoss_Zealot);
    int enemyDragoons = countEnemy(UnitTypes::Protoss_Dragoon);
    int enemyGateways = countEnemy(UnitTypes::Protoss_Gateway);
    int enemyNexus = countEnemy(UnitTypes::Protoss_Nexus);
    int enemyCores = countEnemy(UnitTypes::Protoss_Cybernetics_Core);
    int enemyRobotics = countEnemy(UnitTypes::Protoss_Robotics_Facility);
    int enemyStargates = countEnemy(UnitTypes::Protoss_Stargate);
    int enemyCitadel = countEnemy(UnitTypes::Protoss_Citadel_of_Adun);
    int enemyTemplarArchives = countEnemy(UnitTypes::Protoss_Templar_Archives);
    int enemyDT = countEnemy(UnitTypes::Protoss_Dark_Templar);
    int enemyObservers = countEnemy(UnitTypes::Protoss_Observer);

    threatMask = computeThreatMask(enemyNexus, enemyGateways, enemyZealots, enemyDragoons, enemyStargates, enemyRobotics, enemyCores, enemyCitadel, enemyTemplarArchives, enemyDT, enemyObservers, Position(myStart));
    responseMask = mapThreatToResponse(threatMask);

    if (enemyPresenceNearHome(Position(myStart), 1200))
    {
        responseMask |= Resp_Defend;
    }

    if ((threatMask & Threat_Proxy) && firstProxyFrame < 0) firstProxyFrame = frame;
    if ((threatMask & (Threat_TechRush | Threat_Cloak | Threat_Air)) && firstTechFrame < 0) firstTechFrame = frame;
    if ((threatMask & Threat_FastExpand) && firstExpandFrame < 0) firstExpandFrame = frame;

    bool wantScoutEnemy = (responseMask & Resp_Scout) || (enemyStart && frame < 24 * 60 * 6);
    if (wantScoutEnemy && scoutProbe && enemyStart && (scoutProbe->isIdle() || scoutProbe->getOrder() == Orders::PlayerGuard))
    {
        scoutProbe->move(startLocationCenter(enemyStart));
    }

    // 敌情标签：用于后续分支决策（防守/进攻/科技）
    bool enemyNearHome = enemyPresenceNearHome(startLocationCenter(myStart), 1200);
    bool enemyRush = (enemyZealots >= 3 && frame < 9000) ||
                     (enemyGateways >= 2 && frame < 7000) || (threatMask & Threat_Proxy);
    bool enemyPressure = enemyNearHome || enemyRush || (threatMask & Threat_Proxy);
    bool enemyGreed = (enemyNexus >= 2 && Broodwar->getFrameCount() < 9000);
    bool enemyProtoss = (Broodwar->enemy() && Broodwar->enemy()->getRace() == BWAPI::Races::Protoss);
    bool earlyPvP = enemyProtoss && frame < 24 * 60 * 7;
    bool roboticsDropThreat = enemyProtoss && enemyRobotics > 0 && frame < 24 * 60 * 9;
    if (enemyPressure && lastEnemyPressureFrame < frame) lastEnemyPressureFrame = frame;

    int supplyRemaining = (supplyTotal - supplyUsed);
    int pylonInProgress = Broodwar->self()->incompleteUnitCount(UnitTypes::Protoss_Pylon);
    int incomingSupply = pendingSupplyFromPylons();
    int projectedSupply = supplyRemaining + incomingSupply;

    bool urgentSupply = supplyRemaining <= 4;
    bool needSupply = projectedSupply <= 12 || (projectedSupply <= 14 && gateways >= 2) || (projectedSupply <= 18 && gateways >= 3);
    if ((needSupply || urgentSupply) && pylonInProgress == 0)
    {
        TilePosition near = myStart;
        tryBuild(UnitTypes::Protoss_Pylon, near);
    }
    // If Nexus is idle due to low supply, force an extra pylon
    if (supplyRemaining <= 2 && pylonInProgress == 0 && findIdleNexus())
    {
        tryBuild(UnitTypes::Protoss_Pylon, myStart);
    }
    // PvP gateway floods hit the supply cap quickly; queue pylons before the bank idles production.
    int pylonBufferThreshold = (earlyPvP && gateways >= 3) ? 28 : 18;
    int pylonBufferCap = (earlyPvP && gateways >= 3) ? 3 : 2;
    int pylonBufferMinerals = (earlyPvP && gateways >= 3) ? 150 : 200;
    if ((projectedSupply <= pylonBufferThreshold) && pylonInProgress < pylonBufferCap &&
        Broodwar->self()->minerals() >= pylonBufferMinerals)
    {
        tryBuild(UnitTypes::Protoss_Pylon, myStart);
    }

    bool proxyMode = proxyPylonStarted || proxyPylonCompleted;
    TilePosition gatewayBuildNear = firstCompletedPylonTile();
    if (!gatewayBuildNear) gatewayBuildNear = myStart;
    int nexusCount = countUnits(UnitTypes::Protoss_Nexus);
    int probeCount = countUnits(UnitTypes::Protoss_Probe);
    int targetEarlyZealots = 0;
    int targetOpeningDragoons = 0;
    if (earlyPvP)
    {
        targetEarlyZealots = (frame < 24 * 60 * 6) ? 6 : 4;
        if (enemyRush || (threatMask & Threat_TechRush)) targetEarlyZealots = 6;
        if (enemyNearHome || enemyDragoons > 0) targetEarlyZealots = 8;
        if (enemyRobotics > 0) targetEarlyZealots = std::min(targetEarlyZealots, 4);

        targetOpeningDragoons = ((threatMask & Threat_TechRush) || enemyRobotics > 0 || enemyDragoons > 0 || enemyNearHome) ? 4 : 2;
    }
    bool proactiveDropDefense = enemyProtoss &&
                                completedCores > 0 &&
                                (dragoons >= 3 || (dragoons >= 2 && frame >= 24 * 60 * 6));
    bool pvpPreferRangedMidgame = enemyProtoss &&
                                  completedCores > 0 &&
                                  (proactiveDropDefense || enemyRobotics > 0 || enemyDragoons > 0 || dragoons < 6);
    int basePvPDefensiveCannons = 0;
    if (enemyProtoss)
    {
        if (enemyNearHome || proactiveDropDefense) basePvPDefensiveCannons = std::max(basePvPDefensiveCannons, 2);
        if (enemyRobotics > 0 || roboticsDropThreat) basePvPDefensiveCannons = std::max(basePvPDefensiveCannons, 3);
        if (roboticsDropThreat && completedCores > 0 && dragoons >= 2)
        {
            basePvPDefensiveCannons = std::max(basePvPDefensiveCannons, 3);
        }
    }
    bool pvpOneBaseDropDefense = enemyProtoss &&
                                 nexusCount < 2 &&
                                 frame < 24 * 60 * 10 &&
                                 (roboticsDropThreat ||
                                  proactiveDropDefense ||
                                  enemyNearHome);
    int cannonReadyShortfall = std::max(0, basePvPDefensiveCannons - completedCannons);
    int defensiveCannonMineralReserve = 100 * cannonReadyShortfall;
    if (enemyNearHome && cannonReadyShortfall > 0)
    {
        defensiveCannonMineralReserve = 50 * cannonReadyShortfall;
    }
    bool reserveForFirstDragoons = earlyPvP &&
                                   completedCores > 0 &&
                                   dragoons < targetOpeningDragoons;
    bool zealotFloorNeeded = earlyPvP && zealots < targetEarlyZealots;
    int desiredGasWorkers = 3 * countUnits(UnitTypes::Protoss_Assimilator, true);
    if (earlyPvP && desiredGasWorkers > 0)
    {
        desiredGasWorkers = 2;
        if (completedCores > 0 && dragoons < targetOpeningDragoons)
        {
            desiredGasWorkers = Broodwar->self()->gas() < 150 ? 3 : 1;
        }
        if (completedCores > 0 && (enemyRobotics > 0 || roboticsDropThreat) && dragoons < 6)
        {
            desiredGasWorkers = std::max(desiredGasWorkers, 2);
        }
        if (completedCores > 0 && dragoons >= 2 && Broodwar->self()->gas() > Broodwar->self()->minerals() + 150)
        {
            desiredGasWorkers = 1;
        }
        if (completedCores > 0 && Broodwar->self()->gas() >= 100 && Broodwar->self()->minerals() < 200)
        {
            desiredGasWorkers = dragoons < 2 ? 1 : 0;
        }
    }
    manageGas(scoutProbe, desiredGasWorkers);

    // Economy first: spend the first 50 minerals on a Probe before tech/army can consume it.
    // This is the main fix for the old 13-Probe stall against Easy/DoNothing.
    int probeQueued = 0;
    int desiredProbes = desiredProbeCount();
    if (earlyPvP)
    {
        int pvpProbeCap = 16;
        if (gateways >= 2 && zealots >= 6) pvpProbeCap = 18;
        if ((zealots + dragoons) >= 12) pvpProbeCap = 20;
        desiredProbes = std::min(desiredProbes, pvpProbeCap);
    }
    if (pvpOneBaseDropDefense)
    {
        desiredProbes = std::min(desiredProbes, 18);
    }

    bool prioritizeEarlyArmy = earlyPvP && gateways >= 1 && zealots < targetEarlyZealots;
    if (prioritizeEarlyArmy && supplyRemaining >= 4)
    {
        for (auto &u : Broodwar->self()->getUnits())
        {
            if (!u->exists()) continue;
            if (u->getType() != UnitTypes::Protoss_Gateway) continue;
            if (!u->isCompleted() || !u->isIdle()) continue;
            if (!Broodwar->hasPower(u->getTilePosition())) continue;
            if (Broodwar->self()->minerals() < 100 || supplyRemaining < 4) break;
            if (u->train(UnitTypes::Protoss_Zealot))
            {
                zealots++;
                supplyRemaining -= 4;
                if (zealots >= targetEarlyZealots) break;
            }
        }
    }

    bool holdExtraProbes = prioritizeEarlyArmy && probeCount >= 16;
    if (reserveForFirstDragoons && probeCount >= 16) holdExtraProbes = true;
    if (earlyPvP && cores == 0 && countUnits(UnitTypes::Protoss_Assimilator, true) >= 1 && Broodwar->self()->gas() >= 80 && probeCount >= 16)
    {
        holdExtraProbes = true;
    }
    if (pvpOneBaseDropDefense && probeCount >= 18) holdExtraProbes = true;
    if (pvpOneBaseDropDefense && enemyNearHome && probeCount >= 12) holdExtraProbes = true;
    if (!holdExtraProbes && probeCount < desiredProbes && supplyRemaining >= 2)
    {
        for (auto &u : Broodwar->self()->getUnits())
        {
            if (!u->exists()) continue;
            if (u->getType() != UnitTypes::Protoss_Nexus) continue;
            if (!u->isCompleted() || !u->isIdle()) continue;
            if (Broodwar->self()->minerals() < UnitTypes::Protoss_Probe.mineralPrice()) break;
            if (u->train(UnitTypes::Protoss_Probe))
            {
                probeQueued++;
                probeCount++;
                supplyRemaining -= 2;
                if (probeCount >= desiredProbes || supplyRemaining < 2) break;
            }
        }
    }

    int proxyGateways = 0;

    for (auto &u : Broodwar->self()->getUnits())
    {
        if (!u->exists()) continue;
        if (u->getType() == UnitTypes::Protoss_Gateway && u->isCompleted() && !u->isPowered() && Broodwar->self()->minerals() >= 100)
        {
            tryBuild(UnitTypes::Protoss_Pylon, u->getTilePosition());
        }
    }

    // Against Easy/DoNothing the previous bot lost mostly because it started tech/army
    // while the Nexus sat idle. Keep worker production as top priority, but do NOT
    // hard-return: military production and build order must progress in parallel,
    // otherwise pre-built Gateways in scenario tests sit idle.

    if (enemyStart)
    {
        for (auto &u : Broodwar->self()->getUnits())
        {
            if (!u->exists()) continue;
            if (u->getType() != UnitTypes::Protoss_Gateway) continue;
            if (u->getDistance(Position(enemyStart)) < 1600) proxyGateways++;
        }
    }

    // Easy baseline killer: once the economy is alive, use idle Gateways immediately.
    // Do this before gas/tech/building spends so minerals become fighting units.
    // Do not stop at a tiny army: the 16-Zealot cap allowed huge mineral banks
    // and frame-limit losses on Easy/DoNothing.
    if (!reserveForFirstDragoons &&
        !pvpPreferRangedMidgame &&
        zealots < 48 &&
        supplyRemaining >= 4 &&
        Broodwar->self()->minerals() >= 100)
    {
        for (auto &u : Broodwar->self()->getUnits())
        {
            if (!u->exists()) continue;
            if (u->getType() != UnitTypes::Protoss_Gateway) continue;
            if (!u->isCompleted() || !u->isIdle()) continue;
            if (!Broodwar->hasPower(u->getTilePosition())) continue;
            if (Broodwar->self()->minerals() < 100 || supplyRemaining < 4) break;
            if (u->train(UnitTypes::Protoss_Zealot))
            {
                zealots++;
                supplyRemaining -= 4;
            }
        }
    }

    // PvP early timing template (from replay stats) to avoid being late
    if (enemyProtoss && !proxyMode && zealots >= 48)
    {
        applyBuildMilestones(kPvPMilestones, sizeof(kPvPMilestones) / sizeof(BuildMilestone), myStart);
    }

    // Opening: 2-Gate -> Gas -> Core (稳健PvP节奏)
    if (pylons == 0 && supplyUsed >= 8 * 2)
    {
        tryBuild(UnitTypes::Protoss_Pylon, myStart);
    }

    if (gateways == 0 && supplyUsed >= 10 * 2 && countUnits(UnitTypes::Protoss_Pylon, true) > 0)
    {
        tryBuild(UnitTypes::Protoss_Gateway, gatewayBuildNear);
    }

    if (pylons < 2 && gateways >= 1 && supplyUsed >= 14 * 2)
    {
        tryBuild(UnitTypes::Protoss_Pylon, myStart);
    }

    if (earlyPvP && gateways < 2 && pylons >= 1 && supplyUsed >= 12 * 2)
    {
        tryBuild(UnitTypes::Protoss_Gateway, gatewayBuildNear);
    }

    if (gateways < 2 && pylons >= 2 && supplyUsed >= 16 * 2)
    {
        tryBuild(UnitTypes::Protoss_Gateway, gatewayBuildNear);
    }

    if (proxyPylonCompleted && proxyGateways < 2 && Broodwar->self()->minerals() >= 150)
    {
        tryBuild(UnitTypes::Protoss_Gateway, proxyAnchor, scoutProbe);
    }

    // Economy-first gas: only take gas after we have 2 Gateways and at least
    // some fighting units OR if we are past the early macro phase.
    bool readyForGas = (gateways >= 2 && (zealots >= 2 || frame > 24 * 60 * 4)) || probeCount >= 24;
    if (earlyPvP)
    {
        int gasArmyGate = ((threatMask & Threat_TechRush) || enemyRobotics > 0 || enemyDragoons > 0) ? 6 : 8;
        readyForGas = gateways >= 3 && probeCount >= 18 && zealots >= gasArmyGate;
        if (gateways >= 2 && probeCount >= 13)
        {
            readyForGas = true;
        }
        if (gateways >= 2 && probeCount >= 15 && zealots >= 2)
        {
            readyForGas = true;
        }
    }
    int gasSupplyGate = earlyPvP ? 14 * 2 : 18 * 2;
    if (assimilators == 0 && readyForGas && supplyUsed >= gasSupplyGate && (!proxyMode || proxyGateways >= 2))
    {
        tryBuild(UnitTypes::Protoss_Assimilator, myStart);
    }

    bool coreInProgress = Broodwar->self()->incompleteUnitCount(UnitTypes::Protoss_Cybernetics_Core) > 0;
    bool readyForCore = cores == 0 &&
                        !coreInProgress &&
                        countUnits(UnitTypes::Protoss_Assimilator, true) >= 1 &&
                        pylons >= (earlyPvP ? 1 : 2) &&
                        (!proxyMode || proxyGateways >= 2);
    if (readyForCore &&
        (!earlyPvP ||
         gateways >= 2 ||
         enemyDragoons > 0))
    {
        tryBuild(UnitTypes::Protoss_Cybernetics_Core, myStart);
    }

    bool earlyAntiCloak = enemyProtoss && ((threatMask & Threat_Cloak) || enemyCitadel > 0 || enemyTemplarArchives > 0);

    // Detection chain: after Core, prioritize Robotics -> Observatory.
    // Build earlier if cloak threat detected; otherwise wait until we have a modest army.
    bool needRobotics = (enemyProtoss && (threatMask & (Threat_Cloak | Threat_Air))) ||
                        (!pvpOneBaseDropDefense && dragoons >= 6);
    if (cores > 0 && robotics == 0 && needRobotics && Broodwar->self()->minerals() >= 200 && Broodwar->self()->gas() >= 100)
    {
        tryBuild(UnitTypes::Protoss_Robotics_Facility, myStart);
    }
    if (robotics > 0 && observatories == 0 && Broodwar->self()->minerals() >= 50 && Broodwar->self()->gas() >= 100)
    {
        tryBuild(UnitTypes::Protoss_Observatory, myStart);
    }

    // Early PvP safety: prepare static defense for drop timing, but avoid spending the mineral line dead.
    if (enemyProtoss)
    {
        bool dropDefenseReady = roboticsDropThreat && completedCores > 0 && dragoons >= 2;
        bool earlyThreat = enemyNearHome || proactiveDropDefense || dropDefenseReady || (threatMask & (Threat_Proxy | Threat_Cloak));
        bool pvpForgeTiming = (enemyRobotics > 0 || roboticsDropThreat) && gateways >= 2 && zealots >= 4 && frame >= 24 * 60 * 4;
        if (forges == 0 && pylons >= 1 && (earlyThreat || pvpForgeTiming))
        {
            tryBuild(UnitTypes::Protoss_Forge, myStart);
        }
        int desiredCannons = basePvPDefensiveCannons;
        if (earlyAntiCloak) desiredCannons += 1;
        if (responseMask & Resp_Defend) desiredCannons += 1;
        if (threatMask & Threat_Cloak) desiredCannons += 1;
        desiredCannons = std::min(desiredCannons, 3);
        if (enemyNearHome && cannons > 0) desiredCannons = cannons;
        if (forges > 0 && desiredCannons > 0 && cannons < desiredCannons)
        {
            tryBuild(UnitTypes::Protoss_Photon_Cannon, myStart);
        }
    }

    bool pvpGatewayCatchup = earlyPvP &&
                             gateways < 4 &&
                             pylons >= 2 &&
                             probeCount >= 16 &&
                             (enemyRush ||
                              enemyDragoons > 0 ||
                              (threatMask & Threat_TechRush) ||
                              frame >= 24 * 60 * 4);
    if (pvpGatewayCatchup && Broodwar->self()->minerals() >= 150)
    {
        tryBuild(UnitTypes::Protoss_Gateway, gatewayBuildNear);
    }

    if (enemyRush && gateways < 3 && pylons >= 2 && (!earlyPvP || cores > 0 || coreInProgress))
    {
        tryBuild(UnitTypes::Protoss_Gateway, gatewayBuildNear);
    }

    if (gateways < 3 && pylons >= 3 && supplyUsed >= 20 * 2 && Broodwar->self()->minerals() >= 300 &&
        (!earlyPvP || cores > 0 || coreInProgress))
    {
        tryBuild(UnitTypes::Protoss_Gateway, gatewayBuildNear);
    }

    // Spend large mineral banks on production first. Easy/DoNothing losses were
    // caused by 3 Gateways floating 3000+ minerals, so add up to 6 Gateways
    // before expansion/tech if the economy is already at the 24-Probe target.
    if (gateways < 6 && pylons >= 2 && probeCount >= 24 && Broodwar->self()->minerals() >= 800)
    {
        if (tryBuild(UnitTypes::Protoss_Gateway, gatewayBuildNear)) gateways++;
    }

    if (gateways < 4 && dragoons >= 3 && Broodwar->self()->minerals() >= 300)
    {
        tryBuild(UnitTypes::Protoss_Gateway, gatewayBuildNear);
    }

    if ((responseMask & Resp_IncreaseProd) && gateways < 4 && pylons >= 3 && Broodwar->self()->minerals() >= 300)
    {
        tryBuild(UnitTypes::Protoss_Gateway, gatewayBuildNear);
    }

    if ((enemyNearHome || proactiveDropDefense || roboticsDropThreat || (threatMask & Threat_Cloak) ||
         ((enemyRobotics > 0 || roboticsDropThreat) && gateways >= 2 && zealots >= 4 && frame >= 24 * 60 * 4)) &&
        forges == 0 && pylons >= 1)
    {
        tryBuild(UnitTypes::Protoss_Forge, myStart);
    }

    // Fallback detection defense if the enemy is Protoss and we are mid-game
    if (enemyProtoss && zealots >= 24)
    {
        if (forges == 0 && pylons >= 1 && Broodwar->getFrameCount() > 4800)
        {
            tryBuild(UnitTypes::Protoss_Forge, myStart);
        }
        if (forges > 0 && cannons < 1 && Broodwar->self()->minerals() >= 150)
        {
            tryBuild(UnitTypes::Protoss_Photon_Cannon, myStart);
        }
    }

    if ((enemyNearHome || proactiveDropDefense || roboticsDropThreat || (threatMask & Threat_Cloak)) &&
        forges > 0 &&
        cannons < std::max(2, basePvPDefensiveCannons) &&
        Broodwar->self()->minerals() >= 150)
    {
        tryBuild(UnitTypes::Protoss_Photon_Cannon, myStart);
    }

    // Economy: add a natural/nearby expansion before the one-base economy stalls out.
    // The old bot often sat on one Nexus with 13-19 probes forever; this gives the
    // macro engine room to keep producing workers and army.
    bool safeToExpand = !enemyRush && !(responseMask & Resp_Defend);
    if (nexusCount < 2 && probeCount >= 24 && zealots >= 24 && safeToExpand && Broodwar->self()->minerals() >= 400)
    {
        TilePosition expandPos = chooseExpansionLocation(myStart, enemyStart);
        if (expandPos) tryBuild(UnitTypes::Protoss_Nexus, expandPos);
    }
    if (nexusCount < 2 && probeCount >= 30 && Broodwar->self()->minerals() >= 600)
    {
        TilePosition expandPos = chooseExpansionLocation(myStart, enemyStart);
        if (expandPos) tryBuild(UnitTypes::Protoss_Nexus, expandPos);
    }

    // Worker production fallback in case a Nexus became idle after an earlier order failed.
    // desiredProbes/probeQueued are computed in the economy-first block above.
    if (!holdExtraProbes && probeCount < desiredProbes && supplyRemaining >= 2)
    {
        int availableSupply = supplyRemaining / 2;
        for (auto &u : Broodwar->self()->getUnits())
        {
            if (!u->exists()) continue;
            if (u->getType() != UnitTypes::Protoss_Nexus) continue;
            if (!u->isCompleted() || !u->isIdle()) continue;
            if (availableSupply <= 0) break;
            if (u->train(UnitTypes::Protoss_Probe))
            {
                availableSupply--;
                probeCount++;
                probeQueued++;
                if (probeCount >= desiredProbes) break;
            }
        }
    }

    // Dragoon range timing
    if (cores > 0 && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Singularity_Charge) == 0)
    {
        Unit core = findIdleCore();
        if (core && Broodwar->self()->gas() >= 150 && Broodwar->self()->minerals() >= 150 &&
            (dragoons >= 2 || enemyDragoons > 0 || enemyGreed))
        {
            core->upgrade(UpgradeTypes::Singularity_Charge);
        }
    }

    // ===== 训练阶段 =====
    // 先补观察者，确保隐形单位可被侦测
    int desiredObservers = (responseMask & Resp_TechShift) ? 2 : 1;
    if (earlyAntiCloak && desiredObservers < 2) desiredObservers = 2;
    if (observatories > 0 && observers < desiredObservers && Broodwar->self()->minerals() >= 25 && Broodwar->self()->gas() >= 75)
    {
        for (auto &u : Broodwar->self()->getUnits())
        {
            if (!u->exists()) continue;
            if (u->getType() != UnitTypes::Protoss_Robotics_Facility) continue;
            if (!u->isCompleted() || !u->isIdle()) continue;
            u->train(UnitTypes::Protoss_Observer);
            break;
        }
    }

    if (reserveForFirstDragoons && !zealotFloorNeeded && supplyRemaining >= 4)
    {
        for (auto &u : Broodwar->self()->getUnits())
        {
            if (!u->exists()) continue;
            if (u->getType() != UnitTypes::Protoss_Gateway) continue;
            if (!u->isCompleted() || !u->isIdle()) continue;
            if (!Broodwar->hasPower(u->getTilePosition())) continue;
            if (Broodwar->self()->minerals() < 125 || Broodwar->self()->gas() < 50 || supplyRemaining < 4) break;
            if (u->train(UnitTypes::Protoss_Dragoon))
            {
                dragoons++;
                supplyRemaining -= 4;
                if (dragoons >= targetOpeningDragoons) break;
            }
        }
    }

    // 根据敌情与当前兵力配比在 Gateway 训练 Zealot / Dragoon。
    // 目标：前期抗压、中期转 Dragoon 形成持续火力。
    // Train units
    int availableArmySupply = (supplyRemaining / 2) - probeQueued;
    if (availableArmySupply < 0) availableArmySupply = 0;
    bool reserveForCore = earlyPvP &&
                          cores == 0 &&
                          countUnits(UnitTypes::Protoss_Assimilator, true) >= 1 &&
                          zealots >= 4 &&
                          Broodwar->self()->gas() >= 100 &&
                          gateways >= 2;
    int mineralReserve = reserveForCore ? 200 : 0;
    if (!(earlyPvP && cores > 0 && dragoons < targetOpeningDragoons))
    {
        mineralReserve = std::max(mineralReserve, defensiveCannonMineralReserve);
    }
    for (auto &u : Broodwar->self()->getUnits())
    {
        if (!u->exists()) continue;
        if (u->getType() != UnitTypes::Protoss_Gateway) continue;
        if (!u->isCompleted() || !u->isIdle()) continue;
        if (availableArmySupply <= 0) break;

        if (proxyMode && Broodwar->self()->minerals() >= 100)
        {
            if (u->train(UnitTypes::Protoss_Zealot))
            {
                availableArmySupply--;
            }
            continue;
        }

        if (enemyRush && zealots < 4 && Broodwar->self()->minerals() >= 100)
        {
            if (u->train(UnitTypes::Protoss_Zealot))
            {
                availableArmySupply--;
            }
            continue;
        }

        bool wantGoon = (cores > 0 && Broodwar->self()->gas() >= 50);
        if (zealotFloorNeeded)
        {
            wantGoon = false;
        }
        bool roboticsDragoonPriority = enemyProtoss &&
                                       cores > 0 &&
                                       (roboticsDropThreat || enemyRobotics > 0) &&
                                       dragoons < 6;
        if ((reserveForFirstDragoons && !zealotFloorNeeded) || roboticsDragoonPriority)
        {
            wantGoon = true;
        }
        if (wantGoon && dragoons < (zealots * 2 + 2))
        {
            if (Broodwar->self()->minerals() >= 125 + mineralReserve)
            {
                if (u->train(UnitTypes::Protoss_Dragoon))
                {
                    availableArmySupply--;
                    dragoons++;
                }
            }
        }
        else if (!reserveForFirstDragoons && !roboticsDragoonPriority && Broodwar->self()->minerals() >= 100 + mineralReserve)
        {
            if (u->train(UnitTypes::Protoss_Zealot))
            {
                availableArmySupply--;
                zealots++;
            }
        }
    }

    // ===== 战斗阶段 =====
    // 自适应进攻阈值：对无威胁对手尽早进攻；面对 rush 则积累优势兵力。
    int armyCount = dragoons + zealots;
    int minAttackThreshold = 8;  // previously 24, lowered for aggressive play
    if (Broodwar->enemy()->getRace() == Races::None) minAttackThreshold = 4;
    else if (enemyGreed) minAttackThreshold = 6;
    else if (enemyRush) minAttackThreshold = enemyNearHome ? 16 : 8;
    else if (enemyPressure) minAttackThreshold = 10;
    if (earlyPvP && ((threatMask & Threat_TechRush) || enemyDragoons > 0))
    {
        minAttackThreshold = std::max(minAttackThreshold, 14);
    }
    if (earlyPvP && !enemyGreed && frame < 24 * 60 * 8)
    {
        minAttackThreshold = std::max(minAttackThreshold, 20);
    }
    bool canProactiveAttack = (armyCount >= minAttackThreshold);
    bool recentHomePressure = lastEnemyPressureFrame >= 0 &&
                              (frame - lastEnemyPressureFrame) <= 24 * 20;
    int defensiveCannonsReadyTarget = std::max(2, basePvPDefensiveCannons);
    bool enemyInBaseNow = enemyPresenceNearHome(startLocationCenter(myStart), 600);
    bool dropDefenseHoldHome = enemyProtoss &&
                               frame < 24 * 60 * 10 &&
                               (proactiveDropDefense || roboticsDropThreat || forges > 0) &&
                               (enemyNearHome ||
                                enemyInBaseNow ||
                                recentHomePressure ||
                                completedCannons < defensiveCannonsReadyTarget ||
                                (roboticsDropThreat && dragoons < 6));
    if (enemyProtoss && enemyInBaseNow)
    {
        dropDefenseHoldHome = true;
        canProactiveAttack = false;
    }

    Position attackTarget = enemyStart ? startLocationCenter(enemyStart) : startLocationCenter(myStart);
    if (lastEnemyBuildingTile)
    {
        attackTarget = tileCenter(lastEnemyBuildingTile);
    }
    else if (!enemyStart)
    {
        auto starts = Broodwar->getStartLocations();
        if (!starts.empty())
        {
            if (frame - lastArmySearchFrame > 24 * 45)
            {
                lastArmySearchFrame = frame;
                armySearchIndex++;
            }
            for (int i = 0; i < (int)starts.size(); ++i)
            {
                TilePosition candidate = starts[(armySearchIndex + i) % starts.size()];
                if (candidate == myStart) continue;
                attackTarget = startLocationCenter(candidate);
                break;
            }
        }
    }
    Position defendTarget = startLocationCenter(myStart);

    double friendlyPower = computeLocalPower(Broodwar->self()->getUnits(), defendTarget, 1400);
    double enemyPower = computeLocalPower(Broodwar->enemy()->getUnits(), defendTarget, 1400);
    bool enemyNear = enemyPower > 0.6;

    bool strongAdvantage = (enemyPower <= 0.15) ? (friendlyPower > 3.5) : (friendlyPower > enemyPower * 1.25);
    bool weak = (enemyPower > friendlyPower * 1.25);

    bool counterWindow = (lastEnemyPressureFrame >= 0 && (frame - lastEnemyPressureFrame) <= 24 * 60 * 3);
    bool counterReady = counterWindow && (armyCount >= 12) && (observers >= 1) && (enemyPower <= 0.1 || friendlyPower > enemyPower * 1.6);

    // Hysteresis: once we commit to attack, don't retreat unless army wiped
    if (dropDefenseHoldHome) committedAttack = false;
    if (canProactiveAttack) committedAttack = true;
    if (armyCount <= 2) committedAttack = false;
    bool shouldAttack = canProactiveAttack || counterReady || committedAttack;
    bool shouldDefend = weak || ((responseMask & Resp_Defend) && !counterReady);
    if (committedAttack && !weak) shouldDefend = false;

    if (weak && retreatUntilFrame < frame + 48) retreatUntilFrame = frame + 48;

    // Regroup rule: require 70% of army near rally point before moving out
    int rallyCount = 0;
    for (auto &u : Broodwar->self()->getUnits())
    {
        if (!u->exists()) continue;
        if (u->getType() != UnitTypes::Protoss_Zealot && u->getType() != UnitTypes::Protoss_Dragoon) continue;
        if (u->getDistance(defendTarget) <= 320) rallyCount++;
    }
    (void)rallyCount;

    // Focus target refresh (集火)
    bool homeDefenseActive = dropDefenseHoldHome || recentHomePressure;
    if (enemyNear || shouldAttack || homeDefenseActive)
    {
        Unit focus = (focusTargetId >= 0) ? Broodwar->getUnit(focusTargetId) : nullptr;
        if (!focus || !focus->exists() || (frame - focusTargetFrame) > 18 || focus->getDistance(shouldAttack ? attackTarget : defendTarget) > 900)
        {
            Position focusCenter = shouldAttack ? attackTarget : defendTarget;
            focus = pickFocusTarget(focusCenter, 900);
            focusTargetId = focus ? focus->getID() : -1;
            focusTargetFrame = frame;
        }
    }

    if (homeDefenseActive || enemyNearHome)
    {
        int pulledDefenseWorkers = 0;
        for (auto &u : Broodwar->self()->getUnits())
        {
            if (!u->exists()) continue;
            if (u->getType() != UnitTypes::Protoss_Probe) continue;
            if (u == scoutProbe) continue;
            if (u->isConstructing()) continue;

            double nearestThreatDist = 1000000.0;
            double nearestMeleeThreatDist = 1000000.0;
            Position nearestThreatPos = Position(0, 0);
            Unit nearestThreat = nullptr;
            for (auto &enemy : Broodwar->enemy()->getUnits())
            {
                if (!enemy->exists()) continue;
                if (enemy->getType().isWorker()) continue;
                if (!enemy->getType().canAttack()) continue;
                double dist = enemy->getDistance(u);
                if (dist < nearestThreatDist)
                {
                    nearestThreatDist = dist;
                    nearestThreatPos = enemy->getPosition();
                    nearestThreat = enemy;
                }
                if (!enemy->isFlying() &&
                    enemy->getType().groundWeapon() != WeaponTypes::None &&
                    enemy->getType().groundWeapon().maxRange() <= 40)
                {
                    nearestMeleeThreatDist = std::min(nearestMeleeThreatDist, dist);
                }
            }

            bool urgentThreat = nearestThreatDist <= 240.0 || nearestMeleeThreatDist <= 320.0;
            if (!urgentThreat) continue;

            bool pullWorkerDefense = enemyInBaseNow &&
                                     armyCount < 10 &&
                                     probeCount > 8 &&
                                     pulledDefenseWorkers < 10 &&
                                     nearestThreat &&
                                     nearestThreatDist <= 420.0;
            if (pullWorkerDefense)
            {
                u->attack(nearestThreat);
                pulledDefenseWorkers++;
                continue;
            }

            if (nearestMeleeThreatDist <= 192.0)
            {
                Position fleeDir = u->getPosition() - nearestThreatPos;
                double fleeLen = std::sqrt(fleeDir.x * fleeDir.x + fleeDir.y * fleeDir.y);
                if (fleeLen < 1.0)
                {
                    fleeDir = Position(u->getID() % 3 - 1, u->getID() % 5 - 2);
                    fleeLen = std::sqrt(fleeDir.x * fleeDir.x + fleeDir.y * fleeDir.y);
                }
                if (fleeLen > 0)
                {
                    Position fleeTarget = u->getPosition() + Position(
                        static_cast<int>(fleeDir.x * 192.0 / fleeLen),
                        static_cast<int>(fleeDir.y * 192.0 / fleeLen));
                    fleeTarget.x = std::max(0, std::min(fleeTarget.x, Broodwar->mapWidth() * 32 - 1));
                    fleeTarget.y = std::max(0, std::min(fleeTarget.y, Broodwar->mapHeight() * 32 - 1));
                    u->move(fleeTarget);
                    continue;
                }
            }

            Unit mineral = closestMineralTo(u);
            if (mineral)
            {
                u->gather(mineral);
            }
            else if (u->getDistance(defendTarget) > 64)
            {
                u->move(defendTarget);
            }
        }
    }

    for (auto &u : Broodwar->self()->getUnits())
    {
        if (!u->exists()) continue;
        if (u->getType() != UnitTypes::Protoss_Zealot && u->getType() != UnitTypes::Protoss_Dragoon) continue;

        // ---- 生存判定：严重损伤时撤退 ----
        double maxLife = u->getType().maxShields() + u->getType().maxHitPoints();
        double curLife = u->getShields() + u->getHitPoints();
        bool criticalHealth = (curLife < maxLife * 0.22);
        if (criticalHealth && !shouldAttack)
        {
            u->move(defendTarget);
            continue;
        }

        // ---- 全局撤退指令 ----
        if (retreatUntilFrame > frame)
        {
            u->move(defendTarget);
            continue;
        }

        // ---- 局部劣势检查 ----
        double localFriendly = computeLocalPower(Broodwar->self()->getUnits(), u->getPosition(), 220);
        double localEnemy = computeLocalPower(Broodwar->enemy()->getUnits(), u->getPosition(), 260);
        if ((localEnemy > localFriendly * 1.35) && u->getDistance(defendTarget) > 260)
        {
            u->move(defendTarget);
            continue;
        }

        // ---- 进攻模式：全力压上，Dragoon 带 kiting ----
        if (shouldAttack)
        {
            Unit target = pickCombatTarget(u);
            if (target)
            {
                if (u->getType() == UnitTypes::Protoss_Dragoon)
                {
                    double dist = u->getDistance(target);
                    double range = u->getType().groundWeapon().maxRange();
                    // 若敌人过近或武器冷却中，后退拉距离
                    if (dist < range * 0.55 || (dist <= range && u->getGroundWeaponCooldown() > 0))
                    {
                        Position retreatPos = u->getPosition() + (u->getPosition() - target->getPosition()) * (48.0 / std::max(1.0, dist));
                        u->move(retreatPos);
                        continue;
                    }
                }
                u->attack(target);
            }
            else if (u->getDistance(attackTarget) > 240)
            {
                u->move(attackTarget);
            }
            else
            {
                // Near enemy base, A-move to engage buildings
                u->attack(attackTarget);
            }
            continue;
        }

        // ---- 集火目标 ----
        Unit focus = (focusTargetId >= 0) ? Broodwar->getUnit(focusTargetId) : nullptr;
        if (focus && focus->exists() && u->getDistance(focus) <= 640)
        {
            u->attack(focus);
            continue;
        }

        // ---- 寻找并攻击目标（含防守 kiting）----
        Unit target = pickCombatTarget(u);
        if (target)
        {
            double distToTarget = u->getDistance(target);

            // 兵力极少时完全不主动攻击，退回防守点依靠自动反击
            if (armyCount < 4)
            {
                if (target->getType() == UnitTypes::Protoss_Shuttle ||
                    target->getType() == UnitTypes::Terran_Dropship)
                {
                    u->attack(target);
                    continue;
                }
                if (u->getDistance(defendTarget) > 80)
                    u->move(defendTarget);
                continue;
            }

            if (u->getType() == UnitTypes::Protoss_Dragoon)
            {
                double range = u->getType().groundWeapon().maxRange();
                if (distToTarget < range * 0.45 || (distToTarget <= range && u->getGroundWeaponCooldown() > 0))
                {
                    Position retreatPos = u->getPosition() + (u->getPosition() - target->getPosition()) * (48.0 / std::max(1.0, distToTarget));
                    u->move(retreatPos);
                    continue;
                }
            }

            bool chaseOK = strongAdvantage && (u->getDistance(defendTarget) <= 900);
            if (chaseOK || enemyNear)
            {
                u->attack(target);
                continue;
            }
        }

        // ---- 空闲调度 ----
        if (u->isIdle() || u->getOrder() == Orders::PlayerGuard)
        {
            if (shouldDefend || enemyNear)
                u->attack(defendTarget);
            else
                u->attack(attackTarget);
        }
    }

    CherryVis::frameEnd(Broodwar->getFrameCount());
}

void DemoAIModule::onSendText(std::string text)
{
    Broodwar->sendText("%s", text.c_str());
}

void DemoAIModule::onReceiveText(BWAPI::Player player, std::string text)
{
    Broodwar << player->getName() << " said \"" << text << "\"" << std::endl;
}

void DemoAIModule::onPlayerLeft(BWAPI::Player player)
{
    Broodwar->sendText("Goodbye %s!", player->getName().c_str());
}

void DemoAIModule::onNukeDetect(BWAPI::Position target)
{
    if (target)
    {
        Broodwar << "Nuclear Launch Detected at " << target << std::endl;
    }
    else
    {
        Broodwar->sendText("Where's the nuke?");
    }
}

void DemoAIModule::onUnitDiscover(BWAPI::Unit unit)
{
    if (unit && Broodwar->enemy() && unit->getPlayer() == Broodwar->enemy() && unit->getType().isBuilding())
    {
        lastEnemyBuildingTile = unit->getTilePosition();
        if (!enemyStart) enemyStart = closestStartLocation(unit->getPosition());
    }
}

void DemoAIModule::onUnitEvade(BWAPI::Unit unit)
{
}

void DemoAIModule::onUnitShow(BWAPI::Unit unit)
{
    if (unit && Broodwar->enemy() && unit->getPlayer() == Broodwar->enemy() && unit->getType().isBuilding())
    {
        lastEnemyBuildingTile = unit->getTilePosition();
        if (!enemyStart) enemyStart = closestStartLocation(unit->getPosition());
    }
}

void DemoAIModule::onUnitHide(BWAPI::Unit unit)
{
}

void DemoAIModule::onUnitCreate(BWAPI::Unit unit)
{
}

void DemoAIModule::onUnitDestroy(BWAPI::Unit unit)
{
    if (unit && unit->getID() == scoutProbeId)
    {
        scoutProbeId = -1;
    }
}

void DemoAIModule::onUnitMorph(BWAPI::Unit unit)
{
    if (Broodwar->isReplay())
    {
        if (unit->getType().isBuilding() && !unit->getPlayer()->isNeutral())
        {
            int seconds = Broodwar->getFrameCount() / 24;
            int minutes = seconds / 60;
            seconds %= 60;
            Broodwar->sendText("%.2d:%.2d: %s morphs a %s", minutes, seconds, unit->getPlayer()->getName().c_str(), unit->getType().c_str());
        }
    }
}

void DemoAIModule::onUnitRenegade(BWAPI::Unit unit)
{
}

void DemoAIModule::onSaveGame(std::string gameName)
{
    Broodwar << "The game was saved to \"" << gameName << "\"" << std::endl;
}

void DemoAIModule::onUnitComplete(BWAPI::Unit unit)
{
}
