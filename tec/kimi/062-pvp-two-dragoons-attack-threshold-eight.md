# PvP Two Dragoons Attack Threshold Eight

## 问题背景

现在 PvP 已经能更早做出 2 条 Dragoon，但依然输在 `OutFought`。说明这批兵虽然成型更早了，但还没有被及时拿去施压。

## 败因分析（Replay + Log）

- Replay：PvP replay 里，2 Dragoon + 少量 Zealot 的组合已经足够开始主动推进，不该继续把兵留在家里。
- Log：`Destination` seed `854` 显示最终 `dragoons@5/6/7/8=2/0/0/0`、`zealots@5/6/7/8=5/3/0/0`，而结果仍然是 `OutFought`。这说明兵力到位后没有及时转化成地图压力。
- 代码：`minAttackThreshold` 在 `dragoons >= 2` 的窗口里还没有足够低，导致军队继续滞留。

## 改进方案

- 当 `earlyPvP && cores > 0 && dragoons >= 2 && frame < 9min` 时，把主动进攻阈值压到 8。
- 保留更大威胁或更大兵力时的原有规则。
- 目标是让 2 Dragoon 的窗口尽快离家，把局面推进到对手承压。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是对中期主动性的进一步修正。若它有效，后续日志里应该更早看到军队离家，`OutFought` 的概率也会下降。
