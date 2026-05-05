# 008 - PvP Proxy Enable 改进方案

## 问题背景

2026-05-05 正面防守 Locutus 4Gate/Proxy4Gate/ZealotDrop 仍未拿到首胜。最近改动已使部分样本达到：
- `gates@6=4`
- `army@6=13~15`

但 7 分钟前仍经常 `army@7=0`，说明纯正面防守路线很难快速突破。

## 败因分析（Replay + Log）

| 方向 | 当前问题 |
|------|----------|
| 正面 4Gate 防守 | 6 分钟兵力仍常低于 Locutus，7 分钟前被打光 |
| Drop 防守 | Forge/Cannon/Dragoon 节奏仍不能稳定保住矿区 |
| 随机策略池 | Locutus 不只 4Gate，还有 FakeDTRush/ZealotDrop/Proxy4Gate |

核心问题：
1. **只被动接招**：当前 PvP 禁用了 proxy pylon，默认让 Locutus 按计划展开。
2. **已有 proxy 代码未用于 PvP**：模块已经支持 proxy pylon 完成后补 2 个 proxy Gateway。
3. **首胜优先级高**：目标是先拿到对 Locutus 的第一局真实胜利，cheese 路线值得尝试。

## 改进方案

### 1. PvP 也允许 proxy pylon

```cpp
if (enemyStart && !proxyPylonStarted && scoutProbe && Broodwar->self()->minerals() >= 100) {
    ...
}
```

修改点：
1. 移除 “敌人为 Protoss 时禁止 proxy” 的条件。
2. 复用既有 `proxyPylonCompleted && proxyGateways < 2` 逻辑。
3. 让 PvP 具备主动干扰/cheese 的首胜路径。

## 测试结果

| 测试 | 命令 | 结果 |
|------|------|------|
| 构建 | `cmake --build build --target tests` | PASS |
| Locutus 4GateGoon | `./tests --gtest_filter=Locutus.4GateGoon` | FAIL，Roadrunner seed 66679，7:51 Loss，`army@5/6/7/8=6/6/0/-1`，`gates@5/6/7/8=4/4/4/-1` |
| Locutus RunTwenty | `./tests --gtest_filter=Locutus.RunTwenty` | PASS harness，实际 0 胜 20 负 |

## 评估

本次启用 PvP proxy，不声称已解决 Locutus 首胜。实测 0/20，说明仅打开 proxy 不够；当前 early PvP 20 兵出门阈值会压住 proxy all-in。下一步让 `proxyMode` 使用更低进攻阈值，并避免把 proxy 产能变成被动守家。
