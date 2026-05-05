# 007 - PvP Gas Worker Throttle 改进方案

## 问题背景

2026-05-05 多局 Locutus PvP 失败都出现高 gas、低 minerals、Gateway idle 高的问题。典型样本：
- Icarus seed 43406：终局 `minerals=20 gas=448`，`idleFrames(Gateway/Core/Nexus)=9469/0/5427`
- Icarus 固定复测：终局 `minerals=34 gas=312`，`idleFrames(Gateway/Core/Nexus)=6031/0/5392`
- Neo Moon Glaive seed 15139：终局 `minerals=140 gas=250`，`idleFrames(Gateway/Core/Nexus)=6128/2327/5710`

## 败因分析（Replay + Log）

| 信号 | 含义 |
|------|------|
| gas 明显高于 minerals | 采气过多，矿物不足以持续补 Zealot/Dragoon/Pylon |
| Gateway idle 高 | 产能存在但训练资源不足或供给不足 |
| `army@7=0` | 6~7 分钟交战后没有足够续兵 |

核心问题：
1. **Core 后继续采太多气**：Dragoon 需要 gas，但当前失败样本的瓶颈更偏 minerals。
2. **Gateway 续兵需要矿物**：Zealot、Pylon、Gateway 都吃 minerals。
3. **gas bank 不能转化战斗力**：矿物不足时，多余 gas 无法避免主力断档。

## 改进方案

### 1. Core 后 gas 已够且矿物不足时压低采气

```cpp
if (completedCores > 0 && Broodwar->self()->gas() >= 100 && Broodwar->self()->minerals() < 200) {
    desiredGasWorkers = dragoons < 2 ? 1 : 0;
}
```

预期效果：
1. 保留前 2 个 Dragoon 的基本 gas。
2. 之后矿物不足时暂停采气，把 Probe 转回 minerals。
3. 降低 Gateway idle，提升 6~8 分钟续兵能力。

## 测试结果

| 测试 | 命令 | 结果 |
|------|------|------|
| 构建 | `cmake --build build --target tests` | PASS |
| Locutus 4GateGoon | `./tests --gtest_filter=Locutus.4GateGoon` | FAIL，Destination seed 51756，7:38 Loss，`army@5/6/7/8=10/13/0/-1`，`gates@5/6/7/8=3/4/4/-1`，终局 `minerals=8 gas=424` |

## 评估

本次只调整 PvP 气矿工人节流，不声称已解决 Locutus 首胜。实测仍在 7 分钟前主力归零，终局 gas 仍高，说明战斗失败发生得太早，单靠资源回流不能解决。下一步考虑更主动的 PvP cheese/压制路线，争取先拿到对 Locutus 的首胜样本。
