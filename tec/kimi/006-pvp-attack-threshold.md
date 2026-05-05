# 006 - PvP Attack Threshold 改进方案

## 问题背景

2026-05-05 `Locutus.4GateGoon` 在生产改善后仍失败。最近样本：
- 地图：Icarus
- Seed：43406
- 结果：7:38 Loss
- `army@5/6/7/8=8/15/0/-1`
- `gates@5/6/7/8=3/4/4/-1`

第 3/4 Gateway timing 已经让 6 分钟兵力达到 15，但 7 分钟前仍被打光。

## 败因分析（Replay + Log）

| 时间点 | 我方状态 |
|--------|----------|
| 5:00   | 8 个战斗单位，3 个完成 Gateway |
| 6:00   | 15 个战斗单位，4 个完成 Gateway |
| 7:00   | 0 个战斗单位 |
| 7:38   | 主基地被推平，Loss |

核心问题：
1. **主动出门阈值偏低**：early PvP 下 enemyRush 未在家附近时阈值可低至 8，tech/Dragoon 场景也只抬到 14。
2. **6 分钟 15 兵仍可能接劣势战**：Locutus 4Gate/Proxy4Gate/ZealotDrop 在 6:30~7:00 常见 17~21 个战斗单位。
3. **守家优势没有利用**：在家附近接战更容易获得新兵、Probe 协防和建筑地形支援。

## 改进方案

### 1. 8 分钟前 early PvP 非 greed 对手抬高出门阈值

```cpp
if (earlyPvP && !enemyGreed && frame < 24 * 60 * 8) {
    minAttackThreshold = std::max(minAttackThreshold, 20);
}
```

预期效果：
1. 8 分钟前至少 20 个战斗单位才主动出门。
2. 让 6~7 分钟的 15 个战斗单位优先守家。
3. 配合 Probe 协防，降低 `army@7=0` 的概率。

## 测试结果

| 测试 | 命令 | 结果 |
|------|------|------|
| 构建 | `cmake --build build --target tests` | PASS |
| Locutus 4GateGoon | `./tests --gtest_filter=Locutus.4GateGoon` | FAIL，Neo Moon Glaive seed 15139，7:39 Loss，`army@5/6/7/8=8/5/0/-1`，`gates@5/6/7/8=3/3/3/-1` |

## 评估

本次只调整 early PvP 主动进攻阈值，不声称已解决 Locutus 首胜。随机地图样本仍失败，并且 6 分钟 army 只有 5，说明不同地图上生产/交战波动仍很大。下一步需要稳定对比固定地图/seed，避免随机地图噪声掩盖改动效果。
