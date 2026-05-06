# PvP Core First Dragoon Zero Reserve

## 问题背景

`Core` 已经能稳定落地，但最新日志里仍然经常出现 `dragoons@5/6/7/8=0/0/0/0` 或者只出一条龙骑就断掉。说明问题已经不是“能不能造 Core”，而是 `Core` 之后的矿物预留还在挡第一批龙骑。

## 败因分析（Replay + Log）

- Replay：PvP replay 里，`Core` 成立后应该尽快把 2 门以上的矿全部转成龙骑，不能继续用 50 矿的预留把 125 矿的 Dragoon 卡住。
- Log：`Tau Cross` seed `31982` 显示 `cores@5/6/7/8=0/1/1/0`、`dragoons@5/6/7/8=0/0/0/0`、`gates@5/6/7/8=2/2/2/0`，而最终 `minerals=132`。这正好落在“Dragoon 需要 125 + mineralReserve”的死区里。
- 代码：`mineralReserve` 在 `Core` 后仍被压到 50，导致很多时候矿量到了 132/160 也还不足以启动第一批 Dragoon。

## 改进方案

- 当 `earlyPvP && cores > 0 && gateways >= 3 && dragoons < 2` 时，把 `mineralReserve` 直接压到 0。
- 目标不是无限放开经济，而是先让前两条 Dragoon 真正落地，避免 Gateway 空转。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是一个很窄的生产侧调整，只影响 `Core` 后的前两条龙骑。若它生效，下一步再根据日志决定是继续压龙骑比例，还是回头调进攻阈值。
