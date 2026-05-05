# 003 - PvP Midgame 诊断增强方案

## 问题背景

2026-05-05 `Locutus.RunTwenty` 随机抽样 20 局结果为 0 胜 20 负。失败共性：
- 7:20~10:38 之间被推平
- 终局大多为 `supply=0/0`
- `supplyBlockedSec` 常见为 41~74 秒
- `idleFrames(Gateway/Core/Nexus)` 偏高

现有诊断只记录 `workers@5/8/10/12`，无法判断 5~8 分钟的战斗单位数量是否已经落后 Locutus。

## 败因分析（Replay + Log）

| 时间点 | 已知信息 |
|--------|----------|
| 5:00   | Locutus 常见 4~5 个战斗单位开始前压 |
| 5:33   | Locutus 常见 9~10 个战斗单位 |
| 6:56   | Locutus 常见 19~21 个战斗单位 |
| 7:20+  | 我方多局被正面推平 |

核心问题：
1. **缺少我方 army checkpoint**：无法量化是生产落后还是交战亏损。
2. **缺少 Gateway checkpoint**：无法判断 Gateway 数量是否跟得上 Locutus 的 4Gate 节奏。
3. **后续调参证据不足**：继续改训练/供给/进攻阈值前，需要更细的时间点数据。

## 改进方案

### 1. 记录战斗单位与 Gateway 数

新增诊断字段：

```cpp
DIAG army@5/6/7/8: z gates@5/6/7/8: g
```

统计内容：
1. `army` = completed Zealot + completed Dragoon
2. `gates` = completed Gateway
3. 时间点 = 5、6、7、8 分钟

## 测试结果

| 测试 | 命令 | 结果 |
|------|------|------|
| 构建 | `cmake --build build --target tests` | PASS |
| Locutus 4GateGoon | `./tests --gtest_filter=Locutus.4GateGoon` | FAIL，La Mancha seed 43506，7:38 Loss，`army@5/6/7/8=7/7/0/-1`，`gates@5/6/7/8=2/2/3/-1` |

## 评估

本次是诊断增强，不声称已解决 Locutus 首胜。实测显示 6 分钟仍只有 2 个完成 Gateway，且 7 分钟前主力被打光。下一步优先优化 PvP rush 下第 3/4 Gateway 的建造时机，再观察 `army@6/7` 是否提升。
