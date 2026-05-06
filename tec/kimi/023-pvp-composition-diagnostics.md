# PvP Composition Diagnostics

## 问题背景

Gateway catchup 已经把前中期产能拉上来，但当前失败仍然落在 7-9 分钟的正面换兵。只看总 army 数和 Gateway 数，无法判断是 Zealot 不够还是 Dragoon 比例不对。

## 败因分析（Replay + Log）

- Replay：PvP replay 的关键不是单纯堆军，而是 2Gate/4Gate 阶段的前排/后排比例要匹配，通常会先有一层 Zealot 再补 Dragoon。
- Log：`Benzene` seed `41348` 为 `army@5/6/7/8=9/16/20/21`、`gates@5/6/7/8=3/3/3/3`、`supplyBlockedSec=50`，但最终仍 `OutFought`。只看总军力不足以定位比例问题。
- 当前日志缺少 `zealots@5/6/7/8` 和 `dragoons@5/6/7/8`，无法判断 6 分钟 16 军是偏 Zealot 还是偏 Dragoon，也无法验证 replay 里前排是否不足。

## 改进方案

- 增加 `zealotCountAt5/6/7/8` 和 `dragoonCountAt5/6/7/8`。
- 在 `onEnd` 输出 `zealots@5/6/7/8` 与 `dragoons@5/6/7/8`。
- 保持现有战术逻辑不变，仅增加观测维度。

## 测试结果

- 待运行：`cmake --build build --target tests`
- 待运行：`./tests --gtest_filter=Locutus.4GateGoon`

## 评估

本改动不直接提升胜率，但能把“总军力足够却被打穿”的原因拆成前排/后排结构问题，避免继续盲改攻击阈值或产能阈值。
