# PvP Core Attempt Tracing

## 问题背景

Core 仍然是 0，且仅靠现有日志看不出是“没尝试”还是“反复尝试但失败”。为了继续推进，需要把 Core 的尝试次数和首次尝试时间记录出来。

## 败因分析（Replay + Log）

- Replay：PvP replay 要求 Core 在 Gas 后尽快完成；如果 Core 迟迟不出现，后续任何 Dragoon/科技判断都失效。
- Log：`Andromeda` seed `35259` 为 `cores@5/6/7/8=0/0/0/-1`、`zealots@5/6/7/8=8/14/0/-1`、`gates@5/6/7/8=3/3/3/-1`。当前无法从日志看出 Core 是没被尝试，还是尝试失败。
- 代码：Core 的 `tryBuild` 还没有单独的尝试计数，无法知道它是否在关键窗口反复被调度。

## 改进方案

- 增加 `firstCoreBuildAttemptFrame`。
- 增加 `coreBuildAttemptCount`.
- 在 `onEnd` 中输出 `coreBuild attempts/frame`，为下一步修正 Core 触发或建造点提供证据。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

本改动不改变行为，只把 Core 建造链的调度事实记录下来。它是为了避免后续继续在错误层面上盲调。
