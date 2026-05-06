# PvP Core Dragoon Heavy Ratio

## 问题背景

现在 `Core` 已经能落地，龙骑也能到 2-6 条，但战报仍然经常以 `OutFought` 结束。说明不是“没有龙骑”，而是龙骑比例还不够重，军队结构太偏 Zealot。

## 败因分析（Replay + Log）

- Replay：PvP replay 中，`Core` 后的中期军队需要逐步转成更重的 Dragoon 比例，不能一直维持 Zealot 主体。
- Log：`La Mancha` seed `35419` 显示 `army@5/6/7/8=9/13/16/20`、`zealots@5/6/7/8=9/13/14/14`、`dragoons@5/6/7/8=0/0/2/6`。说明龙骑虽然终于起来了，但军队仍然偏近战。
- 代码：当前 `dragoons < (zealots * 2 + 2)` 的比例上限仍然偏松，允许 Zealot 长时间压过 Dragoon。

## 改进方案

- 在 `earlyPvP && cores > 0` 时，把龙骑比例上限收紧到 `dragoons < zealots + 1`。
- 其他对局仍保留更宽松的比例，避免把非 PvP 开局打坏。
- 目标是让 Core 后的中期军队更快变成龙骑主导，而不是继续堆狂热者。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是兵种结构的微调，不是大改。若这一步能把 4-6 分钟的龙骑密度再抬高，下一步就看是否还能继续把进攻时机提前一点。
