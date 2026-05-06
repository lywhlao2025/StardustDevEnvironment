# Replay PvP Gateway Catchup Fallback

## 问题背景

多场失败显示 5/6 分钟只有 2-3 Gateway，即使供给不满也无法稳定达到 replay 中 3/4 Gateway 的产能。前面禁用 proxy 时已经暴露过 Gateway 建造点选择可能失败，需要把 fallback 扩展到后续 catchup 分支。

## 败因分析（Replay + Log）

- Replay：PvP replay 中 3/4 Gateway 常在 4-5 分钟成型，用来支撑 6-7 分钟的主要交战。
- Log：`Empire of the Sun` seed `5798` 为 `gates@5/6/7=2/2/2`，`Icarus` seed `87695` 为 `gates@5/6/7=3/3/3`，但供给并非主要瓶颈。
- 代码：早期第 1/2 Gateway 已加入 `myStart` fallback，但 `pvpGatewayCatchup`、enemy rush 补 Gateway、第三 Gateway 通用分支仍只在 `gatewayBuildNear` 尝试一次。

## 改进方案

- 在早期训练前增加 PvP Gateway catchup：4 分钟后、至少 2 Pylon、16 Probe、Gateway 少于 4 时优先尝试补 Gateway。
- 为训练前 catchup 增加 `myStart` fallback。
- 为原有 `pvpGatewayCatchup` 增加 `myStart` fallback。
- 为 enemy rush 补 Gateway 分支增加 `myStart` fallback。
- 为第三 Gateway 通用分支增加 `myStart` fallback。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

本改动把 PvP 多 Gateway 的矿物优先级前移到早期训练之前，并提高建造点失败时的恢复能力。预期是减少 5/6 分钟仍停在 2-3 Gateway 的地图样本。

## 最新结果

- `cmake --build build --target tests`：通过；仍有既有 `ninja: warning: premature end of file; recovering` 和 `/usr/local/Cellar/llvm/13.0.0_2/lib` 搜索路径 warning。
- `./tests --gtest_filter=Locutus.4GateGoon`：失败；地图 `Benzene`，seed `41348`，9:34 结束。
- 诊断：`workers@5/8/10/12=16/19/-1/-1`、`army@5/6/7/8=9/16/20/21`、`gates@5/6/7/8=3/3/3/3`、`pylons@5/6/7/8=6/6/9/9`、`supply@5/6/7/8=80/114 108/114 124/162 126/162`、`supplyBlockedSec=50`、`idleFrames(Gateway/Core/Nexus)=8564/0/7326`、最终 `gas=720`、结果 `OutFought`。
- 这次验证说明 Gateway catchup 已经把前中期产能拉起来，下一步不是继续补 Gate，而是让 14 兵左右就能主动打出优势，而不是攒到 20+ 才交战。
