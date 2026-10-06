# AGENTS.md — 下位机项目定义

> AGENTS.md 只定义项目当下状态，即 **做什么**，不放细节实现和过程性研究
> 下位机的细节实现和过程性研究，下位机规划参考当前存储库的 `./.agents/notes/AGENTS.md`

> 本存储库：下位机
> 上位机、服务端请见 `~/Documents/dev/Projects/Playground/ElectricMeter`
> 整体规划请见 `~/Documents/dev/Projects/Playground/ElectricMeter/.agents/notes/AGENTS.md`

> “老项目”请见 `~/Documents/dev/Projects/PlatformIOProjects/ElectricDriveClient`

## 开发原则

- 我们要的是精简，易读可维护原型，不是工业化项目，我们要的是“它能跑起来演示”，而不是“能落地”
  - 想象一个初级程序员， **不看注释** 能不能轻松读懂代码。将这个设为标准
  - 不要堆砌文档和注释！让代码自己说话
  - 不要造名词——Entities 和 Devices 就够了，observers、conns、live 这种稀奇古怪的名词只会让人读不懂
- **已规划** 的东西是长期目标，“已规划”不是 TODO list。用户没有敲定就不要主动去实现规划中的东西
- 涉及到的全部时间，都直接使用 number 秒级时间戳，不使用任何特定时间格式
- 注释用中文，只补充有助于理解的内容

## 项目是什么

这是 ElectricMeter 用电管理系统的 ESP32 下位机工程。系统目标是一台 ESP32 管一间房，采集电能数据、控制回路，并与服务器通信。

## 已实现

> 这里只列项目结构，具体有什么直接读源码。 **不要赘述，不要累赘**

> 结构实现细节，重点学习 **老项目** `~/Documents/dev/Projects/PlatformIOProjects/ElectricDriveClient`
> 各种代码都可以直接从老代码copy过来

- `platformio.ini`
- `src/wsclient.cpp`
- `src/hal/`

# 已规划

> 实现后挪到上方，然后从这里删除，不留

- 联网
- 基于旧 wsclient 的通信跑通
- pub_entities 跑通
- action 动作跑通
- 真实 RS485 电能表数据读取
- 真实继电器驱动

## 构建与烧录

```bash
pio run
pio run -e esp32dev_1000000
pio run -t upload # 默认参数即可，相信PIO自动选择。加额外参数反而可能导致烧录不好
pio device monitor --port /dev/ttyUSB0
```

默认构建环境是 `esp32dev_2000000`，串口监视器波特率为 `115200`。烧录前确认所选串口和实际开发板。
