# PvP Aggressive Pylon Buffer At Two Dragoons

## 问题背景

当前失败仍然大量落在 `SupplyBlocked`，说明成型期的补给仍然没有提前到位。既然矿量在窗口里已经够买一座 pylon，就应该更积极地补。

## 败因分析（Replay + Log）

- Replay：PvP replay 里，2 Dragoon 之后的补给需求会快速上升，不能还按偏保守的 buffer。
- Log：`Jade` seed `7149` 在 4:56 时已经有 `minerals=86 gas=192`，但后续仍然出现 `supplyBlockedSec=64`，说明补给触发还是晚了。
- Code：`pylonBufferThreshold` 对成型期还不够激进，`pylonBufferMinerals` 也偏高。

## 改进方案

- 当 `earlyPvP && dragoons >= 2` 时，把 `pylonBufferThreshold` 提到至少 72。
- 同时把 `pylonBufferCap` 提到至少 7，允许更密集的 pylon 排队。
- 把 `pylonBufferMinerals` 压到 25，让补给不会因为保守阈值而拖延。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是补给侧更激进的前置修正。若它有效，`SupplyBlocked` 的时间应该进一步下降，并且 2 Dragon 之后不会再被补给卡死。
