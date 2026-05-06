# PvP Cap Enemy Near Home Zealot Floor At Six

## 问题背景

当前 PvP 失败里，一个常见模式是 `enemyNearHome` 一触发就把 zealot floor 拉到 8，结果矿物又被过度消耗，Dragoon 还是起不来。

## 败因分析（Replay + Log）

- Replay：PvP replay 里，敌人压家不代表要无限堆 zealot。2 Gate Core 体系还是要给 Dragoon 留矿。
- Log：`Python` seed `15102` 的窗口诊断在 4:29 时显示 `minerals=54 gas=32 zealots=8 dragoons=0`，说明在 `enemyNearHome` 这类压力下，8 条 zealot 已经明显偏多。
- 代码：`if (enemyNearHome || enemyDragoons > 0) targetEarlyZealots = 8;` 把两种不同威胁混成了一档，导致 `enemyNearHome` 也被抬到了 8。

## 改进方案

- 把 `enemyNearHome` 下的 zealot floor 降到 6。
- 只有真正出现 `enemyDragoons > 0` 时才把 zealot floor 拉到 8。
- 目标是保留必要防守，但不给前期 zealot flood 留出太大的矿物吞噬空间。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是在压力场景下细分 zealot floor 的修正。若它有效，前期矿物应该更不容易被 8 条 zealot 吃空，后续 dragoon 转型会更顺。
