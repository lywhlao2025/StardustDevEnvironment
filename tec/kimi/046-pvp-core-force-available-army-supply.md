# PvP Core Force Available Army Supply

## 问题背景

现在已经确认 `Core`、`reserve`、`pvpWindow` 都在工作，但 `dragoons` 还是经常停在 0-2 条。这说明真正卡住的可能不是优先级本身，而是训练循环里可用军力人口被算得过低。

## 败因分析（Replay + Log）

- Replay：PvP replay 里，`Core` 成立后的前几条 Dragoon 必须尽快落地，否则 Gateway 的空闲时间只会继续堆高。
- Log：`Python` seed `60602` 显示 `dragoons@5/6/7/8=0/0/0/0`，但 `pvpWindowFrames corePending/firstDragoons/reserve=2069/3385/2814` 说明保护窗口已经在跑，问题不在“窗口没有生效”。
- 代码：`availableArmySupply = (supplyRemaining / 2) - probeQueued` 可能过早变成 0，导致 Gateway 训练循环直接退出。

## 改进方案

- 在 `earlyPvP && completedCores > 0 && dragoons < 4` 时，把 `availableArmySupply` 至少抬到 8。
- 这样即使探机队列或其它保守计算把军力人口压得太低，Core 后的前几条 Dragoon 仍然能被训练出来。
- 目标是让 Gateway 的空闲时间转成实际兵力，而不是被过于保守的人口计算吞掉。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是一次训练侧的保守修正，只放宽 PvP Core 后的前几条龙骑。若它生效，下一步可以继续观察 `dragoons` 是否开始超过 2 条并转成主动优势。
