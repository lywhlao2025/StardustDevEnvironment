# PvP Core Priority Over Gateway Catchup

## 问题背景

最新一轮 PvP 里，`Core` 还是没真正落地，`dragoons@5/6/7/8=0/0/0/0`，最后死在 7:36 的 `OutFought`。这次不是完全没资源，而是 `Core` 前面的 Gateway 追补把本该留给科技的矿花掉了。

## 败因分析（Replay + Log）

- Replay：PvP replay 里，2 Gateway + Assimilator 后应该优先把 150 矿留给 `Cybernetics Core`，而不是继续补第三个 Gateway。
- Log：`Neo Moon Glaive` seed `81398` 显示 `army@5/6/7/8=9/12/0/-1`、`gates@5/6/7/8=2/3/3/-1`、`pylons@5/6/7/8=5/8/8/-1`、`cores@5/6/7/8=0/0/0/-1`，同时 `coreBuild attempts/frame=1/4.40208`。说明 Core 只在资源已经被前置生产吃紧时尝试了一次，之后就再也没机会。
- 代码：`pvpGatewayCatchup`、`gateways < 3` 以及 `gateways < 6` 的补门逻辑没有识别 `Core pending`，会继续挤占这 150 矿。

## 改进方案

- 引入统一的 `pvpCorePending` 标志，表示“已满足 Core 前置条件，但 Core 还没落地”。
- 在 `pvpCorePending` 期间，关闭 `pvpGatewayCatchup`、敌方压力下的额外 Gateway、以及 800 矿爆门逻辑。
- 同时把训练侧的矿物预留继续维持在 150，保证资源不会再被拖去别的地方。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是一次很窄的优先级修正，目标只是不让 `Core` 再被 Gateway 追补挤掉。若它生效，下一步再看 Core 落地后第一批 Dragoon 的产出和出兵节奏。
