# Replay PvP Earlier Counterpush

## 问题背景

当前 build order 可以在部分地图 6 分钟形成 14-15 兵，但被动守家常在 7 分钟前后被 Locutus 清空。一直等到 20 兵才主动进攻，可能让 Locutus 4Gate 拿到更舒服的进攻节奏。

## 败因分析（Replay + Log）

- Replay：PvP replay 中 2Gate/4Gate 不是长期龟缩到 20 兵后才行动，常见节奏是在前期兵力成型后就争夺地图位置或前压。
- Log：多场失败表现为 `army@6` 达到 14-15，但 `army@7` 归零，例如 Icarus `9/14/0`、Benzene `9/15/0`。
- 代码：PvP 8 分钟前非贪婪敌人会把 `minAttackThreshold` 至少拉到 20，导致 14-16 兵阶段只能守家或被动接战。

## 改进方案

- 将 PvP 8 分钟前的主动进攻下限从 20 降到 16。
- 保留 enemyNear/in-base 时的强制守家逻辑，避免敌人已进基地还出门。
- 目标是在 6 分钟左右以成型 3/4Gate 兵力先争夺位置，减少被 Locutus 完整一波压进主基地的概率。

## 测试结果

- `cmake --build build --target tests`：通过；仍有既有 `ninja: warning: premature end of file; recovering` 和 `/usr/local/Cellar/llvm/13.0.0_2/lib` 搜索路径 warning。
- `./tests --gtest_filter=Locutus.4GateGoon`：失败；地图 `Empire of the Sun`，seed `5798`，7:45 结束。
- 诊断：`army@5/6/7/8=8/12/0/-1`、`gates@5/6/7/8=2/2/2/-1`、`pylons@5/6/7/8=5/6/8/-1`、`supply@5/6/7/8=76/98 86/114 0/128 -1/-1`、最终 `gas=52`，结果 `SupplyBlocked`。
- 本场 6 分钟兵力只有 12，没有达到新的 16 兵主动前压阈值，因此未能有效验证 counterpush 行为。

## 评估

本改动把 PvP 从纯被动防守调整为 replay 更常见的早期前压。本场没有达到 16 兵阈值，未证明收益；后续应在 6 分钟 16 兵以上的样本中继续验证。
