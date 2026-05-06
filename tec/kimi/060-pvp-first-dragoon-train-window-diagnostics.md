# PvP First Dragoon Train Window Diagnostics

## 问题背景

我们已经知道 PvP 的开局资源分流、门数约束和补给触发都在动，但第一条 Dragoon 还是不稳定。需要知道训练循环里究竟是哪一个条件在卡它。

## 败因分析（Replay + Log）

- Replay：PvP replay 里，第一条 Dragoon 的训练窗口通常很短，错过就会被后续 zealot / 生产分流挤掉。
- Log：现有日志只能看到最终结果和阶段计数，不能直接看到 `reserveForFirstDragoons`、`zealotFloorNeeded`、`availableArmySupply` 这些训练侧变量。
- 代码：Gateway 训练循环里缺少一次性诊断，无法定位第一条 Dragoon 为什么没被立刻训练。

## 改进方案

- 当 `earlyPvP && cores > 0 && dragoons < 2` 时，打印一次训练窗口诊断。
- 记录矿、气、剩余人口、可用军力人口、第一龙骑保留开关、zealot floor、目标龙骑数和当前门数。
- 目标是把第一条 Dragoon 的训练障碍精确定位出来。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是观测层增强，不改变策略本身。它应该直接告诉我们下一轮该收训练优先级、经济，还是门数。
