# PvP Core Hard Prioritize First Two Dragoons

## 问题背景

即使 `Core` 已经落地，战报里仍然经常只有 0-1 条 Dragoon，Gateway 还在继续出 Zealot。说明“倾向龙骑”还不够，需要把前两条龙骑做成硬优先级。

## 败因分析（Replay + Log）

- Replay：PvP replay 中，`Core` 后的前两条 Dragoon 是关键转折点，不能让 Gateway 继续按前期 Zealot 节奏运转。
- Log：`Heartbreak Ridge` seed `84677` 显示 `zealots@5/6/7/8=8/11/2/0`、`dragoons@5/6/7/8=0/1/1/0`、`gates@5/6/7/8=2/2/3/4`。这说明 Gateway 的训练节奏仍然偏向 Zealot。
- 代码：仅靠 `wantGoon` 的软偏好不够，需要在训练循环里对 `earlyPvP && cores > 0 && dragoons < 2` 做硬优先级。

## 改进方案

- 在 Gateway 训练逻辑最前面，直接判断 `earlyPvP && cores > 0 && dragoons < 2`。
- 满足条件时强制训练 Dragoon，并 `continue`，绕过后面的 Zealot 分支。
- 目标是先把前两条 Dragoon 吐出来，再回到常规混编。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是一次更硬的兵种优先级修正，只影响 PvP 科技转型窗口。若它有效，下一步再看是否还需要微调进攻阈值。
