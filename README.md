# 🛩️ Beat_Plane — 嵌入式横版飞行射击游戏

基于 **LVGL** 的嵌入式横版飞行射击游戏，支持 **MCU 实体硬件** 与 **PC 模拟器** 双平台运行，具备 UART 串口双机联机对战功能。

---

## ✨ 特性

- 🎮 **4 种战机**：Player（护盾+散射）、Ember（火墙+灼烧）、Stream（冰冻+减速）、Verdant（加速+治疗），各具双技能
- 👾 **5 关关卡**：波次递进 + Boss 战，难度逐级攀升
- 🛡️ **护盾反射**：护盾激活时可反弹敌方子弹，化守为攻
- 🔥 **火墙技能**：Ember 专属，清除敌方弹幕并灼烧敌人
- 🤝 **双机联机**：UART 串口通信，PPP 帧协议，支持邀请/接受/断开全流程
- 💾 **存档系统**：双文件备份，key=value 文本格式，持久化金币与设置
- 🎵 **多通道音频**：1 BGM + 3 SFX 独立通道，7 首 BGM + 8 种音效
- 📐 **AABB 碰撞检测**：三层过滤（活跃→类型→几何），嵌入式友好
- 🏗️ **面向对象 C**：结构体嵌套 + 函数指针实现继承与多态
- 🎛️ **事件驱动架构**：注册/派发回调，模块解耦
- 🔄 **有限状态机**：9 个游戏状态，清晰管理界面流转

---

## 🖼️ 屏幕截图

> 游戏主界面、战斗画面、商店界面等截图可后续补充

---

## 🏗️ 系统架构

```
┌─────────────────────────────────────────────┐
│              主控层 (main.c / fsm.c)          │
│         入口初始化 · 主循环调度 · 状态机       │
├──────────┬──────────┬──────────┬─────────────┤
│ 游戏核心  │  玩家模块 │ 敌人模块  │  关卡管理   │
│ game.c   │ player.c │ enemy.c  │  level.c    │
│ game_obj │ bullet.c │ behavior │  timer.c    │
├──────┬───┴───┬──────┴──┬───────┴──┬──────────┤
│UI总控│游戏UI │ 菜单UI  │商店/设置  │ 通信UI    │
│ui.c  │play   │ menu/cg │shop/set  │comm/key  │
├──────┴───────┴─────────┴──────────┴──────────┤
│  输入驱动   │  通信协议栈  │  工具/音频/外观   │
│ input/key   │  comm/uart  │  tools/audio/apr  │
└─────────────┴─────────────┴───────────────────┘
```

---

## 🎮 操控方式

| 平台                 | 移动          | 技能X | 技能Y | 暂停 |
| -------------------- | ------------- | ----- | ----- | ---- |
| **MCU**（摇杆+按键） | 摇杆          | A 键  | B 键  | X 键 |
| **PC**（键盘）       | WASD / 方向键 | J 键  | K 键  | ESC  |

---

## 📁 项目结构

```
src/
├── main.c              # 程序入口，主循环
├── fsm.c / fsm.h       # 有限状态机
├── game.c / game.h     # 游戏核心管理器
├── game_object.c / .h  # 游戏对象基类
├── player.c / player.h # 玩家飞机系统
├── bullet.c / bullet.h # 子弹对象池
├── enemy.c / enemy.h   # 敌人管理
├── enemy_behaviors.c/.h# 敌人行为策略
├── level.c / level.h   # 关卡与波次
├── timer.c / timer.h   # 全局定时器池
├── event.c / event.h   # 事件系统
├── coin.c / coin.h     # 金币对象
├── flame_wall.c / .h   # 火墙技能
├── ui*.c / ui*.h       # UI 模块（8 组）
├── input_*.c / .h      # 输入驱动
├── key.c / joystick.c  # 按键与摇杆
├── comm*.c / uart.c    # 通信协议栈
├── ring_buffer.c / .h  # 环形缓冲区
├── multiplayer.c / .h  # 联机模块
├── pool.c / pool.h     # 通用对象池
├── save.c / save.h     # 存档系统
├── audio.c / audio.h   # 多通道音频
├── apr.c / apr.h       # 外观描述符
├── tools.c / debug.c   # 工具与调试
├── lv_port.c           # LVGL 平台适配
├── config.h            # 全局配置
└── protocol.h          # PPP 通信协议定义
```

---

## ⚙️ 配置参数

| 参数         | 值         | 说明        |
| ------------ | ---------- | ----------- |
| 屏幕分辨率   | 1024 × 600 | 像素        |
| 游戏逻辑帧率 | 30 Hz      | `GAME_TICK` |
| 最大子弹数   | 50         | 对象池容量  |
| 最大敌人数   | 10         | 同屏上限    |
| UART 波特率  | 115200     | 串口通信    |
| 摇杆死区     | 30         | MCU 端      |
| 长按判定     | 150 ms     | 按键去抖    |

---

## 🔧 依赖

- **[LVGL](https://lvgl.io/)** — 轻量级嵌入式图形库
- **[SDL2](https://www.libsdl.org/)** — PC 模拟器平台（显示/输入/音频）
- **GD32H7** — 目标 MCU 平台（ARM Cortex-M7）

---

## 🚀 编译与运行

### PC 模拟器

```bash
# 确保 SDL2 已安装
mkdir build && cd build
cmake .. -DSIMULATOR=ON
make -j$(nproc)
./sky_strike
```

### MCU 实体

```bash
# 使用 Keil / GD32 工具链
# 定义宏 SIMULATOR=0，选择目标芯片 GD32H7
# 烧录至开发板
```

### 联机模式

1. PC 端启动模拟器，MCU 端烧录固件
2. USB 串口线连接 PC 与 MCU（默认 COM11 / 115200）
3. 双方进入菜单 → 联机连接
4. 一方发送邀请，另一方接受即可开始联机战斗

---

## 🧩 核心设计

### 面向对象的 C 语言

```c
// 结构体嵌套实现继承
typedef struct {
    game_obj_t base;      // 嵌入基类
    int16_t hp;
    skill_t skill_x, skill_y;
} player_t;

// 函数指针实现多态
obj->update(obj);   // 统一接口，各类型自行实现
obj->hide(obj);
```

### 事件驱动

```c
// 注册回调
event_register(EVENT_BULLET_HIT_ENEMY, on_bullet_hit_enemy);

// 碰撞检测中派发
event_dispatch(EVENT_BULLET_HIT_ENEMY, bullet, enemy);
```

### 对象池模式

```c
bullet_t *b = bullet_alloc();   // 从池中分配
bullet_set_source(b, player);
b->base.active = true;          // 激活使用
// ...
b->base.hide(&b->base);        // 回收到池中
```

---

## 📜 协议格式

通信采用类 HDLC 的 **PPP 帧格式**：

```
┌──────┬──────┬──────┬─────────┬──────┬──────┐
│ 0x7B │ Type │ Len  │  Data   │ CRC  │ 0x7D │
│ SOF  │ 1B   │ 2B   │  N Bytes│ 1B   │ EOF  │
└──────┴──────┴──────┴─────────┴──────┴──────┘
```

帧类型：心跳、按键状态、摇杆数据、联机邀请/确认/取消/断开、金币同步。

---

## 📊 代码统计

| 指标         | 数值                |
| ------------ | ------------------- |
| 源文件数     | 94（47 .c + 47 .h） |
| 总代码行数   | 16,565              |
| 有效代码行数 | 10,092              |

---

## 📄 许可证

本项目为课程设计作品，仅供学习交流使用。

---

## 📕 附录

### 如何编译？

在 Windows 平台进行模拟器开发，需要准备以下环境：

- 编译器：MinGW-w64 (推荐 GCC 8.0 以上，确保其 bin 目录已加入系统环境变量 Path)。
- 构建工具：CMake (3.15 或以上版本) 与 mingw32-make。
- 模拟器图形库：SDL2 开发包 (推荐 MinGW 编译版本，如 x86_64-w64-mingw32)。
- LVGL 核心库：从 LVGL 官方仓库或发布页面下载v8.2版本，解压到项目根目录或手动指定。

推荐使用 Visual Studio Code 作为开发环境，配置 CMake 项目，在setting.json中添加以下内容：

```json
{
  "cmake.configureSettings": {
    "SDL2_PATH": "E:\\SDL2-2.32.10\\x86_64-w64-mingw32", // 替换为你的 SDL2 安装路径，要求含有include 和 lib 目录
    "LVGL_DIR": "E:\\lvgl", // 替换为你的 LVGL 安装路径，要求含有include 目录
    "LV_DRIVERS_DIR": "E:\\lv_drivers" // 替换为你的 LVGL 驱动安装路径，要求含有include 目录
  }
}
```
