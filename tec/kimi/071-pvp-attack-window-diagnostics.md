# PvP Attack Window Diagnostics

## 问题背景

当前 PvP 已经不再是单纯的供给问题，更多时候输在 `OutFought`。需要知道军队第一次进入可进攻状态时，实际的 army / threshold / pressure 是什么。

## 败因分析（Replay + Log）

- Replay：PvP replay 里，2 条或 3 条 Dragoon 的窗口可能已经触发进攻，但我们还不确定它是不是太早了。
- Log：目前没有专门的进攻窗口日志，只能从最后的失败结果反推。
- Code：`canProactiveAttack` 的触发点需要一次性打印出来，才能判断是否需要回调阈值。

## 改进方案

- 增加 `pvpAttackDiagPrinted`。
- 在 `earlyPvP && cores > 0 && armyCount >= minAttackThreshold` 的第一次，打印 army、threshold、zealot、dragoon、pressure 和 frame。
- 目标是确定进攻窗口是否太早，或者还不够早。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

这是最后一层决策窗口的可观测性增强。若它显示兵力刚到门槛就离家，但随后很快被打崩，就说明阈值需要回调；若它显示根本没到门槛，那就继续调整军力成型侧。
