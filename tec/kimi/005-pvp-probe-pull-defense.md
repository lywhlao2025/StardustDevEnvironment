# 005 - PvP Probe Pull Defense 改进方案

## 问题背景

2026-05-05 `Locutus.4GateGoon` 仍失败。最近样本：
- 地图：Fighting Spirit
- Seed：84907
- 结果：7:24 Loss
- `army@5/6/7/8=7/6/0/-1`
- `gates@5/6/7/8=2/3/3/-1`

第 3 Gateway 追赶后，6 分钟 Gateway 数有所改善，但主力仍在 7 分钟前被打光。

## 败因分析（Replay + Log）

| 时间点 | 我方状态 |
|--------|----------|
| 5:00   | 7 个战斗单位 |
| 6:00   | 6 个战斗单位，3 个完成 Gateway |
| 7:00   | 0 个战斗单位 |
| 7:24   | 主基地被推平，Loss |

核心问题：
1. **Probe 只逃跑不协防**：敌军进基地时，当前 worker 逻辑主要是 flee/gather。
2. **小规模兵力容易被滚死**：6~7 个 Gateway 单位打不过 Locutus 进攻波时，没有 Probe 补足短期战斗力。
3. **经济已无法保全**：敌军进家并打光主力时，继续纯采矿没有意义。

## 改进方案

### 1. 敌军进基地且主力不足时拉 Probe 协防

```cpp
bool pullWorkerDefense = enemyInBaseNow &&
                         armyCount < 10 &&
                         probeCount > 8 &&
                         pulledDefenseWorkers < 10 &&
                         nearestThreat &&
                         nearestThreatDist <= 420.0;
if (pullWorkerDefense) {
    u->attack(nearestThreat);
    pulledDefenseWorkers++;
    continue;
}
```

预期效果：
1. 主基地 600 像素内有敌军时，最多拉 10 个 Probe 协防。
2. 只在 `armyCount < 10` 时触发，避免优势兵力时扰乱经济。
3. 保留 `probeCount > 8` 下限，避免把最后经济完全拉空。

## 测试结果

| 测试 | 命令 | 结果 |
|------|------|------|
| 构建 | `cmake --build build --target tests` | PASS |
| Locutus 4GateGoon | `./tests --gtest_filter=Locutus.4GateGoon` | FAIL，Icarus seed 43406，7:38 Loss，`army@5/6/7/8=8/15/0/-1`，`gates@5/6/7/8=3/4/4/-1` |

## 评估

本次改变敌军进基地时的 worker 协防行为，不声称已解决 Locutus 首胜。实测配合第 3/4 Gateway timing 后，`gates@6` 达到 4，`army@6` 达到 15，但 `army@7` 仍为 0。下一步重点防止 15 个兵过早出门或接劣势战，调整 early PvP 的进攻阈值/守家条件。
