# PvP Cap Enemy Near Home Zealot Floor At Four

## 问题背景

最新日志里，前期 zealot 仍然会升到 5 条左右，这很像是 `enemyNearHome` 把 zealot floor 抬得过高导致的。我们需要再收紧这一档。

## 败因分析（Replay + Log）

- Replay：PvP replay 里，`enemyNearHome` 代表要防守，但不代表必须把 zealot floor 拉到 6。
- Log：`Python` seed `90263` 在 3:57 时已经有 `zealots=5`，而 `core` 和 `dragoon` 还没完全成型。这很像是前期 zealot floor 过高导致的矿物消耗。
- Code：`if (enemyNearHome) targetEarlyZealots = std::max(targetEarlyZealots, 6);` 仍然偏高。

## 改进方案

- 把 `enemyNearHome` 下的 zealot floor 从 6 降到 4。
- 保留真正 rush / enemyDragoons 的更高上限。
- 目标是减少防守型 zealot flood，让 Core 和 Dragoon 更快成型。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是对防守场景里 zealot 数量的再次收紧。若它有效，前期 zealot 数量应该比当前更低，矿物也更容易留给科技和龙骑。
