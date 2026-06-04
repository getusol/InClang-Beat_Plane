#include "lvgl.h"
#include "lv_port.h"
#include "tools.h"
#include "ui.h"
#include "fsm.h"
#include "uart.h"
#include "input_sw.h"
#include "main.h"
#include "player.h"
#include "bullet.h"
#include "config.h"
#include "game.h"
#include "save.h"
#include "event.h"
#include "perf_monitor.h"
#include "audio.h"
#include "comm.h"

int main(int argc, char **argv)
{
    //Inits
    tools_init();
    lv_port_init();
    uart_enable();
    comm_init();
    input_init();
    audio_init();
    fsm_init();
    event_init();


    ui_init();
    game_init();
    save_load();

    non_blocking_timer_t logic_timer = {
        .func = game_update,
        .tick_get = lv_tick_get,
        .delay_ms = 1000 / GAME_TICK,
        .last_tick = 0,
    };
    non_blocking_timer_t ui_timer = {
        .func = ui_run,
        .tick_get = lv_tick_get,
        .delay_ms = 30,
        .last_tick = 0,
    };
    non_blocking_timer_t input_timer = {
        .func = input_dispatch,
        .tick_get = lv_tick_get,
        .delay_ms = SCAN_RATE_MS,
        .last_tick = 0,
    };
    non_blocking_timer_t comm_timer = {
        .func = comm_update,
        .tick_get = lv_tick_get,
        .delay_ms = 2,
        .last_tick = 0,
    };

    CONSOLE_INFO("Initialization done!");
    LOG_INFO("Initialization done!");

    while(1) {
        non_blocking_delay(&input_timer);
        non_blocking_delay(&logic_timer);
        non_blocking_delay(&ui_timer);
        non_blocking_delay(&comm_timer);

        uint32_t t_start = lv_tick_get();
        lv_timer_handler();
        uint32_t t_end = lv_tick_get();
        perf_monitor_set_mspf(t_end - t_start);

        perf_monitor_update();  //更新信息显示
        delay_ms(1);

    }
    return 0;
}
