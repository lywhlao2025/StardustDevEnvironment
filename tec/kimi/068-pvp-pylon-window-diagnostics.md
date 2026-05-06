# PvP Pylon Window Diagnostics

## 问题背景

现在 PvP 已经有了 2-4 条 Dragoon，但 `SupplyBlocked` 仍然很重。我们还不确定 pylon 触发到底是没到阈值，还是触发了但被 `pylonInProgress` / 其它上限挡住。

## 败因分析（Replay + Log）

- Replay：PvP replay 里，成型期的 supply 问题要尽早定位，否则再调阈值容易继续盲改。
- Log：当前输出已经能看到最终 `supplyBlockedSec`，但看不到 pylon 触发时的 `minerals`、`projectedSupply`、`pylonInProgress`、`threshold` 和 `cap`。
- Code：需要在 `earlyPvP && dragoons >= 2` 时打印一次 pylon 窗口，确认触发条件的真实数值。

## 改进方案

- 增加 `pvpPylonDiagPrinted`。
- 在 `earlyPvP && dragoons >= 2` 的时点，打印矿量、气量、剩余人口、预测人口、pylon 进行中数量、阈值和 cap。
- 目标是确认 pylon 逻辑到底是没触发，还是触发太少。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是下一轮 supply 问题定位所需的观测点。若它显示阈值早就到了但 pylon 仍然少，就该直接改 build 约束；若阈值根本没到，就继续前推 supply 触发。
