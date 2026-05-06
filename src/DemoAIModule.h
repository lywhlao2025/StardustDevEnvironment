#pragma once
#include <BWAPI.h>

// Remember not to use "Broodwar" in any global class constructor!

class DemoAIModule : public BWAPI::AIModule
{
public:
  int frameSkip = 0;

  // Virtual functions for callbacks, leave these as they are.
  virtual void onStart();
  virtual void onEnd(bool isWinner);
  virtual void onFrame();
  virtual void onSendText(std::string text);
  virtual void onReceiveText(BWAPI::Player player, std::string text);
  virtual void onPlayerLeft(BWAPI::Player player);
  virtual void onNukeDetect(BWAPI::Position target);
  virtual void onUnitDiscover(BWAPI::Unit unit);
  virtual void onUnitEvade(BWAPI::Unit unit);
  virtual void onUnitShow(BWAPI::Unit unit);
  virtual void onUnitHide(BWAPI::Unit unit);
  virtual void onUnitCreate(BWAPI::Unit unit);
  virtual void onUnitDestroy(BWAPI::Unit unit);
  virtual void onUnitMorph(BWAPI::Unit unit);
  virtual void onUnitRenegade(BWAPI::Unit unit);
  virtual void onSaveGame(std::string gameName);
  virtual void onUnitComplete(BWAPI::Unit unit);
  // Everything below this line is safe to modify.

  // Basic strategy state
  BWAPI::TilePosition myStart = BWAPI::TilePositions::Invalid;
  BWAPI::TilePosition enemyStart = BWAPI::TilePositions::Invalid;
  BWAPI::TilePosition proxyAnchor = BWAPI::TilePositions::Invalid;
  int scoutIndex = 0;
  int lastScoutFrame = 0;
  bool proxyPylonStarted = false;
  bool proxyPylonCompleted = false;
  int scoutProbeId = -1;

  // Threat/response + diagnostics state
  int threatMask = 0;
  int responseMask = 0;
  int supplyBlockedFrames = 0;
  int idleGatewayFrames = 0;
  int idleCoreFrames = 0;
  int idleNexusFrames = 0;
  int workerCountAt5 = -1;
  int workerCountAt8 = -1;
  int workerCountAt10 = -1;
  int workerCountAt12 = -1;
  int armyCountAt5 = -1;
  int armyCountAt6 = -1;
  int armyCountAt7 = -1;
  int armyCountAt8 = -1;
  int gatewayCountAt5 = -1;
  int gatewayCountAt6 = -1;
  int gatewayCountAt7 = -1;
  int gatewayCountAt8 = -1;
  int pylonCountAt5 = -1;
  int pylonCountAt6 = -1;
  int pylonCountAt7 = -1;
  int pylonCountAt8 = -1;
  int supplyUsedAt5 = -1;
  int supplyUsedAt6 = -1;
  int supplyUsedAt7 = -1;
  int supplyUsedAt8 = -1;
  int supplyTotalAt5 = -1;
  int supplyTotalAt6 = -1;
  int supplyTotalAt7 = -1;
  int supplyTotalAt8 = -1;
  int firstProxyFrame = -1;
  int firstTechFrame = -1;
  int firstExpandFrame = -1;
  int lastFrameCount = 0;
  int lastEnemyPressureFrame = -1;

  // Enemy memory / map search state
  BWAPI::TilePosition lastEnemyBuildingTile = BWAPI::TilePositions::Invalid;
  int armySearchIndex = 0;
  int lastArmySearchFrame = 0;

  // Combat coordination state
  int focusTargetId = -1;
  int focusTargetFrame = 0;
  int retreatUntilFrame = 0;
  bool committedAttack = false;
};
