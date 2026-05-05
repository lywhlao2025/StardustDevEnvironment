# 004 - PvP Rush Gateway Timing 改进方案

## 问题背景

2026-05-05 新增诊断后，`Locutus.4GateGoon` 失败样本显示：
- 地图：La Mancha
- Seed：43506
- 结果：7:38 Loss
- `army@5/6/7/8=7/7/0/-1`
- `gates@5/6/7/8=2/2/3/-1`

Locutus 4Gate 系样本在 6:56 常见 19~21 个战斗单位。我方 6 分钟仍只有 2 个完成 Gateway，7 分钟前主力被打光。

## 败因分析（Replay + Log）

| 时间点 | 我方状态 |
|--------|----------|
| 5:00   | 7 个战斗单位，2 个完成 Gateway |
| 6:00   | 7 个战斗单位，2 个完成 Gateway |
| 7:00   | 0 个战斗单位，3 个完成 Gateway |
| 7:38   | 主基地被推平，Loss |

核心问题：
1. **第 3 Gateway 太晚**：原逻辑在 early PvP 下要求 Core 完成或在建才补 rush Gateway。
2. **第 4 Gateway 缺少 rush 快速分支**：需要等 Dragoon 数或高矿物触发，赶不上 Locutus 的 4Gate 压力。
3. **战斗单位断档**：5~6 分钟 army 数没有增长，说明生产能力不足。

## 改进方案

### 1. PvP rush 下提前补第 3/4 Gateway

```cpp
bool pvpGatewayCatchup = earlyPvP &&
                         gateways < 4 &&
                         pylons >= 2 &&
                         probeCount >= 16 &&
                         (enemyRush ||
                          enemyDragoons > 0 ||
                          (threatMask & Threat_TechRush) ||
                          frame >= 24 * 60 * 4);
if (pvpGatewayCatchup && Broodwar->self()->minerals() >= 150) {
    tryBuild(UnitTypes::Protoss_Gateway, gatewayBuildNear);
}
```

预期效果：
1. 不再等待 Core 完成/在建才响应 PvP rush 的产能需求。
2. 看到 rush/Dragoon/tech 时立即追第 3/4 Gateway；侦查不到时 4 分钟兜底追产能。
3. 提升 `gates@6/7` 与 `army@6/7`。

## 测试结果

| 测试 | 命令 | 结果 |
|------|------|------|
| 构建 | `cmake --build build --target tests` | PASS |
| Locutus 4GateGoon | `./tests --gtest_filter=Locutus.4GateGoon` | FAIL，Fighting Spirit seed 84907，7:24 Loss，`army@5/6/7/8=7/6/0/-1`，`gates@5/6/7/8=2/3/3/-1` |

## 评估

本次只调整 PvP rush 产能时机，不声称已解决 Locutus 首胜。实测 `gates@6` 从前一诊断样本的 2 提升到 3，但 `army@6` 仍只有 6 且 7 分钟前被打光。下一步重点检查防守交战：敌军进家时是否需要拉 Probe 协防，避免少量 Gateway 单位被 Locutus 正面滚死。
