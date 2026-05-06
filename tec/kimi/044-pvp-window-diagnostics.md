# PvP Window Diagnostics

## 问题背景

前面的 PvP 改动已经很多，但我们仍然无法从结果里直接看出：到底是 `Core pending`、`first Dragoons`，还是 `reserve` 窗口没真正活起来。没有这个证据，后续调整还是容易靠猜。

## 败因分析（Replay + Log）

- Replay：同一类 PvP 失败会落在 7-8 分钟之后，但日志只告诉我们输了，没有告诉哪一个保护窗口持续时间不够。
- Log：现有输出里只有 `coreBuild attempts/frame` 和常规兵种计数，缺少对 PvP 关键窗口的持续帧数统计。
- 代码：`pvpCorePending`、`pvpNeedFirstDragoons`、`reserveForFirstDragoons` 都是关键分支，但之前没有汇总计数。

## 改进方案

- 增加 `pvpCorePendingFrames`、`pvpNeedFirstDragoonsFrames`、`pvpReserveFirstDragoonsFrames` 三个统计项。
- 在 `onEnd` 里输出这些计数，方便对照 replay 判断哪个阶段最短。
- 目标是把下一轮调参建立在可验证的分支覆盖数据上。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这不是策略调整本身，而是给后续 PvP 调整加观测点。若计数显示某个窗口几乎没活过，就能直接针对那一段下手。
