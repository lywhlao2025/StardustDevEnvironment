# PvP First Dragoon After Core

## 问题背景

Core 已经能落地，但最新日志里 `cores@5/6/7/8=1/1/1/1` 仍然没有任何 Dragoon，说明第一批龙骑被后续矿物预留挡住了。

## 败因分析（Replay + Log）

- Replay：PvP replay 中 Core 落地后需要尽快把 Gas 转成第一批 Dragoon，不能继续让矿物预留长期压制龙骑生产。
- Log：`Empire of the Sun` seed `95072` 显示 `cores@5/6/7/8=1/1/1/1`、`dragoons@5/6/7/8=0/0/0/0`，而 `gates@5/6/7/8=2/4/3/3`。科技链已经通了，但龙骑仍未出现。
- 代码：`mineralReserve` 在 `defensiveCannonMineralReserve` 之后仍可能保持过高，导致 `u->train(Protoss_Dragoon)` 进不了门槛。

## 改进方案

- 在 `defensiveCannonMineralReserve` 之后再次下调 `mineralReserve`。
- 条件为 `earlyPvP && cores > 0 && gateways >= 3 && dragoons == 0` 时，把矿物预留压到最多 50。
- 目标是保证第一批 Dragoon 不再被防守预留挡住。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

本改动只针对“Core 已成但 0 Dragoon”的链路断点。若它成功，下一步才有资格重新评估兵种比例和主动进攻阈值。
