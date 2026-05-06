# PvP Raise Core Economic Cap To Twenty

## 问题背景

最新验证已经显示，当前失败不只是战斗决策问题，经济也明显偏薄。最终只有 16 个 worker，矿也经常掉到很低，这让 Core 后的兵力持续补不起来。

## 败因分析（Replay + Log）

- Replay：PvP replay 里，Core 成立后如果经济继续锁在过低的 probe cap，后面即使有兵也会因为资源不足而打不起来。
- Log：`Python` seed `79445` 的最后结果里只有 `16` 个 worker，`minerals=15`、`gas=50`，说明经济面太薄。
- Code：`earlyPvP` 下只把 `pvpProbeCap` 提到 18，仍然偏保守，限制了后续矿物增长。

## 改进方案

- 在 `completedCores > 0 || dragoons > 0` 时，把 `pvpProbeCap` 提到 20。
- 这样 Core 成立后可以多补两个 probe，增强矿物流量。
- 目标是让兵力不再因为经济太薄而停在 2 Dragoon 左右。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是一次经济侧的上调，适合当前这种“兵能出来但不够持续”的情况。若它有效，后面会更容易把 Dragoon 数量和总军力都抬上去。
