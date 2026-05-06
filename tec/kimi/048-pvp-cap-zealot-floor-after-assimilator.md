# PvP Cap Zealot Floor After Assimilator

## 问题背景

最新诊断已经明确了真正的瓶颈：在 `Core` 还没完全转型之前，前期 Zealot 数量已经堆到 12 条，把矿物吃得只剩 30，结果第一批 Dragoon 根本起不来。

## 败因分析（Replay + Log）

- Replay：PvP replay 里，Assimilator 完成后就不该继续按纯 Zealot 的前期目标往下堆。
- Log：`Empire of the Sun` seed `94071` 的窗口诊断显示在 5:15 时 `minerals=30 gas=392 supplyRemaining=14 availableArmySupply=8 gateways=2 zealots=12 dragoons=0`。这说明矿物已经被 Zealot floor 吃空。
- 代码：`targetEarlyZealots` 仍然允许 `earlyPvP` 中期继续向 6/4 的 Zealot 目标推进，没有在 Assimilator 完成后尽早收口。

## 改进方案

- 当 `earlyPvP && countUnits(Protoss_Assimilator, true) >= 1` 时，把 `targetEarlyZealots` 压到最多 4。
- 这样气矿一开，前期狂热者目标就不会继续把矿物吞光。
- 目标是给 Core 和第一批 Dragoon 留出足够的矿，让龙骑能真正成为中期主力。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是对 PvP 兵种配比的关键修正。若它有效，后续窗口诊断里 `minerals` 应该会明显抬高，`dragoons` 也应该不再卡在 0 条。
