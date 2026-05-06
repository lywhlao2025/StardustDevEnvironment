# PvP Core Gas Diagnostics

## 问题背景

当前测试多次出现 0 Dragoon 的纯 Zealot 结果，但仅凭 army/pylon/gateway 还无法判断是 Core 没落地、Assimilator 没落地，还是训练逻辑没把 Gas 转成 Dragoon。

## 败因分析（Replay + Log）

- Replay：PvP replay 的中期结构要求 2Gate 后尽快落 Assimilator 和 Core，随后开始把 Gas 转成第一批 Dragoon。
- Log：最新 `Andromeda` seed `10484` 为 `army@5/6/7/8=8/12/0/-1`、`zealots@5/6/7/8=8/12/0/-1`、`dragoons@5/6/7/8=0/0/0/-1`、`gates@5/6/7/8=2/3/3/-1`，但缺少自家 `assimilator/core` 时间线，无法进一步定位问题。
- 由于没有 own tech building 诊断，当前只能知道“没出龙骑”，不能知道是 tech 链路缺失还是训练优先级失效。

## 改进方案

- 增加 `assimilatorCountAt5/6/7/8`。
- 增加 `coreCountAt5/6/7/8`。
- 在 `onEnd` 输出 `assimilators@5/6/7/8` 与 `cores@5/6/7/8`，补齐自己的 tech 链路信息。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

本改动是为了把“有气却没龙骑”拆成更细的链路问题。只有知道 Core / Assimilator 的实际时间点，才知道下一步该改建造还是训练。
