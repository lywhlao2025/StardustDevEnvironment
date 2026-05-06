# PvP Lower One Dragoon Attack Threshold To Ten

## 问题背景

现在 PvP 的问题已经不只是“出不出龙骑”，而是“兵力太晚出门”。日志里甚至出现了 8 分钟时 worker 只剩 1 的情况，说明家里被打穿，而军队还没有形成足够的地图压力。

## 败因分析（Replay + Log）

- Replay：PvP replay 里，1 条 Dragoon + 8 条左右 Zealot 的窗口已经可以开始施压，继续拖着只会让家里的经济先崩。
- Log：`Roadrunner` seed `1655` 显示 `workers@5/8/10/12=18/0/-1/-1`，说明到了 8 分钟时经济已经极不稳定；与此同时 `dragoons@5/6/7/8=0/3/1/0`、`result=SupplyBlocked`，表明兵力没有及时外出。
- 代码：`minAttackThreshold` 对 `dragoons >= 1` 的窗口仍然停在 12，太保守，没把这批兵转成压力。

## 改进方案

- 把 `earlyPvP && cores > 0 && dragoons >= 1 && frame < 8min` 的主动进攻阈值压到 10。
- 保留更大威胁或更高兵力时的原有规则，不影响别的对局。
- 目标是让 1 条 Dragoon 出现后，军队更早离家，把防守压力转到对手身上。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是一次主动性修正，针对“兵在家里憋死”的问题。若它有效，后续日志里应该更早看到军队离家，经济被拆的速度也会下降。
