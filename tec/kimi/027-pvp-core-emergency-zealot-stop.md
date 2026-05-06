# PvP Core Emergency Zealot Stop

## 问题背景

前面的 Core 预留和建造点 fallback 都没有让 Core 落地。最新日志仍然显示 `assimilator=1`、`cores=0`、`zealots=12`，说明 Gateway 继续把矿物转成 Zealot，导致 Core 永远攒不出 150 矿。

## 败因分析（Replay + Log）

- Replay：PvP 的中期节奏需要在 2Gate 后迅速落 Core，然后开始转 Dragoon。继续堆 Zealot 会让 Tech 链路一直延迟。
- Log：`Andromeda` seed `71016` 为 `assimilators@5/6/7/8=1/1/1/-1`、`cores@5/6/7/8=0/0/0/-1`、`zealots@5/6/7/8=8/12/0/-1`、`dragoons@5/6/7/8=0/0/0/-1`。矿和气都在，但 Core 没落地。
- 代码：Gateway 循环里没有在 Core 未落地时停止继续训练 Zealot 的硬保护，所以矿物会持续被 Zealot 吞掉。

## 改进方案

- 当 `earlyPvP && cores == 0 && assimilator >= 1 && gateways >= 2 && zealots >= 8` 时，如果矿物低于 250，直接停止当前 Gateway 训练循环。
- 目的不是永久停兵，而是让 Core 有机会先落地，再恢复 Zealot/Dragoon 混编。
- 保持原有其他 build order 不动，只在 Core 缺失时做紧急刹车。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

本改动是对“有气却没 Core”的最后一层保险。若它仍不能让 Core 出现，就说明 Core 的前置建造条件还有更深层的漏项，需要继续收集日志。
