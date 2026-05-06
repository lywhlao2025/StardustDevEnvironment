# PvP Two-Dragoon Earlier Push

## 问题背景

最新结果已经出现 `cores=1` 和 `dragoons=2`，说明科技链完成了，当前瓶颈转成了推进时机。部队仍旧在 7 分钟左右被换掉，说明还在守家而不是借着龙骑窗口前压。

## 败因分析（Replay + Log）

- Replay：PvP replay 里一旦 2 个 Dragoon 成型，通常就会开始争夺主动，不会一直等到更大规模才出门。
- Log：`Roadrunner` seed `56087` 为 `dragoons@5/6/7/8=0/2/0/-1`、`cores@5/6/7/8=1/1/1/-1`、`army@5/6/7/8=7/7/0/-1`。科技链打通后，7 分钟前后还是没有形成有效压制。
- 代码：当前只有 `dragoons >= 1` 时才把进攻阈值压到 12；对已成型的 2 Dragoon 窗口仍然偏保守。

## 改进方案

- 当 `earlyPvP && cores > 0 && dragoons >= 2 && frame < 8min` 时，把主动进攻阈值压到 10。
- 保留 `enemyInBase` 和其他守家逻辑，不破坏防守。
- 目标是让 2 Dragoon+Zealot 的小型混编提早去争夺地图，而不是等到更大规模才出门。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

本改动建立在 Core 已成、第一批 Dragoon 已出现的前提上，专门针对推进过晚的问题。若有效，后续再观察是否需要进一步提升地图争夺力度。
