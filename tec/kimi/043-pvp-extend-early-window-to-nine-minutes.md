# PvP Extend Early Window To Nine Minutes

## 问题背景

当前 PvP 的一系列保护逻辑都挂在 `earlyPvP` 上，但它只持续到 7 分钟。很多失败日志已经发生在 7:20、7:46、8:58，说明这些保护提前失效了。

## 败因分析（Replay + Log）

- Replay：PvP replay 中，7 到 9 分钟仍然是 `Core`、第一批 Dragoon、补给和门数调整的关键窗口，不能把它当成“已经进入中后期”的阶段。
- Log：`Benzene` seed `74116`、`Neo Moon Glaive` seed `41737`、`La Mancha` seed `44238` 都是在 7 分钟之后失败，说明现有 7 分钟窗口太短。
- 代码：`earlyPvP = enemyProtoss && frame < 24 * 60 * 7`，导致 `pvpCorePending`、`pvpNeedFirstDragoons`、pylon buffer、zealot floor 等逻辑都过早失效。

## 改进方案

- 把 `earlyPvP` 的时限扩展到 9 分钟。
- 保持其它对局逻辑不变，只延长 PvP 的特殊保护窗口。
- 目标是让 Core、第一批 Dragoon、补给前置和兵种切换在真实失败点覆盖范围内继续生效。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是一次窗口修正，不是新策略。若它有效，前面那些 PvP 专用约束才有机会在 8 分钟左右真正发挥作用。
