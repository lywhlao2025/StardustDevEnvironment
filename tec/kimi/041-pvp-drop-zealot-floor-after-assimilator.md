# PvP Drop Zealot Floor After Assimilator

## 问题背景

最新日志里 `zealots@5/6/7/8` 经常堆到 13 左右，而 `dragoons` 还是 0-1。说明在 Core 成立之前，Gateway 仍然过度倾向 Zealot，矿物没有及时转给科技链。

## 败因分析（Replay + Log）

- Replay：PvP replay 里，Assimilator 一旦上线，前期 zealot floor 就不该继续强撑到 Core 才结束。
- Log：`Benzene` seed `74116` 显示 `army@5/6/7/8=9/13/0/-1`、`zealots@5/6/7/8=9/13/0/-1`、`dragoons@5/6/7/8=0/0/0/-1`，同时 `gas=464`。这说明矿被前期地面兵吃掉后，科技转型太晚。
- 代码：`zealotFloorNeeded` 只在 `cores == 0` 下判断，没有在 Assimilator 已经完成时提前退出。

## 改进方案

- 把 `zealotFloorNeeded` 收紧为：只有在 `earlyPvP && cores == 0 && Assimilator 还没完成` 时才成立。
- 一旦气矿上线，就允许 Gateway 更早进入转科流程，而不是继续压 zealot floor。
- 目标是给 Core 和第一批 Dragoon 腾出矿，而不是让前期狂热者把资源吃完。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是一次更前置的节奏切换修正，重点是让气矿上线后不再继续堆 zealot。若它有效，后面再根据日志决定是否还要继续压主动进攻阈值。
