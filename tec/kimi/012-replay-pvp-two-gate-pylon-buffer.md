# Replay PvP Two-Gate Pylon Buffer

## 问题背景

上一轮 Zealot floor 改动后，`Locutus.4GateGoon` 仍在 7:28 失败。诊断显示早期兵力略有恢复，但 `supplyBlockedSec=61` 且 Gateway idle 很高，说明 2-3 Gateway 产能没有被供给链支撑住。

## 败因分析（Replay + Log）

- Replay：`TL2104_Dewalt_Shuttle_Textbook_PvP_Fighting_Spirit_zimp_Aug_30_21_804.rep` 中 Pylon 时间为 0:47、2:00、3:06、4:18、4:49、5:42、6:04，2Gate 开局后持续提前补供给。
- Log：`Locutus.4GateGoon` 失败记录为 `army@5/6/7/8=8/6/0/-1`、`gates@5/6/7/8=2/3/3/-1`、`supplyBlockedSec=61`、`idleFrames(Gateway/Core/Nexus)=5667/4731/5141`。
- 当前代码只在 `gateways >= 3` 时显著提高 Pylon buffer；但 replay 里的 2Gate 开局已经需要更早的供给缓冲，否则 Zealot floor 会把供应吃满。

## 改进方案

- PvP 且 `gateways >= 2` 时，将 Gateway 供给触发阈值从通用 14 提到 22。
- PvP 且 `gateways >= 2` 时，将额外 Pylon buffer 阈值提高到 24，并把矿物门槛从 200 降到 125。
- 保留 `gateways >= 3` 的更高阈值和最多 3 个在建 Pylon 逻辑，避免中期多 Gateway 再次卡供给。

## 测试结果

- `cmake --build build --target tests`：通过；仍有既有 `ninja: warning: premature end of file; recovering` 和 `/usr/local/Cellar/llvm/13.0.0_2/lib` 搜索路径 warning。
- `./tests --gtest_filter=Locutus.4GateGoon`：失败；地图 `Andromeda`，seed `58978`，7:25 结束。
- 诊断：`army@5/6/7/8=9/13/0/-1`、`gates@5/6/7/8=3/4/4/-1`、`workers@5/8/10/12=16/-1/-1/-1`、`supplyBlockedSec=57`、`idleFrames(Gateway/Core/Nexus)=9418/0/6152`、最终 `gas=424`。

## 评估

本改动继续沿 replay 节奏修正 2Gate 开局的供给链。结果显示 6 分钟兵力从上一轮 6 提升到 13，Gateway 数也达到 4，方向有效；但仍有 57 秒 supply block，且最终 gas 浮到 424，下一步应降低 PvP 中期采气/提高矿物转兵效率。
