# PvP First Dragoon Window Diagnostics

## 问题背景

我们已经知道 PvP 的 Core、补给和训练窗口都在运行，但还不知道第一条/前四条 Dragoon 在落地前到底被哪个数值卡住。缺少这个信息，就很难继续往下缩小问题。

## 败因分析（Replay + Log）

- Replay：当前失败经常发生在 7 到 8 分钟，Core 已经出现，但龙骑还是停在 0-2 条。
- Log：现有日志只能看到最终兵种数量，看不到那一刻的 `minerals`、`gas`、`supplyRemaining`、`availableArmySupply` 和 `probeQueued`。
- 代码：`availableArmySupply` 之后没有针对第一条龙骑窗口做一次性诊断输出。

## 改进方案

- 当 `earlyPvP && completedCores > 0 && dragoons < 4` 时，只打印一次窗口诊断。
- 记录矿、气、剩余人口、可用军力人口、probe 队列和当前 gateway/兵种数量。
- 目标是把下一轮调整建立在“到底缺什么”的证据上，而不是继续猜。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是观测点，不是策略本身。它应该直接告诉我们第一条龙骑卡在了资源、人口，还是训练优先级。
