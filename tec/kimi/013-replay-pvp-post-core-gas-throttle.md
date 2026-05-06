# Replay PvP Post-Core Gas Throttle

## 问题背景

两轮 replay 驱动调整后，PvP 早期 Gateway 和兵力曲线已经改善，但 `Locutus.4GateGoon` 仍在 7 分半前后被打穿。最新诊断显示 4Gate 已经成型，6 分钟兵力达到 13，但最终 gas 浮到 424，说明矿物转兵不足。

## 败因分析（Replay + Log）

- Replay：`TL2104_Dewalt_Shuttle_Textbook_PvP_Fighting_Spirit_zimp_Aug_30_21_804.rep` 的前期节奏是 2Gate、Gas、Core 后继续补 Pylon 和 Gateway。这个节奏需要在首批 Dragoon 后继续用矿物支撑 Zealot/Gateway 产能，而不是持续高强度采气。
- Log：`Locutus.4GateGoon` 在 two-gate pylon buffer 后仍失败，记录为 `army@5/6/7/8=9/13/0/-1`、`gates@5/6/7/8=3/4/4/-1`、`supplyBlockedSec=57`、`idleFrames(Gateway/Core/Nexus)=9418/0/6152`、最终 `gas=424`。
- 当前 PvP 采气逻辑在 Core 完成后默认保留 2 个气工，只有矿物低于 200 或气体大幅超过矿物时才降档。对 4Gate 防守战来说，这会过早牺牲矿物收入。

## 改进方案

- PvP 中 Core 完成且首批 Dragoon 达标后，8 分钟前将目标气工降到 1。
- 当已有至少 2 个 Dragoon、gas 达到 150 且矿物低于 300 时，将目标气工进一步降到 0，把工人拉回矿物。
- 保留敌方 Robotics/drop 威胁时至少 2 个气工的保护分支，避免对 Shuttle/科技威胁缺 Dragoon。

## 测试结果

- `cmake --build build --target tests`：通过；仍有既有 `ninja: warning: premature end of file; recovering` 和 `/usr/local/Cellar/llvm/13.0.0_2/lib` 搜索路径 warning。
- `./tests --gtest_filter=Locutus.4GateGoon`：失败；地图 `Andromeda`，seed `54389`，7:56 结束。
- 诊断：`army@5/6/7/8=8/13/3/-1`、`gates@5/6/7/8=2/3/3/-1`、`workers@5/8/10/12=17/-1/-1/-1`、`supplyBlockedSec=52`、`idleFrames(Gateway/Core/Nexus)=7392/0/5926`、最终 `gas=296`。

## 评估

本改动针对最新日志里的 gas 浮余，不改变 build order 主线。结果显示最终 gas 从 424 降到 296，存活时间从 7:25 延长到 7:56，supply block 从 57 秒降到 52 秒，方向有效；但 7 分钟兵力只剩 3，下一步应处理交战损耗和防守集结。
