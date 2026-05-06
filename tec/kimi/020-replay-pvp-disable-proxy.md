# Replay PvP Disable Proxy

## 问题背景

Pylon/supply 诊断显示 5/6 分钟并没有真实卡供给，主要问题是战斗部队在 6-7 分钟被打光。继续检查 build order 后发现 PvP proxy 仍会在发现敌方起点后无条件启动，并且 gas/core 会等待 proxy Gateways。

## 败因分析（Replay + Log）

- Replay：用户提供的 PvP replay 主线是正常 2Gate/Gas/Core 和后续 3/4 Gateway，而不是每局前置 Pylon/Gateway。
- Log：`Neo Moon Glaive` seed `43722` 失败时，5/6 分钟供给为 `76/98`、`68/130`，说明不是 Pylon 不够；但 `army@5/6/7=8/7/0`，兵力在战斗前后断档。
- 代码：PvP proxy pylon 一旦敌方起点确定就启动；同时 `Assimilator` 与 `Cybernetics Core` 条件包含 `(!proxyMode || proxyGateways >= 2)`，会让 replay 主线的 Gas/Core 被 proxy 分支拖延或分散。

## 改进方案

- 对敌方 Protoss 禁用 proxy pylon。
- PvP 主基地 Gateway 建造增加 `myStart` fallback，避免只在第一个 Pylon 附近找点失败导致 0 Gateway。
- 保留非 PvP 的 proxy 逻辑，不扩大本次改动范围。
- PvP 回到 replay-backed 的主基地 2Gate/Gas/Core/3Gate/4Gate 产能线。

## 测试结果

- `cmake --build build --target tests`：通过；仍有既有 `ninja: warning: premature end of file; recovering` 和 `/usr/local/Cellar/llvm/13.0.0_2/lib` 搜索路径 warning。
- 中间版本只禁用 PvP proxy 时，`Tau Cross` seed `15599` 出现 0 Gateway 断链，`army@5/6=0/0`、最终 `minerals=2862`，说明需要主基地 Gateway fallback。
- 最终版本：`./tests --gtest_filter=Locutus.4GateGoon` 失败；地图 `Icarus`，seed `43994`，7:37 结束。
- 诊断：`army@5/6/7/8=9/14/0/-1`、`gates@5/6/7/8=3/3/3/-1`、`pylons@5/6/7/8=6/6/6/-1`、`supply@5/6/7/8=76/114 96/114 0/96 -1/-1`、最终 `gas=448`，结果 `OutFought`。

## 评估

本改动收回之前实测 0/20 的 PvP proxy 分支，减少 build order 分叉和兵力分散。最终版本确认主基地 Gateway 链路正常，6 分钟可到 14 兵；仍在 7 分钟前后被清空，下一步重点是交战质量和防守阵位。
