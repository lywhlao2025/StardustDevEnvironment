# Replay PvP Zealot Floor

## 问题背景

上一轮把 PvP 的二气前科技节奏提前后，Core 和 Dragoon 转型更接近 replay 时间点，但 `Locutus.4GateGoon` 结果显示 5-7 分钟兵力没有同步增长，说明转型过早会压掉 2Gate 开局应有的 Zealot 数量。

## 败因分析（Replay + Log）

- Replay：`TL2104_Dewalt_Shuttle_Textbook_PvP_Fighting_Spirit_zimp_Aug_30_21_804.rep` 显示 Pylon 约 0:47、Gateway 约 1:13、第二 Gateway 约 1:14、Gas 约 1:29、Core 约 1:57。该节奏是先用 2Gate 撑住前排，再进入 Core/Dragoon。
- Replay：`SB_20260323_Unknown_oG_Bluesky_vs_BSL-Gosudark.rep` 显示一方在 1:41-1:45 连续补到多 Gateway，另一方也在 2:06/2:19 补第二、第三 Gateway。PvP replay 的共同点不是裸科技，而是开局 Gateway 产能先形成近战底盘。
- Log：`Locutus.4GateGoon` 在 gas/core timing 后仍失败，记录为 `army@5/6/7/8=7/4/0/-1`、`gates@5/6/7/8=2/3/3/-1`。Gateway 数量有增长，但 6 分钟兵力反而下降，说明 Dragoon 预留逻辑覆盖了 Zealot 底线。

## 改进方案

- PvP 6 分钟前基础 `targetEarlyZealots` 从 4 提到 6，对齐 replay 中 2Gate 先产 Zealot 的常见节奏。
- 增加 `zealotFloorNeeded`，在 Zealot 未达底线时阻止 `reserveForFirstDragoons` 抢占训练逻辑。
- 保留侦测 Robotics/drop 时降低 Zealot 底线的分支，避免对 Shuttle/隐刀风险过度近战化。

## 测试结果

- `cmake --build build --target tests`：通过；仍有既有 `ninja: warning: premature end of file; recovering` 和 `/usr/local/Cellar/llvm/13.0.0_2/lib` 搜索路径 warning。
- `./tests --gtest_filter=Locutus.4GateGoon`：失败；地图 `Circuit Breaker`，seed `50007`，7:28 结束。
- 诊断：`army@5/6/7/8=8/6/0/-1`、`gates@5/6/7/8=2/3/3/-1`、`workers@5/8/10/12=18/-1/-1/-1`、`supplyBlockedSec=61`、`idleFrames(Gateway/Core/Nexus)=5667/4731/5141`。

## 评估

本改动是 replay 驱动的小步调整，目标是恢复 PvP 早期近战兵力底线，而不是扩大随机探索面。相比上一轮 `army@5/6=7/4` 的结果，本次回到 `8/6`，方向有效但不足以取胜。下一步应优先处理 61 秒 supply block 和 Gateway idle，而不是继续提前科技。
