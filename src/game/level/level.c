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

#include <stdint.h>

/**********************
 *      MACROS
 **********************/

#define LEVEL_COUNT 2
#define CLEAN_UP_DELAY_MS 3000  // 波次完成 敌人自然退场时间

/**********************
 *      TYPEDEFS
 **********************/

typedef enum {
    WAVE_PENDING = 0,
    WAVE_SPAWNING,
    WAVE_DONE
} wave_state_t;

typedef struct
{
    uint16_t enemy_total;
    uint16_t enemy_spawned;     // 生成指定数量 而不是杀灭指定数量
    uint32_t spawn_interval;
    uint32_t last_spawn;
    uint32_t wave_complete_tick;
} wave_t;

typedef struct {
    uint8_t total_waves;
    uint8_t current_wave;
    uint32_t wave_delay;
    uint32_t wave_start_time;
    wave_state_t state;
    bool waiting_cleanup;       // 敌人全部清空下一波？直接下一波？
    uint32_t cleanup_delay_ms;
    uint32_t cleanup_start_tick;
    wave_t * waves;
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

static wave_t level1_waves[] = {
    { .enemy_total = 5,  .spawn_interval = 2000 },
    { .enemy_total = 8,  .spawn_interval = 1500 },
    { .enemy_total = 10, .spawn_interval = 1200 }
};

static wave_t level2_waves[] = {
    { .enemy_total = 8,  .spawn_interval = 1200 },
    { .enemy_total = 12, .spawn_interval = 800  },
    { .enemy_total = 15, .spawn_interval = 600  }
};


static uint8_t current_level = 0;
static level_t level;
static level_t levels[LEVEL_COUNT];

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
    levels[0].wave_delay = 3000;
    levels[0].waves = level1_waves;

    levels[1].total_waves = sizeof(level2_waves) / sizeof(wave_t);
    levels[1].current_wave = 0;
    levels[1].wave_delay = 2000;
    levels[1].waves = level2_waves;

    event_register(EVENT_GAME_START,on_game_start);

    CONSOLE("[INFO] Level manager initialized. %d levels.", LEVEL_COUNT);
}

/**
 * @brief 关卡管理器的更新
 */
void level_update(void)
{
    if (fsm_get_state() != GS_PLAY) return;

    if (level.current_wave >= level.total_waves) {
        // 所有波次完成，可能已通关
        return;
    }

    wave_t *wave = &level.waves[level.current_wave];

    switch (level.state) {
        case WAVE_PENDING:
            // 等待波前延迟
            if (lv_tick_elaps(level.wave_start_time) >= level.wave_delay) {
                level.state = WAVE_SPAWNING;
                wave->last_spawn = lv_tick_get();
                CONSOLE("[LEVEL] Wave %d start!", level.current_wave + 1);
            }
            break;

        case WAVE_SPAWNING:
            // 生成敌人
            if (wave->enemy_spawned < wave->enemy_total) {
                if (lv_tick_elaps(wave->last_spawn) >= wave->spawn_interval) {
                    lv_coord_t x = lv_rand(250,774);
                    lv_coord_t y = -64;
                    enemy_spawn(x, y);
                    wave->enemy_spawned++;
                    wave->last_spawn = lv_tick_get();
                }
            }
            // 若已生成完毕，但没有等待清场
            if (wave->enemy_spawned >= wave->enemy_total && !level.waiting_cleanup) {
                level.waiting_cleanup = true;
                level.cleanup_delay_ms = CLEAN_UP_DELAY_MS; // 给敌人消失留出时间
                level.cleanup_start_tick = lv_tick_get();
                CONSOLE("[LEVEL] Wave %d spawning complete, waiting for cleanup...", level.current_wave + 1);
            }
            break;

        case WAVE_DONE:
            break;
    }

    // 等待清场延迟结束后，进入下一波
    if (level.waiting_cleanup) {
        if (lv_tick_elaps(level.cleanup_start_tick) >= level.cleanup_delay_ms) {
            level.waiting_cleanup = false;
            level.current_wave++;

            if (level.current_wave < level.total_waves) {
                // 设置新波为 PENDING
                level.wave_start_time = lv_tick_get();
                level.state = WAVE_PENDING;
                CONSOLE("[LEVEL] Entering wave %d", level.current_wave + 1);
            } else {
                // 关卡完成
                CONSOLE("[LEVEL] Level %d complete!", current_level + 1);
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

    // 重置每波的生成计数
    for (int i = 0; i < level.total_waves; i++) {
        level.waves[i].enemy_spawned = 0;
        level.waves[i].last_spawn = 0;
    }

    // 播放关卡进场动画（UI 模块提供）
    char level_name[32];
    snprintf(level_name, sizeof(level_name), "Level %d", level_id + 1);
    ui_play_level_enter_anim(level_name);

    CONSOLE("[LEVEL] Level %d loaded, %d waves.", level_id + 1, level.total_waves);
}

static void on_level_complete(void)
{
    if (current_level + 1 < LEVEL_COUNT) {
        current_level++;
        load_level(current_level);
    } else {
        CONSOLE("[LEVEL] All levels finished! You win!");
        // 这里可以触发游戏胜利事件或状态切换
    }
}
