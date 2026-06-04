#ifndef UI_BASE_H
#define UI_BASE_H

#include <stdbool.h>

/* ==========================================
 * 函数声明
 * ========================================== */

/**
 * @brief 初始化 UI 基础模块
 * 用于加载资源、初始化界面状态等
 */
void ui_base_init(void);

/**
 * @brief 运行 UI 核心循环
 * 处理界面刷新与用户交互逻辑
 */
void ui_base_run(void);


/* ==========================================
 * 全局变量声明
 * ========================================== */

/**
 * @brief 飞机解锁状态数组
 * 固定长度为 4，true 表示已解锁，false 表示未解锁
 */
extern bool g_plane_unlocked[4];

/**
 * @brief 当前选中的飞机 ID
 * 通常对应数组的索引（例如 0 ~ 3）
 */
int ui_base_get_selected_plane_id(void);

#endif /* UI_BASE_H */