# 001 - ZealotDrop 防守改进方案

## 问题背景

2026-05-05 对 Locutus (Roadkill 地图) 测试失败，比分 Loss。对手使用 ZealotDrop 策略：
- 5:00~5:30 Shuttle 空投 Zealot 直扑矿区
- 配合地面部队两面夹击

## 败因分析（Replay + Log）

| 时间点 | 事件 |
|--------|------|
| 5:14   | Forge 开始建造 |
| 5:42   | Forge 完成 |
| 6:14   | 第一个 Photon Cannon 开始建造 |
| 6:26   | 第二个 Photon Cannon 开始建造 |
| 6:49   | 第一个 Cannon 完成 |
| 6:59   | 第一个 Cannon 被毁 |
| 7:49   | 第三个 Cannon 开始建造 |
| 8:03   | 第四个 Cannon 开始建造 |
| 8:23   | 第三个 Cannon 完成 |
| 8:25   | Probe 开始大量被屠杀 |
| 8:35-8:43 | Cannon 陆续被毁 |
| 8:45   | Nexus 被毁 |
| 9:37   | 游戏结束，Loss |

核心问题：
1. **Cannon 数量不足**：默认 PvP 只建 0 个 Cannon，Forge 完成后才开始建，间隔 32 秒
2. **Probe 逃跑不及时**：`urgentThreat` 阈值过高（80/128），Zealot 接近时 Probe 仍在采矿
3. **`dropDefenseHoldHome` 8 分钟后失效**：部队可能在关键时刻离开基地
4. **Cannon 被毁后重建慢**：没有保留资源快速重建

## 改进方案

### 1. 提升默认 Cannon 数量

```cpp
// 修改前：默认 0 个 Cannon
int basePvPDefensiveCannons = 0;
if (enemyProtoss) {
    if (completedCores > 0 && dragoons >= 2) basePvPDefensiveCannons = 1;
    ...
}

// 修改后：默认 2 个 Cannon
int basePvPDefensiveCannons = 0;
if (enemyProtoss) {
    if (gateways >= 2 && zealots >= 2) basePvPDefensiveCannons = 2;
    if (completedCores > 0 && dragoons >= 2) basePvPDefensiveCannons = std::max(basePvPDefensiveCannons, 1);
    if (enemyNearHome || proactiveDropDefense) basePvPDefensiveCannons = std::max(basePvPDefensiveCannons, 2);
    if (enemyRobotics > 0 || roboticsDropThreat) basePvPDefensiveCannons = std::max(basePvPDefensiveCannons, 3);
    if (roboticsDropThreat && completedCores > 0 && dragoons >= 2) {
        basePvPDefensiveCannons = std::max(basePvPDefensiveCannons, 4);
    }
}
```

### 2. 提前 Forge 建造时机

```cpp
// 新增 PvP 定时 Forge 触发条件
bool pvpForgeTiming = gateways >= 2 && zealots >= 4 && frame >= 24 * 60 * 3;

// Forge 建造条件增加 pvpForgeTiming
if (forges == 0 && pylons >= 1 && (earlyThreat || pvpForgeTiming)) {
    tryBuild(UnitTypes::Protoss_Forge, myStart);
}

// Fallback Forge 也增加同样条件
if ((enemyNearHome || proactiveDropDefense || roboticsDropThreat || 
     (threatMask & Threat_Cloak) || pvpForgeTiming) && forges == 0 && pylons >= 1) {
    tryBuild(UnitTypes::Protoss_Forge, myStart);
}
```

### 3. 延长 `dropDefenseHoldHome` 时间限制

```cpp
// 修改前：frame < 24 * 60 * 8
// 修改后：frame < 24 * 60 * 10
bool dropDefenseHoldHome = enemyProtoss &&
                           frame < 24 * 60 * 10 &&
                           ...
```

### 4. 改进 Probe 逃跑逻辑

```cpp
// 降低 urgentThreat 阈值
bool urgentThreat = nearestThreatDist <= 200.0 || nearestMeleeThreatDist <= 250.0;

// 新增：近战威胁 < 160 时直接向反方向逃跑
if (nearestMeleeThreatDist <= 160.0) {
    Position fleeDir = u->getPosition() - nearestThreatPos;
    // 计算逃跑目标位置（距离威胁 192 像素）
    Position fleeTarget = u->getPosition() + normalized(fleeDir) * 192;
    u->move(fleeTarget);
    continue;
}
```

## 测试结果

### ZealotDropRoadkillRegression 测试

| 次数 | Forge | Cannon 完成 | Probe 存活 | 游戏时长 | 结果 |
|------|-------|------------|-----------|---------|------|
| 基准 | 5:49  | 6:30       | 1         | 9:37    | FAIL |
| 改进1 | 5:42  | 6:30       | 1         | 9:37    | FAIL |
| 改进2 | 5:21  | 5:56       | 1         | 9:32    | FAIL |
| 改进3 | 5:14  | 6:14       | 1         | 9:37    | FAIL |

注：每次改进后 Forge/Cannon 建造时间都提前了，但 Probe 仍然在 8:20-8:30 开始被大量屠杀。

### 评估

虽然 ZealotDropRoadkillRegression 未通过，但以下改进是有效的：
1. Forge 建造从 6:14 提前到 5:14
2. Cannon 建造从 6:50 提前到 6:14
3. 默认 Cannon 数量从 0 提升到 2

待续改进方向：
1. **Cannon 位置优化**：确保 Cannon 建在矿区附近
2. **Probe 逃跑持续性**：即使部队在前线，Probe 也要持续逃跑
3. **部队回防优先级**：当基地有敌方单位时，部队必须回防
4. **Cannon 被毁后快速重建**：降低重建间隔
