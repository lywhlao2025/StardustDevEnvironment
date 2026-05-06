# PvP Core Hard Stop 300

## 问题背景

之前把 `mineralReserve` 提高到 150 仍然没有让 Core 落地。最新日志显示 `zealots` 继续增长到 12 甚至 14，而 `cores` 仍然是 0，说明需要更硬的刹车来阻止 Gateway 把矿物继续兑成兵。

## 败因分析（Replay + Log）

- Replay：PvP replay 中，2Gate 后的核心是尽快落 Core，然后再转 Dragon/混编；无限堆 Zealot 会把科技一直拖死。
- Log：`Neo Moon Glaive` seed `97581` 为 `assimilators@5/6/7/8=1/1/1/-1`、`cores@5/6/7/8=0/0/0/-1`、`zealots@5/6/7/8=9/14/0/-1`、`dragoons@5/6/7/8=0/0/0/-1`，而矿物在中后期仍不断被消费。
- 代码：当前 Core 未落地时，Gateway 训练循环在矿物足够时仍会继续训练 Zealot。仅仅“预留”不足以阻止矿物被持续消耗。

## 改进方案

- 当 `earlyPvP && cores == 0 && assimilator >= 1 && gateways >= 2 && zealots >= 8` 时，如果矿物低于 300，就直接停止当前 Gateway 训练循环。
- 这会让中期矿物先攒给 Core，而不是继续兑成 Zealot。
- 仍保留 Core 成功后正常的 Zealot/Dragoon 混编逻辑。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

本改动是对“Core 永远出不来”的最后一级硬保护。若它仍失败，接下来必须定位 Core 实际建造点或 build order 外的阻塞。
