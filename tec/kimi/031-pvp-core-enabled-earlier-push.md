# PvP Core Enabled Earlier Push

## 问题背景

Core 终于按时落地后，最新结果已经能看到 `dragoons@5/6/7/8=1/3/0/-1`，但整场还是在 7-8 分钟被打穿。说明下一层瓶颈变成了“有 Core 有龙骑，但推进仍然太保守”。

## 败因分析（Replay + Log）

- Replay：PvP replay 中，Core + 第一批 Dragoon 之后通常会更早争夺地图，而不是继续攒到 16 兵再等更久。
- Log：`Empire of the Sun` seed `42703` 为 `cores@5/6/7/8=1/1/1/-1`、`dragoons@5/6/7/8=1/3/0/-1`、`gates@5/6/7/8=3/3/3/-1`。科技链已经通了，但主动权还不够。
- 代码：`minAttackThreshold` 在 PvP 8 分钟前仍保持较高值，虽然之前对纯 Zealot 更安全，但在 Core 成功后的混编阶段会过于保守。

## 改进方案

- 当 `earlyPvP && cores > 0 && dragoons >= 1 && frame < 8min` 时，把主动进攻阈值压到 12。
- 保留敌人进基地时的强制守家逻辑，不破坏防守。
- 目标是让有 Core 的混编部队在 12-15 兵时就开始争夺主动权。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

本改动建立在 Core 已经落地的前提上，专门处理“科技到了但推进太晚”的问题。如果它有效，后续还可以再根据真实战局微调门槛；如果无效，就说明需要回到微操/目标选择层面。
