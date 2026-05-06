# PvP Core Hard Stop Unconditional

## 问题背景

把 `mineralReserve` 和 300 矿阈值都试过后，Core 仍然是 0，说明关键不在“预留多少”，而在于 Gateway 训练本身必须在 Core 未落地时直接停掉。

## 败因分析（Replay + Log）

- Replay：PvP 的中期链路必须先落 Core，再继续出 Dragoon。继续训练 Zealot 会让科技永远被挤掉。
- Log：`Heartbreak Ridge` seed `44568` 为 `cores@5/6/7/8=0/0/0/-1`、`zealots@5/6/7/8=7/9/0/-1`、`dragoons@5/6/7/8=0/0/0/-1`。这说明只要 Gateway 继续工作，Core 就会一直被拖死。
- 代码：之前的 hard stop 仍保留了矿物判断，结果不够硬；需要在 `pvpCorePending` 时无条件停止 Gateway 训练。

## 改进方案

- `pvpCorePending` 直接触发 Gateway 训练循环 `break`，不再看矿物。
- 让 Core 的 build 代码拥有先手，先把科技链打通，再恢复兵力生产。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

本改动是把 Core 的优先级提到绝对最高，代价是短时间内暂停 Zealot 生产。若 Core 仍然不落，问题就更可能在 Core 的建造位置或建造命令本身，而不是矿物分配。
