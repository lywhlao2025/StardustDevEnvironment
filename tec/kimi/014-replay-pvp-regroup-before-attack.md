# Replay PvP Regroup Before Attack

## 问题背景

Post-core gas throttle 后，`Locutus.4GateGoon` 的存活时间延长到 7:56，但 7 分钟兵力只剩 3。Build order 已经能在 6 分钟形成 13 兵，主要问题转向交战前分散和逐个送兵。

## 败因分析（Replay + Log）

- Replay：多个 PvP replay 显示 2Gate/4Gate 开局并不是单兵持续离家，而是在 Pylon/Gateway 节奏稳定后集中一波战力再前压。
- Log：`Locutus.4GateGoon` 最新失败记录为 `army@5/6/7/8=8/13/3/-1`、`gates@5/6/7/8=2/3/3/-1`、7:56 `OutFought`。6 分钟有兵，7 分钟大幅掉兵，说明战斗前或初战时被分散消耗。
- 代码：虽然 PvP 8 分钟前把 `minAttackThreshold` 提高到 20，但空闲兵在 `shouldAttack == false` 时仍会默认 `attack(attackTarget)`，导致未达阈值的小队也会离家。

## 改进方案

- 增加 `pvpRegrouping = earlyPvP && !shouldAttack && frame < 8min`。
- 空闲 Zealot/Dragoon 在 regrouping 状态下攻击/集结到 `defendTarget`，不再默认向敌方基地移动。
- 保留已触发防守、敌人在家附近、或达到进攻阈值时的原有战斗逻辑。

## 测试结果

- `cmake --build build --target tests`：通过；仍有既有 `ninja: warning: premature end of file; recovering` 和 `/usr/local/Cellar/llvm/13.0.0_2/lib` 搜索路径 warning。
- `./tests --gtest_filter=Locutus.4GateGoon`：失败；地图 `Destination`，seed `20133`，7:30 结束。
- 诊断：`army@5/6/7/8=10/12/0/-1`、`gates@5/6/7/8=3/3/3/-1`、`workers@5/8/10/12=16/-1/-1/-1`、`supplyBlockedSec=52`、`idleFrames(Gateway/Core/Nexus)=7415/0/5367`、最终 `gas=648`、`firstScout tech=2.39792`。

## 评估

本改动修复策略阈值和空闲调度之间的不一致，目标是让 6 分钟形成的 2Gate/4Gate 兵力真正抱团防守，而不是分批离家被 Locutus 吃掉。测试仍失败，且在早期侦测 tech 的分支中最终 gas 升到 648，下一步应收紧 tech-threat 分支下的采气上限，避免因过量 gas 牺牲补兵矿物。
