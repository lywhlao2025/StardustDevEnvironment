# PvP Cut Gas Workers Before Four Dragoons

## 问题背景

最新日志里，PvP 进入 4-5 分钟后矿物仍然偏低，而气体已经堆得很高。说明早期气矿工人过多，矿物采集速度不够，第一批 Dragoon 的 125 矿迟迟攒不出来。

## 败因分析（Replay + Log）

- Replay：PvP replay 中，Assimilator 开了以后不代表可以一直维持 2-3 个气矿工人。要先把第一批 Dragoon 的矿攒出来。
- Log：`Destination` seed `64543` 在 4:46 的窗口诊断里显示 `minerals=78 gas=304`、`zealots=9 dragoons=0`。这是一种典型的“气太多、矿太少”的失衡。
- 代码：`desiredGasWorkers` 在早期 PvP 中仍然把气矿工人维持得太高，未在 `dragoons < 4` 的窗口提前收缩。

## 改进方案

- 当 `earlyPvP && Assimilator 已完成 && dragoons < 4` 时，把 `desiredGasWorkers` 压到 1。
- 这样可以把更多 probes 留在矿上，尽快攒出第一批 Dragoon 的 125 矿。
- 目标是恢复矿物流量，而不是继续堆过量气体。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是把经济侧重新拉回矿物优先的修正。若它有效，后续窗口里 `minerals` 应该更早超过 Dragoon 门槛，`dragoons` 也会更稳定地往 2-4 条推进。
