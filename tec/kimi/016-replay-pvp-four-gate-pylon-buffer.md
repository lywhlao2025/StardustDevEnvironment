# Replay PvP Four-Gate Pylon Buffer

## 问题背景

Tech gas cap 后，`Locutus.4GateGoon` 能在 6 分钟形成 14 兵和 4 Gateway，但仍因 55 秒 supply block 与高 Gateway idle 被打穿。说明 4Gate 阶段的供给缓冲仍不足。

## 败因分析（Replay + Log）

- Replay：`TL2104_Dewalt_Shuttle_Textbook_PvP_Fighting_Spirit_zimp_Aug_30_21_804.rep` 中 4 分半后继续补 Pylon，时间点包括 4:18、4:49、5:42、6:04。多 Gateway 阶段需要提前连续补供给。
- Log：`Locutus.4GateGoon` 最新失败记录为 `army@5/6/7/8=8/14/0/-1`、`gates@5/6/7/8=3/4/4/-1`、`supplyBlockedSec=55`、`idleFrames(Gateway/Core/Nexus)=10868/0/6357`。
- 当前代码对 `gateways >= 3` 的 PvP buffer 为 28，最多 3 个在建 Pylon；但 4Gate 连续 Zealot/Dragoon 消耗供给更快，28 阈值仍会让产能断档。

## 改进方案

- PvP 且 `gateways >= 4` 时，额外供给触发阈值提高到 34。
- PvP 4Gate 阶段将 Pylon buffer 阈值提高到 40，最多允许 4 个在建 Pylon。
- 4Gate 阶段 Pylon 矿物门槛降到 100，优先保障供给链不断。

## 测试结果

- `cmake --build build --target tests`：通过；仍有既有 `ninja: warning: premature end of file; recovering` 和 `/usr/local/Cellar/llvm/13.0.0_2/lib` 搜索路径 warning。
- `./tests --gtest_filter=Locutus.4GateGoon`：失败；地图 `Heartbreak Ridge`，seed `79949`，9:02 结束。
- 诊断：`army@5/6/7/8=9/14/19/4`、`gates@5/6/7/8=2/3/4/4`、`workers@5/8/10/12=17/7/-1/-1`、`supplyBlockedSec=53`、`idleFrames(Gateway/Core/Nexus)=12246/0/7138`、最终 `gas=704`、结果 `LowEcon`。

## 评估

本改动沿 replay 的 4 分半后连续补 Pylon 节奏修正 4Gate 供给链。结果显示 7 分钟兵力提升到 19，并存活到 9:02，方向有效；但 8 分钟后兵力和经济仍断档，且最终 gas 堆到 704，下一步应延长 one-base PvP 的 gas cap 并保护最低 Probe 数。
