# PvP Core Pylon Fallback

## 问题背景

新增的 own tech 诊断显示多场失败里 `assimilator` 已经完成，但 `core` 一直是 0，导致 `dragoons` 也始终是 0。说明瓶颈不在 Gas，而在 Core 的实际建造点和落位成功率。

## 败因分析（Replay + Log）

- Replay：2Gate/Gas/Core 的 replay 节奏要求 Core 在第一座 Pylon 附近尽快落地，然后才能把 Gas 转成 Dragoon。
- Log：`Andromeda` seed `10484` 为 `assimilators@5/6/7/8=1/1/1/-1`、`cores@5/6/7/8=0/0/0/-1`、`dragoons@5/6/7/8=0/0/0/-1`。Gas 已有，但 Core 没落地，训练链路自然无法转龙骑。
- 代码：Core 只尝试在 `myStart` 附近下，若主基地地形或可建区域不理想，会出现实际不下 Core 的情况。

## 改进方案

- 增加 `coreBuildNear`，优先用 `firstCompletedPylonTile()` 作为 Core 建造锚点。
- 如果该位置失败，再回退到 `myStart`。
- 保留原有 Core 触发条件，不扩大科技时机，只提升落地成功率。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

本改动专门针对“有气无 Core”的链路断点。若 Core 能按时出现，后续才能继续评估 Dragoon 比例和前压时机是否还需要调。
