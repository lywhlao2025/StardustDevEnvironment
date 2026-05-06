# Replay PvP Tech Gas Cap

## 问题背景

Regroup 修复后，`Locutus.4GateGoon` 在 Destination 仍被 7:30 打穿。该局早期侦测到 tech，最终 gas 达到 648，说明 PvP tech-threat 分支继续过度采气，削弱了 Gateway 补兵的矿物收入。

## 败因分析（Replay + Log）

- Replay：PvP replay 中即使出现 Core/后续科技，前期仍围绕 2Gate/多 Gateway 的 Zealot/Dragoon 产能展开，Pylon 与 Gateway 节奏不能被过量采气拖慢。
- Log：`Locutus.4GateGoon` 失败记录为 `army@5/6/7/8=10/12/0/-1`、`gates@5/6/7/8=3/3/3/-1`、`supplyBlockedSec=52`、最终 `gas=648`、`firstScout tech=2.39792`。
- 代码：Robotics/drop 威胁分支会在 `dragoons < 6` 时保持更高采气。已有 2 个 Dragoon 且 gas 足够时，继续采气收益小于回矿补 Zealot/Pylon/Gateway。

## 改进方案

- Robotics/drop 威胁下，如果 gas 已达到 150，则目标气工最多维持 1，而不是继续推高到 2。
- PvP 8 分钟前，Core 完成且 gas 达到 300 时，将目标气工降到 0。
- 如果 Robotics/drop 威胁已经出现但 Dragoon 少于 2，则在 gas cap 后仍保留 1 个气工，避免完全断掉最低防空/反科技能力。
- 保留 gas 低于 150 时的科技防守采气，确保首批 Dragoon 和 range 不会断气。

## 测试结果

- `cmake --build build --target tests`：通过；仍有既有 `ninja: warning: premature end of file; recovering` 和 `/usr/local/Cellar/llvm/13.0.0_2/lib` 搜索路径 warning。
- 试验版本：gas cap 220 时，`Tau Cross` seed `81256` 失败，8:00 结束，`army@5/6/7/8=7/9/1/0`、最终 `gas=102`。该阈值压气有效但牺牲了 6 分钟兵力。
- 最终版本：gas cap 300 时，`./tests --gtest_filter=Locutus.4GateGoon` 失败；地图 `Tau Cross`，seed `64493`，7:39 结束。
- 诊断：`army@5/6/7/8=8/14/0/-1`、`gates@5/6/7/8=3/4/4/-1`、`workers@5/8/10/12=16/-1/-1`、`supplyBlockedSec=55`、`idleFrames(Gateway/Core/Nexus)=10868/0/6357`、最终 `gas=496`。

## 评估

本改动给 PvP tech 分支加资源上限，目标是避免早期侦测 tech 后进入 500+ gas 浮余状态，让 4Gate 的矿物补兵优先级重新生效，同时保留首批 Dragoon/range 转型空间。最终版本保住了 6 分钟 14 兵，但仍有 496 gas 和 55 秒 supply block，下一步应继续处理 4Gate idle 与战斗后补兵断档。
