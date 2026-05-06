# PvP Core Raise Probe Cap To Eighteen

## 问题背景

现在 PvP 已经能更早出第一条 Dragoon，但最后又变成 `LowEcon`。说明开局资源分流修正是对的，但 Probe 上限仍然过低，后续经济没有跟上。

## 败因分析（Replay + Log）

- Replay：PvP replay 里，Core 开始正常工作后，经济也必须继续补上，否则后面即使有 Dragoon 也会因为经济薄而输。
- Log：`Empire of the Sun` seed `92218` 显示 `result=Loss reason=LowEcon`，同时最终只有 `16` 个 worker。这说明经济面没有重新抬起来。
- 代码：`earlyPvP` 下的 `pvpProbeCap` 仍然固定在 16，Core 已经起来后也没有及时放宽到 18。

## 改进方案

- 当 `earlyPvP && (completedCores > 0 || dragoons > 0)` 时，把 `pvpProbeCap` 提到 18。
- 这样不会把经济长期锁死在 16 Probe，Core 成立后也能继续补经济。
- 目标是避免把局面从“兵不够”修成“经济太差”，让两者同步恢复。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是一次经济收口修正，应该和前面的 Dragoon 提前窗口配合使用。若它有效，最终日志里 worker 数和资源结余应该不再过低。
