# 项目当前状态与下一步计划

## 1. 项目概况

这是一个基于 LVGL 的嵌入式游戏框架，采用面向对象（C风格继承）设计，核心结构体 `game_obj_t` 作为所有游戏对象（玩家、子弹、敌机等）的基类。目前已完成 **玩家模块(player.c)** 和 **子弹模块(bullet.c)** 的基础框架，但存在若干设计和技术债务需要解决。

---

## 2. 当前代码存在的问题

### 2.1 缺少只读 Getter 接口

- 玩家模块只有 `player_hp_modify(delta)` 可以间接获取 HP（但有副作用），位置只能通过 `player_move(0,0)` 获取（也有重绘副作用）。
- 子弹模块目前无暴露接口。
- 通用的 `x`, `y`, `w`, `h`, `speed`, `active` 等属性未提供统一 getter。

### 2.2 对象更新调用方式不清晰

- `game_obj_t` 内部有 `update`, `show`, `hide` 函数指针，但外部目前通过 `player_get()` 暴露基类指针来触发更新，依赖暴露内部结构。
- 未来多种对象共存时，需要一个全局对象管理器统一调度更新，而不是每个对象手动调用。

### 2.3 子弹对象池管理不完整

- 子弹的 `bullet_t` 结构体中没有 `pool_index` 字段，无法在使用 `pool_free` 时归还索引（虽然可以通过指针计算下标，但当前未实现）。
- `bullet_init` 预创建了所有子弹的 LVGL 对象，但未调用 `pool_alloc` 分配池槽位，`hide` 时也不回收。
- 共享的图像缓冲区 (`bullet_img_buf`) 需要正确处理生命周期。

### 2.4 封装与外露矛盾

- `player_get()` 返回 `game_obj_t*`，外部可通过强转访问具体子类字段（如 `((player_t*)player_get())->hp`），破坏封装。
- 缺少原则指导：**对外只暴露功能函数，不暴露结构体指针本身**（除注册到管理器外）。

### 2.5 初始化函数的调用问题

- `player_init()` 函数 包含了玩家的lvgl渲染图片绑定 需要提供父容器 目前将其放在 `ui_play.c` 中以方便获取父容器 但是现在认为 可能将 `play_display` 通过 getter 获取出来 并将所有 `obj_init()` 与 `ui` 分离单独初始化会更好

---

## 3. 待办事项清单（按优先级排序）

### P0：立即实施

- [x] **在 `game_obj.h` 中声明通用 getter 接口**
  - `game_obj_get_x`, `game_obj_get_y`, `game_obj_get_w`, `game_obj_get_h`, `game_obj_get_speed`, `game_obj_is_active`, `game_obj_get_pos`
  - 实现放在 `game_obj.c` 或 `tools.c`，所有子类可直接复用。
- [ ] **为玩家添加必要的只读 getter**
  - `player_get_hp()`, `player_get_max_hp()` （无副作用）
  - 移除 `player_pos()` 和 `player_move(0,0)` 作为位置获取方式，改用 `game_obj_get_pos(player_get())`。

### P1：重要且待设计

- [ ] **设计全局对象管理器（Game Manager）**
  - 维护一个（或两个：仅活跃/全部） `game_obj_t*` 列表。
  - 提供 `game_register_obj(game_obj_t*)`、`game_unregister_obj(game_obj_t*)`、`game_update_all()`。
  - 主循环中只调用 `game_update_all()`，各对象自行注册。
  - 允许 `player_init` 返回 `game_obj_t*` 并自动注册。
- [ ] **完善子弹对象池**
  - 在 `bullet_t` 中添加 `uint16_t pool_index` 字段（或直接通过指针计算下标并调用 `pool_free`）。
  - 修改 `bullet_init`：不再预创所有对象，改为首次发射时创建，或者保留预创但记录每个子弹的槽位索引。
  - 实现 `bullet_create(...)`、`bullet_destroy(int idx)` 接口。
  - `bullet_hide` 中调用 `pool_free` 回收索引，并隐藏 LVGL 对象，**不释放共享缓冲区**。

### P2：后续优化

- [ ] **逐步弱化 `player_get()`**
  - 暂时保留它以便过渡，但文档标注为“内部使用”，推荐外部代码使用专用 getter 和统一更新。
- [ ] **为子弹添加必要 getter**
  - `bullet_get_damage(const game_obj_t*)` 供碰撞检测使用。
- [ ] **规范所有子类的创建模式**
  - 统一使用 `_init(parent)` 或 `_create(parent)` 返回基类指针，并自动注册到全局管理器。

---

## 4. 设计原则（给下一个AI的提示）

### 4.1 封装与接口

- **对外只暴露功能函数**，而非结构体指针（除了注册到管理器时）。
- **通用属性（位置、尺寸、速度、活跃状态）全部走 `game_obj_*` 系列 getter**，不得直接访问结构体字段。
- **特有属性（如玩家 HP、子弹伤害）** 按需提供 `player_get_hp()`、`bullet_get_damage()` 等只读接口，**避免带副作用的函数**（类似 `player_hp_modify(0)` 既是 setter 又是 getter 的设计应重构）。

### 4.2 更新驱动

- 所有游戏对象的 `update` 应通过**全局管理器统一调度**，不得各自为政。
- 每个对象在 `_init` 或创建时将自己注册到管理器，删除/隐藏时移出。
- 尽量避免外部代码直接调用 `player_p->base.update(...)`。

### 4.3 内存与生命周期

- **共享资源（图片缓冲区）** 由对应模块系统级管理（`_init` 一次性分配，`_deinit` 统一释放）。
- **每个对象（子弹、敌机等）** 的私有的 LVGL 对象和状态，采用对象池或链表管理，隐藏时只隐藏/回收槽位，**不释放共享资源**。
- 子弹 `hide` 应归还池索引，标记 `active = false`，隐藏 LVGL 对象。

### 4.4 类型安全

- 谨慎使用 `(game_obj_t*)&player_p` 强转，确保 `player_t` 首成员为 `game_obj_t base;`。
- 所有 getter 接收 `const game_obj_t*`，保证只读语义。

---

## 5. 当前代码风格示例（下一阶段期望）

```c
// player.h
game_obj_t* player_init(lv_obj_t *parent);
int16_t player_get_hp(void);
int16_t player_get_max_hp(void);
void player_clear_registration(void); // 移除注册，以便重启

// player.c
#include "game_mgr.h"
void player_init(lv_obj_t *parent) {
    // ... 初始化 ...
    game_mgr_register(&player_p->base);
}
```

```c
// bullet.c
game_obj_t* bullet_create(lv_obj_t *parent, int16_t damage, game_obj_t *source) {
    int idx = pool_alloc(&bullet_pool);
    if (idx < 0) return NULL;
    bullet_t *b = &bullets[idx];
    // 初始化 ...
    game_mgr_register(&b->base);
    return &b->base;
}
void bullet_destroy(game_obj_t *g) {
    bullet_t *b = (bullet_t*)g;
    game_mgr_unregister(g);
    pool_free(&bullet_pool, b->pool_index);
    lv_obj_del(g->obj);  // 销毁 LVGL 对象（如果预创建则隐藏）
}
```

---

## 6. 给下一个AI的注意事项

- **不要修改头文件中已存在的结构体布局**（`game_obj_t`、`player_t`、`bullet_t`），除非达成一致。
- **优先补充 getter**，不要急于删除现有内部字段暴露接口（如 `player_get()`），应先过渡。
- **所有新增接口必须添加 Doxygen 注释**。
- 不要引入新的第三方库或复杂设计模式，保持嵌入式项目简洁。
- 如果发现某个函数同时承担 getter 和 setter 职责（如 `player_move(0,0)` 实际是位置获取），应拆分为两个独立函数。

---

_以上为当前会议结论，请继续推进 P0 和 P1 事项。_
