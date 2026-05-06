# 010 - Replay PvP Gas Core Timing 改进方案

## 问题背景

2026-05-06 继续从 `/Users/laojiaqi/Downloads/starcraft_replays/pvp/` 学习 PvP 开局。上一处改动提前了第二 Gateway，测试中 `gates@5/6` 达到 3/4，但 `army@6` 仍只有 8，说明产能没有足够早转化为 Dragoon。

## 败因分析（Replay + Log）

| Replay | 地图 | Gas / Core 时间 |
|--------|------|-----------------|
| `TL2104_Dewalt_Shuttle_Textbook_PvP_Fighting_Spirit...rep` | Fighting Spirit | Gas 1:29，Core 1:57 |
| `SB_20260318_Unknown_a_a_vs_BSL-Gosudark.rep` | Blade | Gas 1:32/1:38，Core 1:55/2:39 |
| `SB_20260323_Unknown_oG_Bluesky_vs_BSL-Gosudark.rep` | Blade | 一方 1:33 Gas，2:53 Core；另一方多 Gate 后 3:29 Core |

当前代码的问题：
1. **Gas 条件偏晚**：early PvP 需要 `probeCount >= 15 && zealots >= 2` 才兜底开 gas。
2. **Core 条件偏晚**：要求 `pylons >= 2` 且 `gateways >= 2 && zealots >= 2`。
3. **Replay 中 Core 常在第二 Pylon 前开始**：Textbook 样本 P0 Core 1:57，第二 Pylon 2:18。

## 改进方案

### 1. early PvP 两 Gateway 后 13 Probe 即可 Gas

```cpp
if (gateways >= 2 && probeCount >= 13) {
    readyForGas = true;
}
```

### 2. early PvP Core 不再等待第二 Pylon/2 Zealot

```cpp
pylons >= (earlyPvP ? 1 : 2)
...
(!earlyPvP || gateways >= 2 || enemyDragoons > 0)
```

预期效果：
1. 更接近 replay 的 1:30 Gas、1:55 Core。
2. 更早开始 Dragoon 转型，提升 `army@6` 的质量。
3. 降低 6~7 分钟被 Locutus Dragoon/Zealot timing 打穿的概率。

## 测试结果

| 测试 | 命令 | 结果 |
|------|------|------|
| 构建 | `cmake --build build --target tests` | PASS |
| Locutus 4GateGoon | `./tests --gtest_filter=Locutus.4GateGoon` | FAIL，Empire of the Sun seed 64685，7:21 Loss，`army@5/6/7/8=7/4/0/-1`，`gates@5/6/7/8=2/3/3/-1` |

## 评估

本次只根据 replay 提前 Gas/Core，不声称已解决 Locutus 首胜。实测随机 Empire 样本 `army@6` 下降到 4，说明提前科技需要配套 Zealot 下限，否则会在 Locutus 早压下兵力断档。下一步调整 early PvP 训练优先级，先保证 6~8 个 Zealot 再让 Dragoon 转型接管。
