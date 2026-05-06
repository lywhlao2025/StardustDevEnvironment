# PvP Disable Global Zealot Filler After Two Gates

## 问题背景

最新诊断说明，除了前置 zealot priority 之外，还有一个更全局的 zealot filler 在 PvP 两门阶段持续补兵。它会把本该留给 Core 和 Dragoon 的矿继续吃掉。

## 败因分析（Replay + Log）

- Replay：PvP replay 里，2 Gate 之后如果还继续用全局 zealot filler，就会把资源继续压回 zealot flood。
- Log：`Jade` seed `61762` 在 5:35 时 `minerals=38 gas=160`、`zealots=14`、`dragoons=0`，说明有一条更广的 zealot 入口仍在持续工作。
- Code：`if (!reserveForFirstDragoons && !pvpPreferRangedMidgame && zealots < 48 && supplyRemaining >= 4 && minerals >= 100)` 这条全局 filler 在 PvP 两门阶段没有被关掉。

## 改进方案

- 在 `earlyPvP && gateways >= 2` 时，直接关闭这条全局 zealot filler。
- 这样两门之后资源不会继续被大范围 zealot 补兵吞掉。
- 目标是把矿真正让给 Core、Dragoon 和后续补给，而不是让全局 filler 继续吃空。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是对最广的 zealot 兜底入口的收口。若它有效，5 分钟后的 zealot 数量应该不会再轻易冲到 12+，同时矿物应该更稳定地流向科技和龙骑。
