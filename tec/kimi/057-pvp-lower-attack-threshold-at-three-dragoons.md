# PvP Lower Attack Threshold At Three Dragoons

## 问题背景

现在 PvP 已经能稳定做出 3 条 Dragoon，但战报还是输在 supply block。说明这批兵还没有被及时转成主动压力，拖在家里只会让补给和经济继续恶化。

## 败因分析（Replay + Log）

- Replay：PvP replay 里，3 条 Dragoon 已经是可以开始施压的中期窗口，不该继续等到更大兵力。
- Log：`Roadrunner` seed `1655` 显示 `dragoons@5/6/7/8=0/3/1/0`、`result=SupplyBlocked`、`supplyBlockedSec=65`。这说明兵已经成型，但没有及时兑现成进攻。
- 代码：`minAttackThreshold` 只在 `dragoons >= 1` 时下调到 12，还不够覆盖 3 条 Dragoon 的稳定施压窗口。

## 改进方案

- 当 `earlyPvP && cores > 0 && dragoons >= 3 && frame < 9min` 时，把 `minAttackThreshold` 压到 12。
- 保留更早阶段的原始门槛，不把 1 条 Dragoon 的窗口过度激进化。
- 目标是让 3 条 Dragoon 尽快出门，避免家里继续被 supply block 和局部消耗拖死。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是在兵力成型后的主动性修正。若它有效，后续日志里应该更早看到军队离家并把 supply block 的影响转移到对手身上。
