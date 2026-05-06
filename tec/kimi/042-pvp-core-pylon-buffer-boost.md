# PvP Core Pylon Buffer Boost

## 问题背景

当前 PvP 仍然频繁出现 `SupplyBlocked` 或者带着很低的 supply total 进入中期，导致 `Core` 和 `Dragoon` 都不能稳定兑现。补给跟不上时，前面所有科技和兵种调整都会被打断。

## 败因分析（Replay + Log）

- Replay：PvP replay 里，`Core` 成立后，Pylon 必须更早进入 buffer 状态，否则 Gateway 生产会在 30-50 supply 区间反复停摆。
- Log：`La Mancha` seed `44238` 显示 `supply@5/6/7/8=76/98 72/114 2/32 -1/-1`，说明 7 分钟附近供给已经非常危险，甚至掉到 32 total 的低位。
- 代码：`pylonBufferThreshold` 在 `earlyPvP && cores > 0` 时仍然只把矿门槛压到 75，太晚。

## 改进方案

- 在 `earlyPvP && cores > 0` 时，把 `pylonBufferThreshold` 提到至少 52。
- 同时把 `pylonBufferCap` 提到至少 5，允许连续排队更多 Pylon。
- 把 `pylonBufferMinerals` 压到 50，让补给不再被 75 矿门槛卡住。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是把补给前置到 Core 中期的保守修正。若它能降低 supply block 时间，后面再继续看是否需要进一步提前首个 Dragoon 的出门节奏。
