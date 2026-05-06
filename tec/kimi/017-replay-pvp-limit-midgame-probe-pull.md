# Replay PvP Limit Midgame Probe Pull

## 问题背景

Four-gate Pylon buffer 后，`Locutus.4GateGoon` 存活到 9:02，但失败原因变成 `LowEcon`。8 分钟时只剩 7 个 Probe，说明中期防守把经济牺牲得过重。

## 败因分析（Replay + Log）

- Replay：PvP replay 的中期防守核心仍是 Gateway 兵力抱团换兵，不是已有 3-4 Gateway 后持续大量拉 Probe 参战。
- Log：`Locutus.4GateGoon` 最新失败记录为 `army@5/6/7/8=9/14/19/4`、`workers@5/8/10/12=17/7/-1/-1`、结果 `LowEcon`。
- 代码：Probe 防守逻辑在敌人进基地且兵力不足时每帧最多拉 10 个 Probe。PvP 6 分钟后已有 Gateway 部队时，这会让经济在后续战斗中迅速归零。

## 改进方案

- PvP 6 分钟后，将拉防 Probe 上限从 10 降到 4。
- PvP 6 分钟后，只有当我方战斗兵少于 6 时才拉 Probe；有足够 Gateway 兵时优先保护经济。
- 保留 6 分钟前和非 PvP 的原有紧急拉防力度。

## 测试结果

- `cmake --build build --target tests`：通过；仍有既有 `ninja: warning: premature end of file; recovering` 和 `/usr/local/Cellar/llvm/13.0.0_2/lib` 搜索路径 warning。
- `./tests --gtest_filter=Locutus.4GateGoon`：失败；地图 `Circuit Breaker`，seed `35438`，7:23 结束。
- 诊断：`army@5/6/7/8=8/11/0/-1`、`gates@5/6/7/8=3/4/4/-1`、`workers@5/8/10/12=16/-1/-1/-1`、`supplyBlockedSec=58`、`idleFrames(Gateway/Core/Nexus)=9073/0/5976`、最终 `gas=400`、结果 `OutFought`。

## 评估

本改动针对上一场 `LowEcon` 信号，目标是减少中期 Probe 被持续投入战斗，保留 8 分钟后的最低采矿能力。本次随机地图未复现 9 分钟经济断档，仍在 7:23 被 `OutFought`；该改动需要后续多场验证是否应保留。
