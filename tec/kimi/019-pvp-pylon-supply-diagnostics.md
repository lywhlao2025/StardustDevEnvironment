# PvP Pylon Supply Diagnostics

## 问题背景

连续多轮 replay 驱动调整后，`Locutus.4GateGoon` 的主要失败信号仍包含 50-60 秒 supply block。现有日志只有总卡供给秒数，无法区分 Pylon 数不足、Pylon 下得太晚、还是战斗后供给容量被瞬间吃满。

## 败因分析（Replay + Log）

- Replay：textbook PvP 在 3:06、4:18、4:49、5:42、6:04 持续补 Pylon，说明 Pylon 时间线是判断 3Gate/4Gate 是否健康的关键指标。
- Log：最近 Benzene 失败记录为 `army@5/6/7/8=9/15/0/-1`、`gates@5/6/7/8=3/3/3/-1`、`supplyBlockedSec=66`，但缺少 `pylons@5/6/7/8` 和 `supply@5/6/7/8`。
- 当前缺失的诊断会导致下一步继续盲目调阈值，不能定位 `tryBuild(Pylon)`、矿物抢占或产能节奏中的具体问题。

## 改进方案

- 增加 `pylonCountAt5/6/7/8`，记录关键时间点已完成 Pylon 数。
- 增加 `supplyUsedAt5/6/7/8` 与 `supplyTotalAt5/6/7/8`，记录关键时间点供给容量状态。
- 在 `onEnd` 输出 `DIAG pylons@5/6/7/8` 与 `supply@5/6/7/8`，只增强诊断，不改变行为。

## 测试结果

- `cmake --build build --target tests`：通过；仍有既有 `ninja: warning: premature end of file; recovering` 和 `/usr/local/Cellar/llvm/13.0.0_2/lib` 搜索路径 warning。
- `./tests --gtest_filter=Locutus.4GateGoon`：失败；地图 `Neo Moon Glaive`，seed `43722`，7:46 结束。
- 新诊断：`pylons@5/6/7/8=5/7/7/-1`，`supply@5/6/7/8=76/98 68/130 2/112 -1/-1`。
- 原诊断：`army@5/6/7/8=8/7/0/-1`、`gates@5/6/7/8=2/3/4/-1`、`supplyBlockedSec=59`、结果 `OutFought`。

## 评估

本改动是定位 supply block 根因的诊断补充。新日志显示 5/6 分钟供给并未卡满，7 分钟是战斗部队已被清空后的低供给状态；因此此前 `supplyBlockedSec` 很可能混入了战败后的供给归零/建筑损失，不应继续优先调 Pylon 阈值。下一步应回到交战损耗、集结和防守点。
