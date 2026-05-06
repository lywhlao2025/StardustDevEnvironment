# PvP Raise Core Economic Cap To Twenty Two

## 问题背景

当前 PvP 仍然会在 7-8 分钟时把 worker 压到 16 左右，经济底盘偏薄，后续持续产兵和补给都不稳定。

## 败因分析（Replay + Log）

- Replay：PvP replay 里，Core 成立后如果 worker cap 仍然太低，后续就很难维持稳定兵力和补给。
- Log：`Destination` seed `63295` 最终只有 `16` 个 worker，`minerals=9 gas=50`，说明经济底盘还是太薄。
- Code：`completedCores > 0 || dragoons > 0` 时的 `pvpProbeCap` 只有 20，仍然偏保守。

## 改进方案

- 把 Core 后的 `pvpProbeCap` 再提到 22。
- 目标是让经济更早恢复，让后续兵力和补给都能继续跟上。
- 这一步不改变前面 zealot / dragoon 的分流，只是把经济上限再往前推。

## 测试结果

- 通过：`cmake --build build --target tests`
- 失败：`./tests --gtest_filter=Locutus.4GateGoon`
- 地图：`(2)Heartbreak Ridge.scx`
- seed：`30842`
- 关键日志：`workers@5/8/10/12: 16/0/-1/-1`
- 关键日志：`dragoons@5/6/7/8: 0/4/1/0`
- 关键日志：`pvpWindowFrames corePending/firstDragoons/reserve=580/2903/4533`
- 结果：`Loss reason=LowEcon`

## 评估

单纯提高 `pvpProbeCap` 不足以恢复经济，因为 `reserveForFirstDragoons && probeCount >= 16` 在目标龙骑未满 6 前持续暂停补 Probe。下一步应缩短这个 hold 窗口，让两龙骑后恢复补工人。
