# PvP Hard Stop Zealot When Assimilator Is Up

## 问题背景

最新诊断已经说明，真正吃掉矿的不是 Dragon 逻辑，而是 `Core` 前的兜底 Zealot 分支。它在气矿已经开、门数已经够的情况下，仍然继续把矿花在 Zealot 上，导致 Core 和第一批 Dragon 都起不来。

## 败因分析（Replay + Log）

- Replay：PvP replay 里，一旦 Assimilator 已经完成，且前期 Zealot 数量已经达标，就不应该再继续靠兜底分支补 Zealot。
- Log：`Roadrunner` seed `12826` 的训练窗口诊断显示 `minerals=50 gas=80 supplyRemaining=30 availableArmySupply=15 reserveForFirstDragoons=0 zealotFloorNeeded=0 targetOpeningDragoons=4 gateways=2 dragoons=0`，但后续又继续堆到 `zealots@5/6/7/8=8/12/1/0`，说明兜底 Zealot 分支仍在吞矿。
- 代码：Gateway 训练循环里缺少一个硬停，当 `cores == 0 && Assimilator 已完成 && gateways >= 2 && zealots >= targetEarlyZealots` 时，仍然会继续走到 Zealot 的兜底训练分支。

## 改进方案

- 在上述条件成立时，直接 `break` 退出 Gateway 训练循环。
- 这样气矿已开且 Zealot 数量已够时，Gateway 不会再继续把矿吃到 Zealot 上。
- 目标是把资源留给 Core 和后续 Dragoon，而不是让兜底分支继续吞矿。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是一个硬停修正，应该直接减少 `Core` 前的矿物浪费。若它有效，下一轮窗口诊断里 `minerals` 应该更高，`Core` 和第一批 Dragoon 的成型时间也会提前。
