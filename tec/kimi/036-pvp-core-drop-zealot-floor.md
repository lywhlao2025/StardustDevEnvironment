# PvP Core Drop Zealot Floor

## 问题背景

现在 `Core` 已经能落地，但战报里仍然常见 `cores@5/6/7/8=0/1/1/0`、`dragoons@5/6/7/8=0/1/0/0` 这种情况。说明 `Core` 之后的 Gateway 产能还在继续补 Zealot 门槛，没有及时切到 Dragoon。

## 败因分析（Replay + Log）

- Replay：PvP replay 里，`Core` 成功后应该尽快把 Gateway 切到 Dragoon，而不是继续维持前期的 Zealot floor。
- Log：`Circuit Breaker` seed `93505` 显示 `zealots@5/6/7/8=9/9/0/0`、`dragoons@5/6/7/8=0/1/0/0`、`cores@5/6/7/8=0/1/1/0`。这说明核心科技已经出现，但兵种切换仍然偏慢。
- 代码：`zealotFloorNeeded` 只看 `earlyPvP && zealots < targetEarlyZealots`，没有在 Core 落地后释放龙骑生产。

## 改进方案

- 把 `zealotFloorNeeded` 收紧为 `earlyPvP && cores == 0 && zealots < targetEarlyZealots`。
- 让 `Core` 落地后，Gateway 生产更早转向 Dragoon，避免继续堆不够决定性的狂热者。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是一个只影响 PvP 开本的兵种切换修正。若它能把第一批 Dragoon 数量往前推，下一步再看是否还需要调进攻阈值或补给缓冲。
