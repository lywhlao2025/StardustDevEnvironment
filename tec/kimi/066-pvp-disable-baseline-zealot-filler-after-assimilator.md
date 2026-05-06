# PvP Disable Baseline Zealot Filler After Assimilator

## 问题背景

最新排查已经证明，真正把矿吃掉的不是后面的 Dragoon 分支，而是更早的“基础 zealot 补兵器”。它在 PvP 气矿已经上线后仍然持续补 zealot，直接把 Core 和 Dragoon 的资源挤没了。

## 败因分析（Replay + Log）

- Replay：PvP replay 里，Assimilator 完成后，基础 zealot 填充器不应该继续无脑补兵。
- Log：`Jade` seed `61762` 和多个后续样本都显示，在气矿上线后 zealot 仍会继续升到 10+，而 minerals 反而掉到 30-40，dragoon 卡住。
- Code：`if (!reserveForFirstDragoons && !pvpPreferRangedMidgame && zealots < 48 && supplyRemaining >= 4 && minerals >= 100)` 这条基础 zealot 填充分支没有在 `Assimilator >= 1` 的 PvP 场景里停下来。

## 改进方案

- 在 `earlyPvP && Assimilator 已完成` 时，直接关闭这条基础 zealot 填充分支。
- 这样资源会转向 Core 和 Dragoon，而不是继续被 zealot 吃掉。
- 目标是从源头切断 early PvP 的 zealot flood。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是对最早的 zealot 自动补兵入口的硬关停。若它有效，4-5 分钟时的矿物应该明显抬高，Core 和 Dragoon 的成型会更顺。
