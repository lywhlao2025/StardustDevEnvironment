#pragma once

#include <BWAPI.h>

// Minimal PvP Protoss opponent: builds probes, gateways, zealots, and A-moves.
// No tech, no micro, no expansion. This is a "weak" benchmark opponent.
class WeakPvPModule : public BWAPI::AIModule
{
public:
    virtual void onStart();
    virtual void onEnd(bool isWinner);
    virtual void onFrame();
    virtual void onSendText(std::string text) {}
    virtual void onReceiveText(BWAPI::Player player, std::string text) {}
    virtual void onPlayerLeft(BWAPI::Player player) {}
    virtual void onNukeDetect(BWAPI::Position target) {}
    virtual void onUnitDiscover(BWAPI::Unit unit) {}
    virtual void onUnitEvade(BWAPI::Unit unit) {}
    virtual void onUnitShow(BWAPI::Unit unit) {}
    virtual void onUnitHide(BWAPI::Unit unit) {}
    virtual void onUnitCreate(BWAPI::Unit unit) {}
    virtual void onUnitDestroy(BWAPI::Unit unit) {}
    virtual void onUnitMorph(BWAPI::Unit unit) {}
    virtual void onUnitRenegade(BWAPI::Unit unit) {}
    virtual void onSaveGame(std::string gameName) {}
    virtual void onUnitComplete(BWAPI::Unit unit) {}

private:
    BWAPI::TilePosition myStart;
    int pylonBuilt = 0;
    int gatewayBuilt = 0;
    bool attacked = false;
};
