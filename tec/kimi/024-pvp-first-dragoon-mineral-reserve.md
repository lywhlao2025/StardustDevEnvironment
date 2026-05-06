# PvP First Dragoon Mineral Reserve

## 问题背景

最新诊断显示 5-8 分钟的 army 几乎全是 Zealot，`dragoons@5/6/7/8=0/0/0/0`，尽管 Gas 充足。这说明问题不是“有没有经济”，而是第一批 Dragoon 被矿物预留挡住了。

## 败因分析（Replay + Log）

- Replay：PvP replay 的 2Gate/3Gate 节奏中，Core 完成后通常会尽快把第一批 Gas 转成 Dragoon，再继续按比例补 Zealot/Dragoon。
- Log：`Heartbreak Ridge` seed `9924` 为 `army@5/6/7/8=9/13/4/-1`、`zealots@5/6/7/8=9/13/4/-1`、`dragoons@5/6/7/8=0/0/0/-1`、`gates@5/6/7/8=2/3/3/-1`、`gas=592`。这说明军队结构偏纯 Zealot，和 replay 的混编节奏不一致。
- 代码：早期 PvP 在 `cores > 0 && gateways >= 3 && dragoons == 0` 的情况下，仍可能被 `defensiveCannonMineralReserve` 和其它矿物预留挡住第一批 Dragoon。

## 改进方案

- 当 `earlyPvP && cores > 0 && gateways >= 3 && dragoons == 0` 时，把 `mineralReserve` 压到最多 50。
- 保留 Cannon/防守预留的存在，但不让它挡住第一批 Dragoon 的出现。
- 目标是把 3Gate 阶段从纯 Zealot 转向 replay 常见的 Zealot + Dragoon 混编。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

本改动针对“有气却不出龙骑”的结构性问题，目标是让第一批 Dragoon 尽快落地，以提升 7-9 分钟的正面换兵质量。
