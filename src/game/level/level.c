/**
 * @file level.c
 * @brief 关卡管理器 —— 管理游戏的 5 个关卡，每关由若干波次(Wave)组成
 *
 * 核心概念：
 *   关卡(Level) = N 个波次(Wave)
 *   波次有两种：
 *     - NORMAL: 逐个生成敌人，最后一个是大怪
 *     - BOSS:   生成一个 Boss，打死才过关
 *
 * 波次状态机：
 *   WAVE_PENDING ──等待 wave_delay ms──> WAVE_SPAWNING ──清场──> 下一波 / 通关
 */

/*********************
 *      INCLUDES
 *********************/

#include "level.h"
#include "tools.h"
#include "event.h"
#include "ui_play.h"
#include "fsm.h"
#include "enemy.h"
#include "enemy_behaviors.h"
#include "apr.h"

#include <stdint.h>

/**********************
 *      MACROS
 **********************/

#define LEVEL_COUNT         5       // 总共 5 个关卡
#define CLEAN_UP_DELAY_MS   1500    // 普通波次生成完毕后，等 1.5 秒再进下一波

/**********************
 *      TYPEDEFS
 **********************/

/** 波次运行状态 */
typedef enum {
    WAVE_PENDING = 0,   // 等待开始（波间延时中）
    WAVE_SPAWNING,      // 正在生成敌人
    WAVE_DONE           // 已完成
} wave_state_t;

/** 波次类型 */
typedef enum {
    WAVE_TYPE_NORMAL = 0,   // 普通敌人波次：逐个生成多个敌人
    WAVE_TYPE_BOSS,         // Boss 波次：只生成一个 Boss
} wave_type_t;

/** 一波敌人的定义 + 运行时状态 */
typedef struct
{
    wave_type_t type;           // NORMAL 还是 BOSS

    // —— 普通敌人参数 ——
    uint16_t enemy_total;       // 这一波总共要生成几个敌人
    uint32_t spawn_interval;    // 两个敌人之间的间隔(ms)

    // —— Boss 参数 ——
    uint16_t boss_hp;           // Boss 血量
    int16_t boss_damage;        // Boss 伤害

    // —— 运行时计数 ——
    uint16_t enemy_spawned;     // 已经生成了几个
    uint32_t last_spawn;        // 上一次生成的时间戳(lv_tick)
} wave_t;

/** 一个关卡的完整状态 */
typedef struct {
    uint8_t total_waves;        // 本关共几波
    uint8_t current_wave;       // 当前进行到第几波(从0开始)
    uint32_t wave_delay;        // 波与波之间的等待时间(ms)
    uint32_t wave_start_time;   // 当前波开始的时间戳
    wave_state_t state;         // 当前波的状态(PENDING/SPAWNING/DONE)
    bool waiting_cleanup;       // true = 正在等待清场/等 Boss 死亡
    uint32_t cleanup_delay_ms;  // 清场等待时间
    uint32_t cleanup_start_tick;// 清场开始时间戳
    wave_t *waves;              // 指向本关波次定义数组的指针
} level_t;


/**********************
 *  STATIC PROTOTYPES
 **********************/

static void on_game_start(game_obj_t *source, game_obj_t *target);
static void load_level(uint8_t level_id);
static void on_level_complete(void);

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**********************
 *  STATIC VARIABLES
 **********************/

// ==================== 关卡定义 ====================

// Level 1: 1 波普通敌人（入门关，熟悉操作）
static wave_t level1_waves[] = {
    { .type = WAVE_TYPE_NORMAL, .enemy_total = 2, .spawn_interval = 1200 },
};

// Level 2: 2 波普通敌人 + 1 波 Boss
static wave_t level2_waves[] = {
    { .type = WAVE_TYPE_NORMAL, .enemy_total = 3, .spawn_interval = 1000 },
    { .type = WAVE_TYPE_NORMAL, .enemy_total = 4, .spawn_interval = 800  },
    { .type = WAVE_TYPE_BOSS,   .boss_hp = 800,    .boss_damage = 30 },
};

// Level 3: 3 波普通敌人 + 1 波 Boss
static wave_t level3_waves[] = {
    { .type = WAVE_TYPE_NORMAL, .enemy_total = 3, .spawn_interval = 900  },
    { .type = WAVE_TYPE_NORMAL, .enemy_total = 4, .spawn_interval = 700  },
    { .type = WAVE_TYPE_NORMAL, .enemy_total = 5, .spawn_interval = 600  },
    { .type = WAVE_TYPE_BOSS,   .boss_hp = 1500,   .boss_damage = 40 },
};

// Level 4: 4 波普通敌人 + 1 波 Boss
static wave_t level4_waves[] = {
    { .type = WAVE_TYPE_NORMAL, .enemy_total = 4, .spawn_interval = 800  },
    { .type = WAVE_TYPE_NORMAL, .enemy_total = 5, .spawn_interval = 700  },
    { .type = WAVE_TYPE_NORMAL, .enemy_total = 5, .spawn_interval = 600  },
    { .type = WAVE_TYPE_NORMAL, .enemy_total = 6, .spawn_interval = 500  },
    { .type = WAVE_TYPE_BOSS,   .boss_hp = 2500,   .boss_damage = 50 },
};

// Level 5: 4 波普通敌人 + 1 波 Boss（噩梦难度）
static wave_t level5_waves[] = {
    { .type = WAVE_TYPE_NORMAL, .enemy_total = 5, .spawn_interval = 700  },
    { .type = WAVE_TYPE_NORMAL, .enemy_total = 6, .spawn_interval = 600  },
    { .type = WAVE_TYPE_NORMAL, .enemy_total = 7, .spawn_interval = 500  },
    { .type = WAVE_TYPE_NORMAL, .enemy_total = 8, .spawn_interval = 400  },
    { .type = WAVE_TYPE_BOSS,   .boss_hp = 4000,   .boss_damage = 60 },
};


static uint8_t current_level = 0;           // 玩家当前在第几关(0-based)
static level_t level;                       // 当前正在玩的关卡(从 levels 数组拷贝出来)
static level_t levels[LEVEL_COUNT];         // 5 个关卡的模板数组
static game_obj_t *current_boss = NULL;     // 当前 Boss 对象指针，用于检测 Boss 是否还活着

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 关卡管理器初始化
 * @note  把 5 个关卡的静态定义灌入 levels[] 数组，
 *        并注册 EVENT_GAME_START 事件回调
 */
void level_init()
{
    // Level 1: 1 波普通敌人
    levels[0].total_waves = sizeof(level1_waves) / sizeof(wave_t);
    levels[0].current_wave = 0;
    levels[0].wave_delay = 1500;
    levels[0].waves = level1_waves;

    // Level 2: 2 波普通 + 1 波 Boss
    levels[1].total_waves = sizeof(level2_waves) / sizeof(wave_t);
    levels[1].current_wave = 0;
    levels[1].wave_delay = 1500;
    levels[1].waves = level2_waves;

    // Level 3: 3 波普通 + 1 波 Boss
    levels[2].total_waves = sizeof(level3_waves) / sizeof(wave_t);
    levels[2].current_wave = 0;
    levels[2].wave_delay = 1200;
    levels[2].waves = level3_waves;

    // Level 4: 4 波普通 + 1 波 Boss
    levels[3].total_waves = sizeof(level4_waves) / sizeof(wave_t);
    levels[3].current_wave = 0;
    levels[3].wave_delay = 1000;
    levels[3].waves = level4_waves;

    // Level 5: 4 波普通 + 1 波 Boss (噩梦)
    levels[4].total_waves = sizeof(level5_waves) / sizeof(wave_t);
    levels[4].current_wave = 0;
    levels[4].wave_delay = 1000;
    levels[4].waves = level5_waves;

    // 游戏开始时自动加载第一关
    event_register(EVENT_GAME_START, on_game_start);

    CONSOLE_INFO("Level manager initialized. %d levels.", LEVEL_COUNT);
}

/**
 * @brief 关卡管理器每帧更新（由 game_update 驱动，30Hz）
 * @note  这是整个关卡系统的核心，做了三件事：
 *        1. PENDING  → 等延时到了，切 SPAWNING
 *        2. SPAWNING → 逐个生成敌人 / 生成 Boss，完了设 waiting_cleanup
 *        3. 清场完毕 → 进下一波 / 通关
 */
void level_update(void)
{
    // 只在游戏进行中运行
    if (fsm_get_state() != GS_PLAY) return;

    // 所有波次已完成，不再处理
    if (level.current_wave >= level.total_waves) {
        return;
    }

    wave_t *wave = &level.waves[level.current_wave];

    switch (level.state) {

        // ===== 阶段1: 等待波次开始 =====
        case WAVE_PENDING:
            // 波间延时到了？切 SPAWNING
            if (lv_tick_elaps(level.wave_start_time) >= level.wave_delay) {
                level.state = WAVE_SPAWNING;
                wave->last_spawn = lv_tick_get();

                if (wave->type == WAVE_TYPE_BOSS) {
                    CONSOLE_INFO("Boss wave %d incoming!", level.current_wave + 1);
                } else {
                    CONSOLE_INFO("Wave %d start! (%d enemies)", level.current_wave + 1, wave->enemy_total);
                }
            }
            break;

        // ===== 阶段2: 正在生成敌人 =====
        case WAVE_SPAWNING:

            if (wave->type == WAVE_TYPE_BOSS) {
                // —— Boss 波次 ——
                // 只生成一次 Boss，然后等它被打死
                if (wave->enemy_spawned == 0) {
                    behave_t boss_behave = { .f = enemy_behave_boss, .usr_data = NULL };
                    current_boss = enemy_spawn(
                        SCREEN_WIDTH / 2, 70,    // 屏幕顶部中间
                        0, 0,                     // 不移动
                        wave->boss_hp,
                        2000,                      // 高伤害 = 触碰秒杀
                        boss_behave,
                        APR_ENEMY_BOSS
                    );
                    wave->enemy_spawned = 1;
                }

                // Boss 已生成，开始等待它被消灭
                if (!level.waiting_cleanup) {
                    level.waiting_cleanup = true;
                    CONSOLE_INFO("Boss wave waiting for boss defeat...");
                }

            } else {
                // —— 普通敌人波次 ——
                // 逐个生成敌人，前 N-1 个是小怪，最后 1 个是大怪

                // 前 N-1 个：小怪 (HP 100)
                if (wave->enemy_spawned < wave->enemy_total - 1) {
                    if (lv_tick_elaps(wave->last_spawn) >= wave->spawn_interval) {
                        lv_coord_t x = lv_rand(250, 774);  // 随机 X 位置
                        lv_coord_t y = -64;                 // 从屏幕上方飞入
                        behave_t behave = { .f = enemy_behave_normal, .usr_data = NULL };
                        enemy_spawn(x, y, 0, 0, 100, 20, behave, APR_ENEMY_DEFAULT);
                        wave->enemy_spawned++;
                        wave->last_spawn = lv_tick_get();
                    }
                }
                // 最后一个：大怪 (HP 1000)，间隔 ×2 给玩家喘息时间
                else if (wave->enemy_spawned == wave->enemy_total - 1) {
                    if (lv_tick_elaps(wave->last_spawn) >= wave->spawn_interval * 2) {
                        lv_coord_t x = lv_rand(250, 774);
                        lv_coord_t y = -64;
                        behave_t behave = { .f = enemy_behave_normal, .usr_data = NULL };
                        enemy_spawn(x, y, 0, 0, 1000, 20, behave, APR_ENEMY_DEFAULT);
                        wave->enemy_spawned++;
                        wave->last_spawn = lv_tick_get();
                    }
                }

                // 全部生成完毕，开始清场等待
                if (wave->enemy_spawned >= wave->enemy_total && !level.waiting_cleanup) {
                    level.waiting_cleanup = true;
                    level.cleanup_delay_ms = CLEAN_UP_DELAY_MS;   // 1.5 秒
                    level.cleanup_start_tick = lv_tick_get();
                    CONSOLE_INFO("Wave %d spawning complete, waiting for cleanup...", level.current_wave + 1);
                }
            }
            break;

        case WAVE_DONE:
            break;
    }

    // ===== 清场判断：是否可以进下一波 =====
    if (level.waiting_cleanup) {
        bool can_advance = false;

        if (wave->type == WAVE_TYPE_BOSS) {
            // Boss 波次：Boss 死了才进下一波
            if (current_boss == NULL || !game_obj_is_active(current_boss)) {
                can_advance = true;
                current_boss = NULL;
            }
        } else {
            // 普通波次：计时到了就进下一波
            if (lv_tick_elaps(level.cleanup_start_tick) >= level.cleanup_delay_ms) {
                can_advance = true;
            }
        }

        if (can_advance) {
            level.waiting_cleanup = false;
            level.current_wave++;

            if (level.current_wave < level.total_waves) {
                // 还有下一波 → 重置状态，等 PENDING 延时
                level.wave_start_time = lv_tick_get();
                level.state = WAVE_PENDING;
                CONSOLE_INFO("Entering wave %d", level.current_wave + 1);
            } else {
                // 本关所有波次已完成
                CONSOLE_INFO("Level %d complete!", current_level + 1);
                on_level_complete();
            }
        }
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief EVENT_GAME_START 回调：游戏开始时从第 1 关开始
 */
static void on_game_start(game_obj_t *source, game_obj_t *target)
{
    current_level = 0;
    load_level(current_level);
}

/**
 * @brief 加载指定关卡
 * @note  从 levels[level_id] 模板拷贝一份到 level，重置所有运行时状态
 */
static void load_level(uint8_t level_id)
{
    if (level_id >= LEVEL_COUNT) return;

    // 从模板拷贝关卡数据
    level = levels[level_id];
    // 重置运行时状态
    level.current_wave = 0;
    level.state = WAVE_PENDING;
    level.wave_start_time = lv_tick_get();
    level.waiting_cleanup = false;
    current_boss = NULL;

    // 清空所有波次的生成计数器
    for (int i = 0; i < level.total_waves; i++) {
        level.waves[i].enemy_spawned = 0;
        level.waves[i].last_spawn = 0;
    }

    // 播放关卡标题入场动画
    char level_name[32];
    snprintf(level_name, sizeof(level_name), "Level %d", level_id + 1);
    ui_play_level_enter_anim(level_name);

    CONSOLE_INFO("Level %d loaded, %d waves.", level_id + 1, level.total_waves);
}

/**
 * @brief 关卡完成回调：进下一关，通关后循环最后一关
 */
static void on_level_complete(void)
{
    if (current_level + 1 < LEVEL_COUNT) {
        // 还有下一关 → 继续
        current_level++;
        load_level(current_level);
    } else {
        // 5 关全通 → 循环最后一关
        CONSOLE_INFO("All %d levels finished! You win!", LEVEL_COUNT);
        load_level(LEVEL_COUNT - 1);
    }
}
