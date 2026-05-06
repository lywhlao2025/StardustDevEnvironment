# PvP Lock Fourth Gateway Until Two Dragoons

## 问题背景

当前失败日志已经显示，4 分钟后门数还是会继续冲到 4，但龙骑和经济没有同步跟上。门数先膨胀会吃掉矿，让后面的兵和补给一起出问题。

## 败因分析（Replay + Log）

- Replay：PvP replay 里，第三、第四个 Gateway 都不应该早于第一批 Dragoon 的稳定成型。
- Log：`Icarus` seed `9679` 显示 `gates@5/6/7/8=2/2/4/-1`、`zealots@5/6/7/8=6/3/0/-1`、`dragoons@5/6/7/8=1/0/0/-1`。这说明扩门继续早于龙骑成型。
- 代码：`gateways < 4` 和 `Resp_IncreaseProd` 的分支还允许在 `dragoons < 2` 时继续补门。

## 改进方案

- 把第三、第四个 Gateway 的补建条件都锁到 `dragoons >= 2` 之后。
- 这样在 PvP Core 成型前后，矿物会先流向 Dragoon，而不是继续堆生产面。
- 目标是把生产扩张和军力成型分开，先保证能打，再谈继续扩门。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是一次针对门数膨胀的约束修正。若它有效，后续日志里应该不再出现 4 Gate 早于 2 Dragoon 的情况。
