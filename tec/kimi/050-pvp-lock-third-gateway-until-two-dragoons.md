# PvP Lock Third Gateway Until Two Dragoons

## 问题背景

最新窗口诊断里已经看见，4 分多钟时 `Gateway` 数量先到 3，但第一条 Dragoon 还没起，矿也只剩 26。说明第三个 Gateway 在抢资源，而不是在服务转型。

## 败因分析（Replay + Log）

- Replay：PvP replay 中，2 Gate -> Core -> Dragoon 的节奏里，第三个 Gateway 不能早于第一批 Dragoon 出现。
- Log：`Destination` seed `64543` 显示在 4:46 时 `minerals=26 gas=248 supplyRemaining=30 availableArmySupply=15 gateways=3 zealots=8 dragoons=0`。这说明门数扩张提前消耗了本该留给 Dragoon 的矿。
- 代码：`pvpGatewayCatchup` 和大矿量扩门条件都允许在 `dragoons == 0/1` 时继续补第三个 Gateway。

## 改进方案

- 给 PvP 的第三个 Gateway 设置硬门槛：至少已有 2 条 Dragoon 才能触发。
- 同样把 800 矿的大扩门逻辑锁到 `dragoons >= 2` 之后。
- 目标是先把矿转成可战斗的 Dragoon，再考虑继续增加生产面。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是对 PvP 经济分流的直接纠偏。若它有效，下一轮日志里 Gateway 数量应该不再提前冲到 3，而 Dragoon 应该能更早稳定成型。
