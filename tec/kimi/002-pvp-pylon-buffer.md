# 002 - PvP Pylon Buffer 改进方案

## 问题背景

2026-05-05 对 Locutus `4GateGoon` 测试失败。样本：
- 地图：Icarus
- Seed：50655
- 结果：Loss
- 终局诊断：`supplyBlockedSec=55`，`idleFrames(Gateway/Core/Nexus)=3277/1076/5061`
- 6:56 附近仍有约 17 个战斗单位、619 minerals、212 gas，但后续生产被供给卡顿拖慢。

## 败因分析（Replay + Log）

| 时间点 | 事件 |
|--------|------|
| 5:02   | Locutus 约 5 个战斗单位开始进攻 |
| 5:33   | Locutus 约 9 个战斗单位 |
| 6:56   | Locutus 约 17 个战斗单位，我方出现明显供给/生产空转 |
| 7:50   | 我方被清空，结果 Loss |

核心问题：
1. **Pylon buffer 偏保守**：三门以上 PvP 产兵速度快，`projectedSupply <= 18` 才补额外 Pylon 容易晚。
2. **在途 Pylon 数量偏少**：高压 PvP 下最多 2 个在途 Pylon 不足以覆盖连续 Gateway 训练。
3. **矿物仍有余量**：失败样本里矿物没有完全用尽，说明更早补 Pylon 有空间。

## 改进方案

### 1. 三门 PvP 提前补 Pylon

```cpp
int pylonBufferThreshold = (earlyPvP && gateways >= 3) ? 28 : 18;
int pylonBufferCap = (earlyPvP && gateways >= 3) ? 3 : 2;
int pylonBufferMinerals = (earlyPvP && gateways >= 3) ? 150 : 200;
if ((projectedSupply <= pylonBufferThreshold) && pylonInProgress < pylonBufferCap &&
    Broodwar->self()->minerals() >= pylonBufferMinerals) {
    tryBuild(UnitTypes::Protoss_Pylon, myStart);
}
```

预期效果：
1. 三门以上 PvP 在剩余/在途供给低于 14 人口时就开始补额外 Pylon。
2. 允许最多 3 个 Pylon 在途，降低 Gateway 连续训练时的 supply block。
3. 非 PvP 或低 Gateway 场景保持原阈值，避免过度占用矿物。

## 测试结果

| 测试 | 命令 | 结果 |
|------|------|------|
| 构建 | `cmake --build build --target tests` | PASS |
| Locutus 4GateGoon | `./tests --gtest_filter=Locutus.4GateGoon` | FAIL，Andromeda seed 8075，7:33 Loss，`supplyBlockedSec=58`，`idleFrames(Gateway/Core/Nexus)=4451/1849/5379` |

## 评估

本次只解决供给缓冲，不声称已解决 Locutus 首胜。实测 `4GateGoon` 仍失败，且供给阻塞秒数仍高，说明瓶颈不只是 Pylon 触发阈值。下一步继续检查 Pylon 建造成功率、建筑位置与作战阶段是否导致 Pylon/Gateway 被提前击毁。
