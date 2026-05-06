# PvP Drop Default Zealot Floor To Four

## 问题背景

最新日志已经反复说明，默认 6 条 Zealot 的前期目标会把矿物吃得太紧，导致 Core 后面还是没有足够的资源起 Dragoon。

## 败因分析（Replay + Log）

- Replay：PvP replay 里，2 Gate Core 的标准开局不需要默认堆到 6 条 Zealot 才转型。
- Log：`Fighting Spirit` seed `33263` 显示在 5:24 时 `zealots=13`、`minerals=32`、`gas=216`、`dragoons=0`，说明默认 Zealot floor 明显过高。
- 代码：`targetEarlyZealots` 默认值仍然给到了 6，只有在少数条件下才会再收紧，导致资源先被狂热者吃掉。

## 改进方案

- 把 `earlyPvP` 的默认 `targetEarlyZealots` 直接降到 4。
- 保留 rush / enemyNearHome / enemyDragoons 等威胁下的更高上限。
- 目标是让默认 PvP 开局更早把矿让给 Core 和 Dragoon，而不是继续堆过多 zealot。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是对 PvP 默认兵种配比的直接纠偏。若它有效，前期 `zealots` 数量应该显著下降，`dragoons` 会更早开始积累。
