# Replay PvP Three-Gate Pylon Buffer

## 问题背景

Four-gate buffer 能在部分地图把 7 分钟兵力推到 19，但后续采样在 La Mancha 上显示 3Gate 阶段仍被 supply block 卡住，6 分钟兵力只有 6。

## 败因分析（Replay + Log）

- Replay：PvP replay 的 3-4 分钟后 Pylon 是连续补的，不是等到 4Gate 完整成型才补。例如 textbook replay 在 3:06、4:18、4:49、5:42 连续追加 Pylon。
- Log：补采样 `Locutus.4GateGoon` 在 `La Mancha` seed `35157` 失败，`army@5/6/7/8=8/6/0/-1`、`gates@5/6/7/8=2/3/3/-1`、`supplyBlockedSec=62`、结果 `SupplyBlocked`。
- 当前代码只在 4Gate 阶段明显提高 buffer；当地图或路线让第三 Gateway 后供给先断，第四 Gateway 和后续补兵都无法稳定出现。

## 改进方案

- PvP 且 `gateways >= 3` 时，供给触发阈值提高到 30。
- PvP 3Gate 阶段将 Pylon buffer 阈值提高到 36，最多允许 4 个在建 Pylon，矿物门槛降到 100。
- PvP 4Gate 阶段的 buffer 阈值继续提高到 44，维持最多 4 个在建 Pylon。

## 测试结果

- `cmake --build build --target tests`：通过；仍有既有 `ninja: warning: premature end of file; recovering` 和 `/usr/local/Cellar/llvm/13.0.0_2/lib` 搜索路径 warning。
- `./tests --gtest_filter=Locutus.4GateGoon`：失败；地图 `Benzene`，seed `22916`，7:58 结束。
- 诊断：`army@5/6/7/8=9/15/0/-1`、`gates@5/6/7/8=3/3/3/-1`、`workers@5/8/10/12=16/-1/-1/-1`、`supplyBlockedSec=66`、`idleFrames(Gateway/Core/Nexus)=9144/0/5513`、最终 `gas=496`、结果 `SupplyBlocked`。

## 评估

本改动把 supply buffer 从 4Gate 前移到 3Gate，目标是让不同地图上的 PvP 产能都能跨过 5-6 分钟供给断点。测试显示 6 分钟兵力达到 15，但 supply block 仍升到 66 秒，说明下一步应检查 Pylon 建造成功率和其它建造/训练对 Pylon 资金的抢占。
