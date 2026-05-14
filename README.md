# 根目录（你原有的不变）

├── .vscode/
├── assets/ # 图片/字体资源（敌机、飞机、精灵图）
├── build/ # CMake编译输出
├── js/
├── logs/
├── lv_conf.h # LVGL配置
├── lv_driver_conf.h # LVGL驱动配置
├── CMakeLists.txt
└── src/ # ✅ 核心改造区（我们重点设计）
├── main.h # 全局宏定义（屏幕尺寸、帧率、池大小）
├── main.c # 主函数：仅主循环 + 调用初始化
├── init/ # ✅ 你要求的：专属初始化模块（总调度）
│ ├── init.h
│ └── init.c
├── driver/ # 驱动层：硬件/LVGL底层（和业务无关）
│ ├── lv_port/ # LVGL移植（屏幕、触摸、定时器）
│ ├── display.h/.c # 画布渲染（清空、绘制、刷新）
│ └── input.h/.c # 触摸/按键输入
├── core/ # 核心层：通用游戏引擎（全项目复用）
│ ├── game_object.h/.c # OOP基类（封装/继承）
│ ├── pool.h/.c # 通用对象池（子弹/敌机复用）
│ ├── event.h/.c # 事件系统（解耦通信）
│ └── fsm.h/.c # 有限状态机
├── game/ # 游戏业务层：打飞机专属逻辑
│ ├── player.h/.c # 玩家飞机（FSM、移动、射击）
│ ├── enemy.h/.c # 敌机（对象池、AI）
│ ├── bullet.h/.c # 子弹（对象池、飞行）
│ └── collision.h/.c # 碰撞检测
└── ui/ # UI层：LVGL界面（和游戏逻辑分离）
├── menu.h/.c # 开始/暂停/结束菜单
└── hud.h/.c # 实时UI（分数、血条）
