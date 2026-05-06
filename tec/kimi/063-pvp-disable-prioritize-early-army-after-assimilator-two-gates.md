# PvP Disable Prioritize Early Army After Assimilator Two Gates

## 问题背景

最新日志已经说明，前期 zealot 之所以会堆到 12-14 条，不是后面的 Dragoon 分支，而是更早的 `prioritizeEarlyArmy` 还在持续补 zealot。只靠后面的硬停不够。

## 败因分析（Replay + Log）

- Replay：PvP replay 里，Assimilator 完成并且两门齐备后，前置 zealot flood 就应该停掉，否则矿会一直被吃空。
- Log：`Jade` seed `61762` 的诊断显示在 5:35 时 `minerals=38 gas=160`、`zealots=14`、`dragoons=0`。这表明早期 zealot 优先分支仍然在持续吞矿。
- Code：`prioritizeEarlyArmy` 只看 `gateways >= 1 && zealots < targetEarlyZealots`，没有在 Assimilator 完成 + 两门齐备时关掉。

## 改进方案

- 在 `Assimilator >= 1 && gateways >= 2` 的情况下，直接关闭 `prioritizeEarlyArmy`。
- 这样前置 zealot flood 会在气矿上线后停止，资源可以转给 Core 和 Dragon。
- 目标是从更早的层面切断 zealot 继续吃矿的入口。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是比后置硬停更前的一道闸。若它有效，`zealots` 不应再在 5 分钟后继续攀升到 12+，第一批 Dragoon 的矿物也会更稳定。
