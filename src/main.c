#include "lvgl.h"
#include "lv_port.h"
#include "uart.h"
#include "tools.h"
#include "ui.h"
#include "fsm.h"
#include "input_sw.h"
#include "main.h"
#include "player.h"
#include "bullet.h"
#include "config.h"
#include "game.h"
#include "event.h"
#include "perf_monitor.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    //Inits
    tools_init();
    lv_port_init();
    uart_debug_init(115200);
    input_init();
    fsm_init();
    event_init();
    ui_init();
    game_init();

    non_blocking_timer_t logic_timer = {
        .func = game_update,
        .tick_get = lv_tick_get,
        .delay_ms = 1000 / GAME_TICK,
        .last_tick = 0,
        .usr_data = NULL,
    };
    non_blocking_timer_t ui_timer = {
        .func = ui_run,
        .tick_get = lv_tick_get,
        .delay_ms = 30,
        .last_tick = 0,
        .usr_data = NULL
    };
    non_blocking_timer_t input_timer = {
        .func = input_dispatch,
        .tick_get = lv_tick_get,
        .delay_ms = SCAN_RATE_MS,
        .last_tick = 0,
        .usr_data = NULL
    };

    CONSOLE("[INFO] Initialization done!");
    LOG("[INFO] Initialization done!");

    while(1) {
        non_blocking_delay(&input_timer,NULL,false);
        non_blocking_delay(&logic_timer,NULL,false);
        non_blocking_delay(&ui_timer,NULL,false);
        
        uint32_t t_start = lv_tick_get();
        lv_timer_handler();
        uint32_t t_end = lv_tick_get();
        perf_monitor_set_mspf(t_end - t_start);

        perf_monitor_update();  //更新信息显示
        delay_ms(1);
    }
    return 0;
}
