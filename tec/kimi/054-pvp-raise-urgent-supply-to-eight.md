# PvP Raise Urgent Supply To Eight

## 问题背景

现在 PvP 已经能做出 2-4 条 Dragoon，但 supply block 仍然很重。说明补给触发点还是偏晚，等到只剩 4 格人口才补已经来不及。

## 败因分析（Replay + Log）

- Replay：PvP replay 里，成型期的 Dragoon + Zealot 组合会很快吃掉人口，补给必须更提前。
- Log：`Icarus` seed `51829` 显示 `result=SupplyBlocked`、`supplyBlockedSec=58`，而 7:38 时 `supply@5/6/7/8=64/114 52/146 2/64`，说明补给触发仍然太晚。
- 代码：`urgentSupply` 仍然只在 `supplyRemaining <= 4` 时触发，成型期不够激进。

## 改进方案

- 在 `earlyPvP` 下，把 `urgentSupply` 提前到 `supplyRemaining <= 8`。
- 这样在成型期会更早下 Pylon，减少长时间卡人口的风险。
- 目标是把补给触发点提前到龙骑成型后的实际需求范围内。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是补给侧的进一步前置，专门针对成型后的 supply block。若它有效，`supplyBlockedSec` 应该下降，且不会再出现 2 条龙骑后继续长时间卡人口。
