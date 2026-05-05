#pragma once

#include <BWAPI.h>

class AttackNearestModule : public BWAPI::AIModule
{
public:
    void onStart() override {}

    void onEnd(bool isWinner) override {}

    void onFrame() override
    {
        if (BWAPI::Broodwar->isReplay() || BWAPI::Broodwar->isPaused() || !BWAPI::Broodwar->self() || !BWAPI::Broodwar->enemy()) return;

        for (auto &u : BWAPI::Broodwar->self()->getUnits())
        {
            if (!u->exists()) continue;
            if (!u->getType().canAttack()) continue;
            if (!u->isCompleted()) continue;

            BWAPI::Unit target = u->getClosestUnit(BWAPI::Filter::IsEnemy && BWAPI::Filter::CanAttack);
            if (!target)
            {
                target = u->getClosestUnit(BWAPI::Filter::IsEnemy);
            }
            if (!target) continue;

            if (u->getOrderTarget() != target || (u->isIdle() || u->getOrder() == BWAPI::Orders::PlayerGuard))
            {
                u->attack(target);
            }
        }
    }

    void onSendText(std::string text) override {}

    void onReceiveText(BWAPI::Player player, std::string text) override {}

    void onPlayerLeft(BWAPI::Player player) override {}

    void onNukeDetect(BWAPI::Position target) override {}

    void onUnitDiscover(BWAPI::Unit unit) override {}

    void onUnitEvade(BWAPI::Unit unit) override {}

    void onUnitShow(BWAPI::Unit unit) override {}

    void onUnitHide(BWAPI::Unit unit) override {}

    void onUnitCreate(BWAPI::Unit unit) override {}

    void onUnitDestroy(BWAPI::Unit unit) override {}

    void onUnitMorph(BWAPI::Unit unit) override {}

    void onUnitRenegade(BWAPI::Unit unit) override {}

    void onSaveGame(std::string gameName) override {}

    void onUnitComplete(BWAPI::Unit unit) override {}
};
