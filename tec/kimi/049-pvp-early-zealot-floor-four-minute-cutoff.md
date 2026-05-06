# PvP Early Zealot Floor Four Minute Cutoff

## 问题背景

最新窗口诊断说明，在 5 分钟左右矿已经被 zealot flood 压得很紧，第一条 Dragoon 还没有起，资源就已经被吃掉太多了。

## 败因分析（Replay + Log）

- Replay：PvP replay 里，前期 Zealot floor 只需要撑到最早的一小段对抗期，之后就应该尽快让位给 Core 和 Dragoon。
- Log：`Icarus` seed `43876` 的窗口诊断显示在 4:16 时 `minerals=26 gas=88 supplyRemaining=26 availableArmySupply=13 gateways=2 zealots=6 dragoons=0`，说明即使在较早阶段，矿也已经被前期兵种压得很低。
- 代码：`targetEarlyZealots` 仍然保持到 6 分钟才收口，导致 4-5 分钟这个关键转型点还是过度出 zealot。

## 改进方案

- 把 PvP 的早期 zealot floor 收口点提前到 4 分钟。
- 这样 4 分钟之后，Gateway 会更早让位给 Core 和 Dragoon，减少前期 zealot flood 对矿物的吞噬。
- 目标是让第一批 Dragoon 能在更早的时间点成型，而不是等到矿已经被吃空后才开始补。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是一个更早的兵种分流修正，直接针对 5 分钟前后的矿物枯竭问题。若它有效，第一条 Dragoon 的窗口应该会比现在更早出现。
