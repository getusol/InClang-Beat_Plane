/**
 * @file ui_lobby.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "ui_lobby.h"
#include "lvgl.h"
#include "config.h"
#include "lvgl_utils.h"
#include "input_device.h"
#include "fsm.h"
#include "character.h"
#include "input_sw.h"
#include "ui_base.h" // for ui_base_get_selected_character_id()
#include "event.h"
#include "player.h"
#include "debug.h"
#include "player_behaviors.h"
#include "comm_tx.h"
#include "comm_rx.h"
#include "comm_status.h"

/**********************
 *      MACROS
 **********************/

#define SINGLE_ICON "2DSingleIcon.bin"   // 77 * 96
#define MULTI_ICON "2DMultiIcon.bin"     // 96 * 82
#define KEYBOARD_IMG "keyboard.bin"      // 96 * 46
#define JOYSTICK_IMG "joystick.bin"      // 64 * 41
#define MCU_IMG "MCU.bin"                // 76 * 35
#define LEFT_ARROW_IMG "LeftArrow.bin"   // 16 * 16
#define RIGHT_ARROW_IMG "RightArrow.bin" // 16 * 16
#define HAND_SHAKE "handshake.bin"       // 64 * 50

/**********************
 *      TYPEDEFS
 **********************/

typedef enum
{
    MODE_CHOOSE = 0,
    MODE_CHOOSE_SINGLE,
    MODE_CHOOSE_MULTI,

    MODE_CNT,
} lobby_state_t;

typedef struct
{
    lobby_state_t state;
    int navigate_index[MAX_PLAYER_COUNT];
    int player_count;
    struct
    {
        input_device_t *dev;
        character_id_t character_id;
        bool ready;
    } slots[MAX_PLAYER_COUNT];
    bool ui_dirty;
    bool game_start_requested;
} lobby_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

// 工具函数
static void fetch_unlocked_characters(); // 获取已解锁的角色ID

// event callbacks
static void ccard_single_on_click(lv_event_t *e);
static void ccard_multi_on_click(lv_event_t *e);
static void ccard_join_on_click(lv_event_t *e);

// 按键回调
static void ui_lobby_A_pressed_handler(input_event_t *e);

/**********************
 *  STATIC VARIABLES
 **********************/

static lobby_t lobby;

static character_id_t unlockeds[CHARACTER_ID_MAX] = {0};
static int unlockeds_count = 0;

static lv_obj_t *dp_lobby = NULL;
static lv_obj_t *mode_root[MODE_CNT] = {NULL};
static lv_obj_t *ccard_single = NULL;
static lv_obj_t *ccard_multi = NULL;
static lv_obj_t *ccard_join = NULL;
static lv_obj_t *dev_imgs[INPUT_DEVICE_COUNT] = {NULL};
static lv_obj_t *character_img[MAX_PLAYER_COUNT] = {NULL}; // 显示选择的飞机图标，支持3个设备 3个玩家
static lv_obj_t *info_cards[MAX_PLAYER_COUNT] = {NULL};    // 显示玩家信息的卡片

// imgs
#ifdef SIMULATOR
static lv_img_dsc_t single_icon_dsc;
static lv_img_dsc_t multi_icon_dsc;
static lv_img_dsc_t keyboard_img_dsc;
static lv_img_dsc_t mcu_img_dsc;
static lv_img_dsc_t joystick_img_dsc;
static lv_img_dsc_t left_arrow_img_dsc;
static lv_img_dsc_t right_arrow_img_dsc;
static lv_img_dsc_t handshake_img_dsc;
#endif

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 初始化lobby界面 第一阶段
 */
void ui_lobby_init_stage1(void)
{
    dp_lobby = lv_obj_create(NULL);
    for (int i = 0; i < MODE_CNT; i++)
    {
        mode_root[i] = lv_obj_create(dp_lobby);
        lv_obj_set_size(mode_root[i], 1024, 600);
        lv_obj_clear_flag(mode_root[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_bg_opa(mode_root[i], LV_OPA_0, LV_STATE_DEFAULT);
        lv_obj_add_flag(mode_root[i], LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_set_style_bg_color(dp_lobby, lv_color_hex(DP_PLAY_FILL_COLOR), 0);
    memset(&lobby, 0, sizeof(lobby));
}

/**
 * @brief ui内容绘制函数 第二阶段 初始化lobby界面的元素
 */
void ui_lobby_init_stage2(void)
{

    char path_buf[64] = {0};

    // 选择界面

    // 选项卡单人游戏
    ccard_single = lv_obj_create(mode_root[MODE_CHOOSE]);
    lv_obj_set_size(ccard_single, 250, 400);
    lv_obj_align(ccard_single, LV_ALIGN_CENTER, -125, 0); /* 默认 join 隐藏布局 */
    lv_obj_set_style_bg_color(ccard_single, lv_color_hex(0x808080), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ccard_single, LV_OPA_50, LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ccard_single, lv_palette_main(LV_PALETTE_BLUE), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ccard_single, 5, LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ccard_single, LV_OPA_COVER, LV_STATE_DEFAULT); // 完全不透明
    lv_obj_set_style_radius(ccard_single, 10, LV_STATE_DEFAULT);
    lv_obj_add_flag(ccard_single, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(ccard_single, ccard_single_on_click, LV_EVENT_CLICKED, NULL);

    // Icon 图片 单人
    lv_obj_t *single_icon = NULL;
#ifdef SIMULATOR
    single_icon = img_create_from_dsc(ccard_single, img_path(SINGLE_ICON, path_buf, sizeof(path_buf)), 77, 96, NULL, &single_icon_dsc, true);
#else
    single_icon = lv_img_create(ccard_single);
    lv_img_set_src(single_icon, img_path(SINGLE_ICON, path_buf, sizeof(path_buf)));
#endif
    lv_obj_align(single_icon, LV_ALIGN_CENTER, 0, -50);

    // Label 单人
    lv_obj_t *single_label = lv_label_create(ccard_single);
    lv_label_set_text(single_label, "Single Player");
    lv_obj_align(single_label, LV_ALIGN_CENTER, 0, 50);
    lv_obj_set_style_text_color(single_label, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(single_label, &lv_font_montserrat_24, LV_STATE_DEFAULT);

    // 选项卡多人游戏
    ccard_multi = lv_obj_create(mode_root[MODE_CHOOSE]);
    lv_obj_set_size(ccard_multi, 250, 400);
    lv_obj_align(ccard_multi, LV_ALIGN_CENTER, +125, 0); /* 默认 join 隐藏布局 */
    lv_obj_set_style_bg_color(ccard_multi, lv_color_hex(0x808080), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ccard_multi, LV_OPA_50, LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ccard_multi, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ccard_multi, 5, LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ccard_multi, LV_OPA_COVER, LV_STATE_DEFAULT); // 完全不透明
    lv_obj_set_style_radius(ccard_multi, 10, LV_STATE_DEFAULT);
    lv_obj_add_flag(ccard_multi, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(ccard_multi, ccard_multi_on_click, LV_EVENT_CLICKED, NULL);

    // Icon 图片 多人
    lv_obj_t *multi_icon = NULL;
#ifdef SIMULATOR
    multi_icon = img_create_from_dsc(ccard_multi, img_path(MULTI_ICON, path_buf, sizeof(path_buf)), 96, 82, NULL, &multi_icon_dsc, true);
#else
    multi_icon = lv_img_create(ccard_multi);
    lv_img_set_src(multi_icon, img_path(MULTI_ICON, path_buf, sizeof(path_buf)));
#endif
    lv_obj_align(multi_icon, LV_ALIGN_CENTER, 0, -50);

    // Label 多人
    lv_obj_t *multi_label = lv_label_create(ccard_multi);
    lv_label_set_text(multi_label, "Multi Player");
    lv_obj_align(multi_label, LV_ALIGN_CENTER, 0, 50);
    lv_obj_set_style_text_color(multi_label, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(multi_label, &lv_font_montserrat_24, LV_STATE_DEFAULT);

    // 选项卡加入多人游戏 (根据对端状态显示/隐藏)
    ccard_join = lv_obj_create(mode_root[MODE_CHOOSE]);
    lv_obj_set_size(ccard_join, 250, 400);
    lv_obj_align(ccard_join, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(ccard_join, lv_color_hex(0x808080), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ccard_join, LV_OPA_50, LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ccard_join, lv_color_hex(0x44FF44), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ccard_join, 5, LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ccard_join, LV_OPA_COVER, LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ccard_join, 10, LV_STATE_DEFAULT);
    lv_obj_add_flag(ccard_join, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(ccard_join, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(ccard_join, ccard_join_on_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t *join_label = lv_label_create(ccard_join);
    lv_label_set_text(join_label, "Join Multiplayer");
    lv_obj_align(join_label, LV_ALIGN_CENTER, 0, 50);
    lv_obj_set_style_text_color(join_label, lv_color_hex(0x44FF44), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(join_label, &lv_font_montserrat_22, LV_STATE_DEFAULT);

    lv_obj_t *handshake_img = NULL;
#ifdef SIMULATOR
    handshake_img = img_create_from_dsc(ccard_join, img_path(HAND_SHAKE, path_buf, sizeof(path_buf)), 64, 50, NULL, &handshake_img_dsc, true);
#else
    handshake_img = lv_img_create(ccard_join);
    lv_img_set_src(handshake_img, img_path(HAND_SHAKE, path_buf, sizeof(path_buf)));
#endif
    lv_obj_align(handshake_img, LV_ALIGN_CENTER, 0, -50);

    // 三个输入的图片初始化
#ifdef SIMULATOR
    lv_obj_t *keyboard_img = img_create_from_dsc(dp_lobby, img_path(KEYBOARD_IMG, path_buf, sizeof(path_buf)), 96, 46, NULL, &keyboard_img_dsc, false);
    lv_obj_t *joystick_img = img_create_from_dsc(dp_lobby, img_path(JOYSTICK_IMG, path_buf, sizeof(path_buf)), 64, 41, NULL, &joystick_img_dsc, false);
    lv_obj_t *mcu_img = img_create_from_dsc(dp_lobby, img_path(MCU_IMG, path_buf, sizeof(path_buf)), 76, 35, NULL, &mcu_img_dsc, false);
    dev_imgs[INPUT_DEVICE_LOCAL] = keyboard_img;
    dev_imgs[INPUT_DEVICE_CONTROLLER] = joystick_img;
    dev_imgs[INPUT_DEVICE_REMOTE] = mcu_img;
#else
    lv_obj_t *keyboard_img = lv_img_create(dp_lobby);
    lv_obj_t *joystick_img = lv_img_create(dp_lobby);
    lv_obj_t *mcu_img = lv_img_create(dp_lobby);
    lv_img_set_src(mcu_img, img_path(MCU_IMG, path_buf, sizeof(path_buf)));
    lv_img_set_src(joystick_img, img_path(JOYSTICK_IMG, path_buf, sizeof(path_buf)));
    lv_img_set_src(keyboard_img, img_path(KEYBOARD_IMG, path_buf, sizeof(path_buf)));
    dev_imgs[INPUT_DEVICE_LOCAL] = mcu_img;
    dev_imgs[INPUT_DEVICE_CONTROLLER] = joystick_img;
    dev_imgs[INPUT_DEVICE_REMOTE] = keyboard_img;
#endif

    // 飞机角色选择初始化
    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
    {
        character_img[i] = lv_img_create(dp_lobby);
    }

    // 玩家信息卡片初始化
    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
    {
        info_cards[i] = lv_obj_create(mode_root[MODE_CHOOSE_MULTI]);
        lv_obj_set_size(info_cards[i], 200, 350);                         // 宽200，高350
        lv_obj_align(info_cards[i], LV_ALIGN_CENTER, 270 * i - 270, -20); // 居中显示
        lv_obj_set_style_bg_color(info_cards[i], lv_color_hex(0x808080), LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(info_cards[i], LV_OPA_50, LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(info_cards[i], lv_color_white(), LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(info_cards[i], 5, LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(info_cards[i], LV_OPA_COVER, LV_STATE_DEFAULT); // 完全不透明
        lv_obj_set_style_radius(info_cards[i], 10, LV_STATE_DEFAULT);

        lv_obj_t *right_arrow = NULL;
        lv_obj_t *left_arrow = NULL;
#ifdef SIMULATOR
        right_arrow = img_create_from_dsc(info_cards[i], img_path(RIGHT_ARROW_IMG, path_buf, sizeof(path_buf)), 16, 16, NULL, &right_arrow_img_dsc, true);
        left_arrow = img_create_from_dsc(info_cards[i], img_path(LEFT_ARROW_IMG, path_buf, sizeof(path_buf)), 16, 16, NULL, &left_arrow_img_dsc, true);
#else
        right_arrow = lv_img_create(info_cards[i]);
        left_arrow = lv_img_create(info_cards[i]);
        lv_img_set_src(right_arrow, img_path(RIGHT_ARROW_IMG, path_buf, sizeof(path_buf)));
        lv_img_set_src(left_arrow, img_path(LEFT_ARROW_IMG, path_buf, sizeof(path_buf)));
#endif
        lv_obj_align(right_arrow, LV_ALIGN_CENTER, 70, -80);
        lv_obj_align(left_arrow, LV_ALIGN_CENTER, -70, -80);
    }

    // 单人游戏界面
    // 提示标签
    lv_obj_t *single_info_label = lv_label_create(mode_root[MODE_CHOOSE_SINGLE]);
    lv_label_set_text(single_info_label, "Press A on the device you want to use.Press A again to start,B to cancel.");
    lv_obj_align(single_info_label, LV_ALIGN_CENTER, 0, 200);
    lv_obj_set_style_text_color(single_info_label, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(single_info_label, &lv_font_montserrat_14, LV_STATE_DEFAULT);
    // 左侧 显示设备图标
    // 右侧 显示选择的飞机

    // 多人游戏界面
    // 提示标签
    lv_obj_t *multi_info_label = lv_label_create(mode_root[MODE_CHOOSE_MULTI]);
    lv_label_set_text(multi_info_label, "Press A on the device to join.The host press A again to start,B to cancel.");
    lv_obj_align(multi_info_label, LV_ALIGN_CENTER, 0, 200);
    lv_obj_set_style_text_color(multi_info_label, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(multi_info_label, &lv_font_montserrat_14, LV_STATE_DEFAULT);
    // 左侧 显示设备图标
    // 右侧 显示选择的飞机

    // 按键注册
    input_sw_register_press_callback(INPUT_DEVICE_ANY, KEY_EVENT_A, ui_lobby_A_pressed_handler, NULL);
}

/**
 * @brief 运行lobby界面
 */
void ui_lobby_run(void)
{
    for (int i = 0; i < MODE_CHOOSE_MULTI + 1; i++)
    {
        lv_obj_add_flag(mode_root[i], LV_OBJ_FLAG_HIDDEN);
    }
    // 初始化时隐藏所有输入设备的图片
    for (int i = 0; i < INPUT_DEVICE_COUNT; i++)
    {
        lv_obj_add_flag(dev_imgs[i], LV_OBJ_FLAG_HIDDEN);
    }
    // 隐藏所有选择的飞机图标
    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
    {
        lv_obj_add_flag(character_img[i], LV_OBJ_FLAG_HIDDEN);
    }
    // 隐藏info cards
    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
    {
        lv_obj_add_flag(info_cards[i], LV_OBJ_FLAG_HIDDEN);
    }
    fetch_unlocked_characters();
    CONSOLE_DEBUG("About to load lobby screen");
    lv_scr_load(dp_lobby);
    set_group(NULL);
    lv_obj_clear_flag(mode_root[MODE_CHOOSE], LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_border_color(ccard_single, lv_palette_main(LV_PALETTE_BLUE), LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ccard_multi, lv_color_white(), LV_STATE_DEFAULT);
    memset(&lobby, 0, sizeof(lobby));
    lobby.state = MODE_CHOOSE;
}

/**
 * @brief 处理lobby界面的esc按键事件
 */
void ui_lobby_esc_behave(input_device_type_t device_type)
{
    if (fsm_get_state() != GS_LOBBY)
        return;
    if (lobby.state == MODE_CHOOSE)
    {
        fsm_switch_state(GS_MENU);
        return;
    }

    if (lobby.state == MODE_CHOOSE_SINGLE)
    {
        if (lobby.slots[0].ready)
        {
            lobby.slots[0].ready = false;
            lobby.ui_dirty = true;
        }
        else
        {
            lobby.state = MODE_CHOOSE;
            lobby.ui_dirty = true;
        }
        return;
    }

    if (lobby.state == MODE_CHOOSE_MULTI)
    {
        bool ready_canceled = false;
        for (int i = 0; i < MAX_PLAYER_COUNT; i++)
        {
            if (lobby.slots[i].ready == true && lobby.slots[i].dev == input_device_get(device_type))
            {
                lobby.slots[i].ready = false;
                ready_canceled = true;
                lobby.ui_dirty = true;
            }
        }
        // 更新lobby顺序
        for (int i = 0; i < MAX_PLAYER_COUNT; i++)
        {
            if (!ready_canceled)
                break;
            if (lobby.slots[i].ready == true)
                continue;
            // 如果这个lobby没有被占用，就将它后面的占用lobby移动到第它的位置
            for (int j = i + 1; j < MAX_PLAYER_COUNT; j++)
            {
                if (lobby.slots[j].ready == true)
                {
                    lobby.slots[i] = lobby.slots[j];
                    lobby.slots[j].ready = false;
                    lobby.ui_dirty = true;
                    break;
                }
            }
        }
        if (ready_canceled)
        {
            return;
        }
        if (device_type == INPUT_DEVICE_LOCAL || device_type == INPUT_DEVICE_CONTROLLER)
        {
            lobby.state = MODE_CHOOSE;
            lv_obj_clear_flag(mode_root[MODE_CHOOSE], LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(mode_root[MODE_CHOOSE_MULTI], LV_OBJ_FLAG_HIDDEN);
            lobby.navigate_index[0] = 0;
            lobby.ui_dirty = true;
            comm_send_lobby_state(0); /* 通知对端: 离开多人模式 */
            /* #6: 清理所有 REMOTE slots */
            for (int i = 0; i < MAX_PLAYER_COUNT; i++) {
                if (lobby.slots[i].dev && lobby.slots[i].dev->type == INPUT_DEVICE_REMOTE)
                    lobby.slots[i].ready = false;
            }
        }
        else if (device_type == INPUT_DEVICE_REMOTE)
        {
            // REMOTE退出ui_comm
        }
        return;
    }
}

/**
 * @brief 刷新lobby界面的元素
 */
void ui_lobby_flush(void)
{
    /* ---- 每帧检查 (仅 GS_LOBBY / GS_COMM, 不受 ui_dirty 限制) ---- */

    game_state_t gs = fsm_get_state();

    /* #2: 从机在 GS_COMM 中检测主机游戏结束 */
    if (gs == GS_COMM && comm_get_lobby_state() == 0)
    {
        CONSOLE_INFO("Host ended game, leaving GS_COMM.");
        fsm_switch_state(GS_LOBBY);
        memset(&lobby, 0, sizeof(lobby));
        return;
    }

    if (gs == GS_LOBBY)
    {
        /* #1: comm 断开 → 回 lobby */
        static bool s_was_connected = false;
        bool connected = (comm_get_status() == COMM_STATUS_CONNECTED);
        if (s_was_connected && !connected)
        {
            CONSOLE_INFO("Comm lost, returning to lobby.");
            memset(&lobby, 0, sizeof(lobby));
            lobby.state = MODE_CHOOSE;
            lobby.ui_dirty = true;
        }
        s_was_connected = connected;

        /* #3+4: join 可见性跟随对端状态 (仅变化时更新) */
        if (lobby.state == MODE_CHOOSE)
        {
            static uint8_t s_last_hosting = 0xFF;
            uint8_t hosting = comm_get_lobby_state();
            if (hosting != s_last_hosting)
            {
                s_last_hosting = hosting;
                if (hosting)
                    lv_obj_clear_flag(ccard_join, LV_OBJ_FLAG_HIDDEN);
                else
                    lv_obj_add_flag(ccard_join, LV_OBJ_FLAG_HIDDEN);
                lobby.ui_dirty = true; /* 触发布局刷新 */
            }
        }

    }

    /* ---- ui_dirty 门控的 UI 刷新 (仅 GS_LOBBY) ---- */
    if (gs != GS_LOBBY || !lobby.ui_dirty)
        return;
    lobby.ui_dirty = false;

    switch (lobby.state)
    {
    case MODE_CHOOSE:
    {
        uint8_t remote_hosting = comm_get_lobby_state();

        /* #5: 动态排版 */
        lv_obj_set_size(ccard_single, 250, 400);
        lv_obj_set_size(ccard_multi, 250, 400);
        lv_obj_set_size(ccard_join, 250, 400);
        if (remote_hosting)
        {
            lv_obj_align(ccard_single, LV_ALIGN_CENTER, -250, 0);
            lv_obj_align(ccard_join, LV_ALIGN_CENTER, 0, 0);
            lv_obj_align(ccard_multi, LV_ALIGN_CENTER, +250, 0);
        }
        else
        {
            lv_obj_align(ccard_single, LV_ALIGN_CENTER, -125, 0);
            lv_obj_align(ccard_multi, LV_ALIGN_CENTER, +125, 0);
        }

        int idx = lobby.navigate_index[0];
        lv_obj_set_style_border_color(ccard_single, lv_color_white(), LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(ccard_multi, lv_color_white(), LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(ccard_join, lv_color_white(), LV_STATE_DEFAULT);
        if (idx == 0)
            lv_obj_set_style_border_color(ccard_single, lv_palette_main(LV_PALETTE_BLUE), LV_STATE_DEFAULT);
        else if (idx == 1 && remote_hosting)
            lv_obj_set_style_border_color(ccard_join, lv_palette_main(LV_PALETTE_BLUE), LV_STATE_DEFAULT);
        else
            lv_obj_set_style_border_color(ccard_multi, lv_palette_main(LV_PALETTE_BLUE), LV_STATE_DEFAULT);
        break;
    }
    case MODE_CHOOSE_SINGLE:
        // 显示选择的飞机图标
        lv_obj_clear_flag(character_img[0], LV_OBJ_FLAG_HIDDEN);
        lv_img_set_src(character_img[0], &apr_get(character_get_config(ui_base_get_selected_character_id())->apr_id)->img_dsc);
        lv_obj_align(character_img[0], LV_ALIGN_CENTER, 100, 20);
        if (lobby.slots[0].ready)
        {
            lv_obj_clear_flag(dev_imgs[(int)(lobby.slots[0].dev->type)], LV_OBJ_FLAG_HIDDEN);
            lv_obj_align(dev_imgs[(int)(lobby.slots[0].dev->type)], LV_ALIGN_CENTER, -100, 20);
        }
        else
        {
            for (int i = 0; i < INPUT_DEVICE_COUNT; i++)
            {
                lv_obj_add_flag(dev_imgs[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        break;
    case MODE_CHOOSE_MULTI:
        for (int i = 0; i < MAX_PLAYER_COUNT; i++)
        {
            // 先隐藏所有元素
            lv_obj_add_flag(info_cards[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(dev_imgs[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(character_img[i], LV_OBJ_FLAG_HIDDEN);
        }
        CONSOLE_DEBUG("Elements Hidden");
        // 显示占用的元素
        for (int i = 0; i < MAX_PLAYER_COUNT; i++)
        {
            if (lobby.slots[i].ready)
            {
                int dev_id = (int)(lobby.slots[i].dev->type);
                lv_obj_clear_flag(info_cards[i], LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(dev_imgs[dev_id], LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(character_img[i], LV_OBJ_FLAG_HIDDEN);
                lv_img_set_src(character_img[i], &apr_get(character_get_config(lobby.slots[i].character_id)->apr_id)->img_dsc);
                lv_obj_align(character_img[i], LV_ALIGN_CENTER, -270 + i * 270, -100);
                lv_obj_align(dev_imgs[dev_id], LV_ALIGN_CENTER, -270 + i * 270, 100);
            }
        }
        break;
    default:
        break;
    }
}

/**
 * @brief 处理lobby界面的导航事件
 */
void ui_lobby_navigate(void)
{
    static int last_dir[MAX_PLAYER_COUNT] = {0};

    int x = 0;
    int dir = 0;
    // 只有上次dir为0才能继续输入新的dir
    // ==========================================
    // 1. MODE_CHOOSE: 选择单人/多人
    // ==========================================
    if (lobby.state == MODE_CHOOSE)
    {
        x = LOCAL->x();
        dir = 0;
        if (x > JS_THRESHOLD)
            dir = 1;
        else if (x < -JS_THRESHOLD)
            dir = -1;
        if (last_dir[0] != 0 && dir != 0)
            return;
        last_dir[0] = dir;

        int current = lobby.navigate_index[0];
        int max_idx = (comm_get_lobby_state() != 0) ? 2 : 1; // +Join 选项卡
        if (current > max_idx)
            current = 0;
        int new_index = (current + dir + (max_idx + 1)) % (max_idx + 1);
        if (new_index != current)
        {
            lobby.navigate_index[0] = new_index;
            lobby.ui_dirty = true;
        }
        return;
    }

    // ==========================================
    // 2. MODE_CHOOSE_SINGLE: 单人模式选角色
    // ==========================================
    if (lobby.state == MODE_CHOOSE_SINGLE)
    {
    }

    // ==========================================
    // 3. MODE_CHOOSE_MULTI: 多人模式选角色
    // ==========================================
    if (lobby.state == MODE_CHOOSE_MULTI)
    {
        for (int i = 0; i < MAX_PLAYER_COUNT; i++)
        {
            if (lobby.slots[i].ready == false)
                continue;
            x = lobby.slots[i].dev->x();
            dir = 0;
            if (x > JS_THRESHOLD)
                dir = 1;
            else if (x < -JS_THRESHOLD)
                dir = -1;
            if (last_dir[i] != 0 && dir != 0)
                continue;
            last_dir[i] = dir;

            // 使用LOCAL的角色数据
            int current = lobby.navigate_index[i];
            int max_idx = unlockeds_count - 1;
            int new_index = (current + dir + (max_idx + 1)) % (max_idx + 1);
            if (new_index != current)
            {
                lobby.navigate_index[i] = new_index;
                lobby.slots[i].character_id = unlockeds[new_index];
                lobby.ui_dirty = true;
            }
        }
        return;
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief 初始化unlockeds和unlockeds_count
 */
static void fetch_unlocked_characters()
{
    unlockeds_count = 0;
    memset(unlockeds, 0, sizeof(unlockeds));
    for (int i = 0; i < CHARACTER_ID_MAX; i++)
    {
        if (ui_base_character_is_unlocked(i))
        {
            unlockeds[unlockeds_count++] = i;
        }
    }
}

/**
 * @brief 处理单人游戏选项卡点击事件/选中事件
 */
static void ccard_single_on_click(lv_event_t *e)
{
    LV_UNUSED(e);
    lobby.state = MODE_CHOOSE_SINGLE;
    lv_obj_clear_flag(mode_root[MODE_CHOOSE_SINGLE], LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(mode_root[MODE_CHOOSE], LV_OBJ_FLAG_HIDDEN);
    lobby.ui_dirty = true;
}

/**
 * @brief 处理多人游戏选项卡点击事件/选中事件
 */
static void ccard_multi_on_click(lv_event_t *e)
{
    LV_UNUSED(e);
    lobby.state = MODE_CHOOSE_MULTI;
    lv_obj_clear_flag(mode_root[MODE_CHOOSE_MULTI], LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(mode_root[MODE_CHOOSE], LV_OBJ_FLAG_HIDDEN);
    lobby.ui_dirty = true;
    comm_send_lobby_state(1); /* 广播: 本端正在开房 */
    CONSOLE_DEBUG("Multi Player Room Entered");
}

/**
 * @brief 加入多人游戏选项卡点击 — 切换到通信界面
 */
static void ccard_join_on_click(lv_event_t *e)
{
    LV_UNUSED(e);
    fsm_switch_state(GS_COMM);
    CONSOLE_DEBUG("Join Multiplayer -> GS_COMM");
}

/**
 * @brief 处理A键按下事件
 * @param e 按键事件结构体指针
 */
static void ui_lobby_A_pressed_handler(input_event_t *e)
{
    if (fsm_get_state() != GS_LOBBY)
        return;
    input_device_type_t dev_type = e->dev_type;

    if (lobby.state == MODE_CHOOSE)
    {
        if (dev_type != INPUT_DEVICE_LOCAL)
            return;
        int idx = lobby.navigate_index[0];
        if (idx == 0)
            ccard_single_on_click(NULL);
        else if (idx == 1 && comm_get_lobby_state() != 0)
            ccard_join_on_click(NULL); /* Join → GS_COMM */
        else
            ccard_multi_on_click(NULL);
        return; /* 防止穿透 */
    }

    if (lobby.state == MODE_CHOOSE_SINGLE)
    {
        if (lobby.slots[0].ready == false)
        {
            lobby.slots[0].dev = input_device_get(dev_type);
            lobby.slots[0].ready = true;
            lobby.slots[0].character_id = ui_base_get_selected_character_id(); // 之后可以改为navigate
            lobby.ui_dirty = true;
        }
        else // 游戏开始
        {
            behave_t behave = {
                .f = player_control,
                .usr_data = lobby.slots[0].dev,
            };
            fsm_switch_state(GS_PLAY);
            game_obj_t *p = player_spawn(512, 400, lobby.slots[0].character_id, behave);
            CONSOLE_DEBUG("Single Player Game Start");
            CONSOLE_DEBUG("player_spawn: %p", p);
            event_dispatch(EVENT_GAME_START, NULL, NULL);
        }
        return;
    }

    if (lobby.state == MODE_CHOOSE_MULTI)
    {
        CONSOLE_DEBUG("A on dev:%d pressed.", e->dev_type);
        bool found = false;
        for (int i = 0; i < MAX_PLAYER_COUNT; i++)
        {
            if (lobby.slots[i].ready == false)
                continue;
            if (lobby.slots[i].dev == input_device_get(dev_type))
                found = true;
        }
        CONSOLE_DEBUG("found: %d", found);
        if (!found)
        {
            for (int i = 0; i < MAX_PLAYER_COUNT; i++)
            {
                if (lobby.slots[i].ready == false)
                {
                    lobby.slots[i].dev = input_device_get(dev_type);
                    lobby.slots[i].ready = true;
                    lobby.slots[i].character_id = PLAYER;
                    lobby.ui_dirty = true;
                    CONSOLE_DEBUG("player %d registered,dev:%d", i, e->dev_type);
                    break;
                }
            }
            return; /* 首次注册设备, 不启动游戏 */
        }
        /* 已注册设备再次按 A → 启动游戏 */
        if (lobby.slots[0].ready == true && lobby.slots[0].dev == input_device_get(e->dev_type))
        {
            // 游戏开始
            fsm_switch_state(GS_PLAY);
            for (int i = 0; i < MAX_PLAYER_COUNT; i++)
            {
                if (lobby.slots[i].ready == false)
                    continue;
                behave_t behave = {
                    .f = player_control,
                    .usr_data = lobby.slots[i].dev,
                };
                game_obj_t *p = player_spawn(412 + i * 100, 400, lobby.slots[i].character_id, behave);
                CONSOLE_DEBUG("player_spawn: %p", p);
            }
            CONSOLE_DEBUG("Multi Player Game Start");
            event_dispatch(EVENT_GAME_START, NULL, NULL);
        }
        return;
    }
}
