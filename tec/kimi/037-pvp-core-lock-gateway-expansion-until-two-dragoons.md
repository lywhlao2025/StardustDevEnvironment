# PvP Core Lock Gateway Expansion Until Two Dragoons

## 问题背景

`Core` 已经能落地，但日志里仍然出现 `cores=1` 之后只有 `dragoons=1`，同时 `gateways` 却继续长到 4。说明科技链已经通了，资源却被继续扩门吃掉，第一批龙骑没有被优先完成。

## 败因分析（Replay + Log）

- Replay：PvP replay 中，`Core` 成立后，至少要先把前两条 Dragoon 吐出来，再考虑继续扩 Gateway。
- Log：`Circuit Breaker` seed `2056` 显示 `army@5/6/7/8=7/5/0/-1`、`dragoons@5/6/7/8=1/0/0/-1`、`gates@5/6/7/8=3/4/4/-1`。这说明 Gateway 扩张比龙骑转型更快。
- 代码：`pvpGatewayCatchup` 和后面的 Gateway 补门逻辑都没有识别 “Core 已有但前两条 Dragoon 还没完成” 这个窗口。

## 改进方案

- 引入 `pvpNeedFirstDragoons`，表示 `earlyPvP && cores > 0 && dragoons < 2`。
- 在这个窗口里，禁止 `pvpGatewayCatchup`、敌方压力下的额外 Gateway、300 矿扩门和 800 矿爆门。
- 目标是先把第一批 Dragon 升起来，再恢复 Gateway 扩张。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是一次明确的资源优先级重排，重点只在 `Core` 后的前两条 Dragoon。若它有效，下一步再看是否需要给 3 门后的兵种比例再做一次微调。
