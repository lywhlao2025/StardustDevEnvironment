# 009 - Replay PvP Early Second Gateway 改进方案

## 问题背景

2026-05-06 根据 `/Users/laojiaqi/Downloads/starcraft_replays/pvp/` 的 PvP replay 重新学习开局节奏。此前多轮 Locutus 测试显示，6 分钟前产能和兵力波动较大，经常在 7 分钟前主力归零。

## 败因分析（Replay + Log）

| Replay | 地图 | 关键 PvP 开局时间 |
|--------|------|-------------------|
| `TL2104_Dewalt_Shuttle_Textbook_PvP_Fighting_Spirit...rep` | Fighting Spirit | Pylon 0:47，Gateway 1:13/1:14，Gas 1:29，Core 1:57 |
| `SB_20260318_Unknown_a_a_vs_BSL-Gosudark.rep` | Blade | Pylon 0:47/0:48，Gateway 1:13/1:17，Gas 1:32/1:38，Core 1:55 |
| `SB_20260323_Unknown_oG_Bluesky_vs_BSL-Gosudark.rep` | Blade | Pylon 0:48/0:49，Gateway 1:14/1:16，一方 1:41 前连续补多 Gateway |

核心问题：
1. **当前第二 Gateway 偏晚**：原逻辑要求 `pylons >= 2 && supplyUsed >= 16*2`，比 replay 中 1:13~1:17 的第二 Gateway 节奏慢。
2. **前 5~6 分钟兵力不稳定**：Locutus 样本中 `army@6` 经常只有 5~14，第二 Gateway 延迟会放大这种波动。
3. **真实 PvP 不先裸静态防御**：样本优先用 2Gate/多Gate 产能撑住前期。

## 改进方案

### 1. early PvP 下提前第二 Gateway

```cpp
if (earlyPvP && gateways < 2 && pylons >= 1 && supplyUsed >= 12 * 2) {
    tryBuild(UnitTypes::Protoss_Gateway, gatewayBuildNear);
}
```

预期效果：
1. early PvP 第二 Gateway 不再等待第二个 Pylon。
2. 更接近 replay 中 1:13~1:17 的双 Gateway 节奏。
3. 提升 `gates@5/6` 和 `army@5/6` 稳定性。

## 测试结果

| 测试 | 命令 | 结果 |
|------|------|------|
| 构建 | `cmake --build build --target tests` | PASS |
| Locutus 4GateGoon | `./tests --gtest_filter=Locutus.4GateGoon` | FAIL，Circuit Breaker seed 94691，7:21 Loss，`army@5/6/7/8=9/8/0/-1`，`gates@5/6/7/8=3/4/4/-1` |

## 评估

本次只根据 replay 提前 early PvP 第二 Gateway，不声称已解决 Locutus 首胜。实测 `gates@5/6` 达到 3/4，但 `army@6` 仍只有 8，说明下一步应继续参考 replay 的 Gas/Core/Dragoon 时间点，提前把产能转化为 Dragoon。
