# PvP Raise Core Opening Dragoons To Six

## 问题背景

当前 PvP 的兵线已经不再是完全没龙骑，而是 Core 后常常只做出 2 条左右 Dragoon 就停住。这个兵力重量明显偏轻，容易在中期接战中输掉。

## 败因分析（Replay + Log）

- Replay：PvP replay 里，Core 成立后如果只停在 2-4 条 Dragoon，整体军队还是偏近战，火力不够重。
- Log：`Fighting Spirit` seed `67088` 的验证里，`pylonWindow` 已经显示 `dragoons=2 cores=1`，但最终还是 `OutFought`。这说明 2 Dragoon 的开门目标太轻。
- Code：`targetOpeningDragoons` 在 `cores > 0` 时只被抬到 4，仍然不足以让中期军队稳定成型。

## 改进方案

- 把 `earlyPvP && cores > 0` 时的 `targetOpeningDragoons` 提高到至少 6。
- 同步保持前期 zealot 收口和供给前置，避免开门龙骑的资源被别的入口吃掉。
- 目标是让 Core 之后的窗口真正转成更重的 Dragoon 核心，而不是停在半成品状态。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是兵力结构层面的加强。若它有效，Core 后的军队会更接近“龙骑主导”，接战时的持续输出应该更稳定。
