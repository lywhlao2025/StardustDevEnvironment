# PvP Raise Pylon Buffer After Two Dragoons

## 问题背景

现在 PvP 已经能把 Dragon 数量抬到 4 条了，但新的失败原因变成了 `SupplyBlocked`。说明兵能造出来了，却又被补给卡住。

## 败因分析（Replay + Log）

- Replay：PvP replay 里，2 条以上 Dragoon 出现后，补给必须更积极，否则这批兵会直接被 supply cap 锁住。
- Log：`Benzene` seed `72852` 显示 `dragoons@5/6/7/8=1/4/0/0`，同时 `result=SupplyBlocked`、`supplyBlockedSec=68`。这说明兵已经起来，但补给没有跟上。
- 代码：`pylonBufferThreshold` 在 Core 后虽然已经前置，但在 `dragoons >= 2` 的成型窗口里还不够激进。

## 改进方案

- 当 `earlyPvP && dragoons >= 2` 时，把 `pylonBufferThreshold` 提到至少 60。
- 同时把 `pylonBufferCap` 提到至少 6，允许更多 Pylon 并行排队。
- 把 `pylonBufferMinerals` 压到 75，避免在补给上手太晚。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是在兵力成型后的补给纠偏。若它有效，后续日志里的 `supplyBlockedSec` 应该明显下降。
