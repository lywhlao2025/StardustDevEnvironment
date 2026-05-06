# PvP Default Zealot Floor To Two

## 问题背景

最新日志持续显示，默认 4 条 zealot 还是会把矿物吃得太紧，导致 Core 和 Dragoon 仍然很晚才成型。现在需要进一步收紧默认前期 zealot 数量。

## 败因分析（Replay + Log）

- Replay：2 Gate Core 的 PvP 开局里，默认并不需要先堆到 4 条 zealot 才转型；继续堆只会延迟 Core 和 Dragoon。
- Log：`Circuit Breaker` seed `20905` 在 5:21 时已经出现 `zealots=12`、`minerals=40`、`gas=152`，说明 4 条的默认 floor 仍然不够紧。
- Code：`targetEarlyZealots` 默认仍然给到 4，仍会在多数对局里把矿物吞掉。

## 改进方案

- 把 `earlyPvP` 的默认 `targetEarlyZealots` 再降到 2。
- 保留真正 rush / enemyNearHome / enemyDragoons 的更高上限，让防守场景仍然可用。
- 目标是让资源更早转给 Core 和 Dragoon，而不是继续让默认 zealot floor 吃矿。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是更激进的兵种配比修正，风险在于前期防守更轻，但如果它能把矿物让出来，Core 和 Dragoon 的成型会明显提前。
