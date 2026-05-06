# PvP Lock Third And Fourth Gateway Until Four Dragoons

## 问题背景

最新日志已经说明，当前问题不只是兵种比例，还有生产面扩张太快。3、4 门一旦在龙骑成型前铺开，就会继续吃矿，反过来把军队和经济都压薄。

## 败因分析（Replay + Log）

- Replay：PvP replay 里，第三、第四个 Gateway 不应该早于 4 条 Dragoon 成型。
- Log：`Fighting Spirit` seed `95274` 在 4:51 时已经有 `dragoons=2 cores=1`，但后面还是只剩 2 条龙骑和 4 条门，说明门数扩张仍然太早。
- Code：`gateways < 3`、`gateways < 4` 和 `Resp_IncreaseProd` 的分支都还允许在 `dragoons < 4` 时继续扩张。

## 改进方案

- 把第三、第四个 Gateway 的补建条件都锁到 `dragoons >= 4`。
- 这样在 PvP Core 成型前后，资源会先留给 Dragoon，而不是继续铺生产面。
- 目标是让军队先厚起来，再考虑门数扩张。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是一次对生产面扩张的收口。若它有效，日志里会更少看到“门数先涨、龙骑没跟上”的情况。
