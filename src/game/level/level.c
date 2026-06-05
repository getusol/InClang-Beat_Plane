/**
 * @file level.c
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

#define LEVEL_COUNT         5
#define CLEAN_UP_DELAY_MS   1500

/**********************
 *      TYPEDEFS
 **********************/

typedef enum {
    WAVE_PENDING = 0,
    WAVE_SPAWNING,
    WAVE_DONE
} wave_state_t;

typedef enum {
    WAVE_TYPE_NORMAL = 0,   // 普通敌人波次
    WAVE_TYPE_BOSS,         // Boss 波次
} wave_type_t;

typedef struct
{
    wave_type_t type;
    // 普通敌人参数
    uint16_t enemy_total;
    uint32_t spawn_interval;
    // Boss 参数
    uint16_t boss_hp;
    int16_t boss_damage;
    // 运行时
    uint16_t enemy_spawned;
    uint32_t last_spawn;
} wave_t;

typedef struct {
    uint8_t total_waves;
    uint8_t current_wave;
    uint32_t wave_delay;
    uint32_t wave_start_time;
    wave_state_t state;
    bool waiting_cleanup;
    uint32_t cleanup_delay_ms;
    uint32_t cleanup_start_tick;
    wave_t *waves;
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

// Level 1: 1 波普通敌人（入门）
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


static uint8_t current_level = 0;
static level_t level;
static level_t levels[LEVEL_COUNT];
static game_obj_t *current_boss = NULL;  // 追踪当前 Boss 是否存活

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 关卡管理器的初始化
 */
void level_init()
{
    levels[0].total_waves = sizeof(level1_waves) / sizeof(wave_t);
    levels[0].current_wave = 0;
    levels[0].wave_delay = 1500;
    levels[0].waves = level1_waves;

    levels[1].total_waves = sizeof(level2_waves) / sizeof(wave_t);
    levels[1].current_wave = 0;
    levels[1].wave_delay = 1500;
    levels[1].waves = level2_waves;

    levels[2].total_waves = sizeof(level3_waves) / sizeof(wave_t);
    levels[2].current_wave = 0;
    levels[2].wave_delay = 1200;
    levels[2].waves = level3_waves;

    levels[3].total_waves = sizeof(level4_waves) / sizeof(wave_t);
    levels[3].current_wave = 0;
    levels[3].wave_delay = 1000;
    levels[3].waves = level4_waves;

    levels[4].total_waves = sizeof(level5_waves) / sizeof(wave_t);
    levels[4].current_wave = 0;
    levels[4].wave_delay = 1000;
    levels[4].waves = level5_waves;

    event_register(EVENT_GAME_START, on_game_start);

    CONSOLE_INFO("Level manager initialized. %d levels.", LEVEL_COUNT);
}

/**
 * @brief 关卡管理器的更新
 */
void level_update(void)
{
    if (fsm_get_state() != GS_PLAY) return;

    if (level.current_wave >= level.total_waves) {
        return;
    }

    wave_t *wave = &level.waves[level.current_wave];

    switch (level.state) {
        case WAVE_PENDING:
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

        case WAVE_SPAWNING:
            if (wave->type == WAVE_TYPE_BOSS) {
                // Boss 波次：立即生成 Boss，保存指针用于检测存活
                if (wave->enemy_spawned == 0) {
                    behave_t boss_behave = { .f = enemy_behave_boss, .usr_data = NULL };
                    current_boss = enemy_spawn(SCREEN_WIDTH / 2, 70,
                                               0, 0,
                                               wave->boss_hp,
                                               200,  // 高伤害 ≈ 触碰秒杀
                                               boss_behave,
                                               APR_ENEMY_BOSS);
                    wave->enemy_spawned = 1;
                }

                // Boss 生成完毕 → 等待 Boss 被消灭（不设超时）
                if (!level.waiting_cleanup) {
                    level.waiting_cleanup = true;
                    CONSOLE_INFO("Boss wave waiting for boss defeat...");
                }

            } else {
                // 普通敌人波次：逐个生成
                if (wave->enemy_spawned < wave->enemy_total - 1) {
                    if (lv_tick_elaps(wave->last_spawn) >= wave->spawn_interval) {
                        lv_coord_t x = lv_rand(250, 774);
                        lv_coord_t y = -64;
                        behave_t behave = { .f = enemy_behave_normal, .usr_data = NULL };
                        enemy_spawn(x, y, 0, 0, 100, 20, behave, APR_ENEMY_DEFAULT);
                        wave->enemy_spawned++;
                        wave->last_spawn = lv_tick_get();
                    }
                } else if (wave->enemy_spawned == wave->enemy_total - 1) {
                    // 最后一个敌人：大怪
                    if (lv_tick_elaps(wave->last_spawn) >= wave->spawn_interval * 2) {
                        lv_coord_t x = lv_rand(250, 774);
                        lv_coord_t y = -64;
                        behave_t behave = { .f = enemy_behave_normal, .usr_data = NULL };
                        enemy_spawn(x, y, 0, 0, 1000, 20, behave, APR_ENEMY_DEFAULT);
                        wave->enemy_spawned++;
                        wave->last_spawn = lv_tick_get();
                    }
                }

                if (wave->enemy_spawned >= wave->enemy_total && !level.waiting_cleanup) {
                    level.waiting_cleanup = true;
                    level.cleanup_delay_ms = CLEAN_UP_DELAY_MS;
                    level.cleanup_start_tick = lv_tick_get();
                    CONSOLE_INFO("Wave %d spawning complete, waiting for cleanup...", level.current_wave + 1);
                }
            }
            break;

        case WAVE_DONE:
            break;
    }

    // 等待清场后进入下一波
    if (level.waiting_cleanup) {
        bool can_advance = false;

        if (wave->type == WAVE_TYPE_BOSS) {
            // Boss 波次：等到 Boss 被消灭
            if (current_boss == NULL || !game_obj_is_active(current_boss)) {
                can_advance = true;
                current_boss = NULL;
            }
        } else {
            // 普通波次：计时清场
            if (lv_tick_elaps(level.cleanup_start_tick) >= level.cleanup_delay_ms) {
                can_advance = true;
            }
        }

        if (can_advance) {
            level.waiting_cleanup = false;
            level.current_wave++;

            if (level.current_wave < level.total_waves) {
                level.wave_start_time = lv_tick_get();
                level.state = WAVE_PENDING;
                CONSOLE_INFO("Entering wave %d", level.current_wave + 1);
            } else {
                // 关卡完成
                CONSOLE_INFO("Level %d complete!", current_level + 1);
                on_level_complete();
            }
        }
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void on_game_start(game_obj_t *source, game_obj_t *target)
{
    current_level = 0;
    load_level(current_level);
}

static void load_level(uint8_t level_id)
{
    if (level_id >= LEVEL_COUNT) return;

    level = levels[level_id];
    level.current_wave = 0;
    level.state = WAVE_PENDING;
    level.wave_start_time = lv_tick_get();
    level.waiting_cleanup = false;
    current_boss = NULL;

    for (int i = 0; i < level.total_waves; i++) {
        level.waves[i].enemy_spawned = 0;
        level.waves[i].last_spawn = 0;
    }

    char level_name[32];
    snprintf(level_name, sizeof(level_name), "Level %d", level_id + 1);
    ui_play_level_enter_anim(level_name);

    CONSOLE_INFO("Level %d loaded, %d waves.", level_id + 1, level.total_waves);
}

static void on_level_complete(void)
{
    if (current_level + 1 < LEVEL_COUNT) {
        current_level++;
        load_level(current_level);
    } else {
        CONSOLE_INFO("All %d levels finished! You win!", LEVEL_COUNT);
        // 游戏通关后可循环到最后一关
        load_level(LEVEL_COUNT - 1);
    }
}
