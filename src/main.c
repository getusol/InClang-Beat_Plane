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
#include <stdio.h>

int main(int argc, char **argv)
{
    //Inits
    tools_init();
    lv_port_init();
    uart_debug_init(115200);
    input_init();
    fsm_init();
    ui_init();
    game_init();

    non_blocking_timer_t logic_timer = {
        .func = game_update,
        .tick_get = lv_tick_get,
        .delay_ms = 1000 / GAME_TICK,
        .last_tick = 0
    };
    non_blocking_timer_t ui_timer = {
        .func = ui_run,
        .tick_get = lv_tick_get,
        .delay_ms = 30,
        .last_tick = 0
    };
    non_blocking_timer_t input_timer = {
        .func = input_dispatch,
        .tick_get = lv_tick_get,
        .delay_ms = SCAN_RATE_MS,
        .last_tick = 0
    };

    CONSOLE("[INFO] Initialization done!");
    LOG("[INFO] Initialization done!");

    while(1) {
        non_blocking_delay(&input_timer);
        non_blocking_delay(&logic_timer);
        non_blocking_delay(&ui_timer);
        //last tick 末尾更新 写在函数中，而不是while循环;
        delay_ms(max(1, lv_timer_handler())); // 避免过度占用CPU，至少延时1ms
    }
    return 0;
}
