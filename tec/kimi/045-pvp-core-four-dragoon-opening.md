# PvP Core Four Dragoon Opening

## 问题背景

当前 PvP 已经能稳定落到 2 条 Dragoon，但这还不足以把中期战斗转成优势。日志里仍然常见 `dragoons=2` 就停住，之后被对面反推。

## 败因分析（Replay + Log）

- Replay：PvP replay 里，`Core` 之后如果只做 2 条 Dragoon，军队仍偏轻，容易被后续一波打穿。
- Log：`Tau Cross` seed `681` 显示 `army@5/6/7/8=7/8/0/-1`、`dragoons@5/6/7/8=0/2/0/-1`、`zealots@5/6/7/8=7/6/0/-1`。这说明 2 条龙骑只是起步，不是足够的开门兵力。
- 代码：`targetOpeningDragoons` 在非强威胁 PvP 里仍然只要求 2，导致第一阶段的龙骑生产过早收口。

## 改进方案

- 当 `earlyPvP && cores > 0` 时，把 `targetOpeningDragoons` 提升到至少 4。
- 这样 `reserveForFirstDragoons`、训练优先级和矿物保留都会继续围绕更大的龙骑开门目标运转。
- 目标是让 PvP 的第一批可战斗兵力不止停在 2 条龙骑，而是能形成更稳的 4 条开门窗口。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这不是终局策略，而是把转型门槛从“够用”提高到“能打”的修正。若它有效，后面就能直接看 4 条龙骑的进攻窗口是否足以终结对局。
