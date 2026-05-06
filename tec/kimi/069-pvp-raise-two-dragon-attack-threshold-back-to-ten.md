# PvP Raise Two Dragon Attack Threshold Back To Ten

## 问题背景

最新一轮验证里，pylon 侧已经有了更完整的诊断，说明 supply 并不是当前最主要的短板。相反，局面输在了 `OutFought`，意味着 2 条 Dragoon 的主动门槛可能太低，军队太早出门了。

## 败因分析（Replay + Log）

- Replay：PvP replay 里，2 条 Dragoon 的窗口应该足够施压，但还不一定适合把军队完全送出家门。
- Log：`Empire of the Sun` seed `26517` 显示 4:42 的 `pylonWindow` 已经很健康，`threshold=72 cap=7 pylonInProgress=1`，说明供给逻辑并不是主要卡点；但最终结果还是 `OutFought`。
- Code：`minAttackThreshold` 在 `dragoons >= 2` 时被压到 8，可能让军队过早离家，进而在接战中被打崩。

## 改进方案

- 把 `earlyPvP && cores > 0 && dragoons >= 2` 的主动门槛收回到 10。
- 保留更大兵力时的原有更积极规则。
- 目标是让 2 Dragoon 窗口先稳住阵型，再开始对外施压。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是一次战斗侧的保守修正，针对 `OutFought` 而不是 `SupplyBlocked`。若它有效，军队应该能活得更久，之后再看是否仍需要继续调供给和经济。
