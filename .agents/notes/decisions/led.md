# 板载 LED（GPIO2）指示灯

模块：`src/hal/led.{h,cpp}`，`led::init()` / `led::update()`，LEDC PWM 只用于两档亮度（高/低），不做呼吸。

## 灯效

间隔一段时间，闪三拍。三拍固定对应：**WiFi → 服务器 → 485**。

- 这一项已连通：这一拍高亮一下
- 这一项没通：这一拍低亮一下（灯不灭，上电时也能看出在工作）
- 拍与拍之间、三拍后的停顿：全灭

| 状态 | 灯效（● 高亮，○ 低亮） |
|---|---|
| 全部正常 | ● ● ● |
| 只有 WiFi 通 | ● ○ ○ |
| WiFi、服务器通，485 没通 | ● ● ○ |
| WiFi、485 通，服务器没通 | ● ○ ● |

## 时序

- 每拍 300ms：点亮 150ms，熄灭 150ms
- 三拍后停顿 1000ms，整个周期 1900ms

## 状态来源

`led::update()` 自己读：

- WiFi：`WiFi.status() == WL_CONNECTED`
- 服务器：`wsclient::isServerConnected`
- 485：`ztw_ddsu666::online`

## 约束

- 不做上电动画、升级动画等未讨论的状态
- `main.cpp` 不再直接操作 `PIN_LED`
- `setup()` 中等待 WiFi 的循环也要调用 `led::update()`
