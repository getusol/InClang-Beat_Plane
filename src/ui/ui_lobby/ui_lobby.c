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

/**********************
 *      MACROS
 **********************/

#define SINGLE_ICON "2DSingleIcon.bin" // 77 * 96
#define MULTI_ICON "2DMultiIcon.bin"   // 96 * 82
#define KEYBOARD_IMG "keyboard.bin"    // 96 * 46
#define JOYSTICK_IMG "joystick.bin"    // 64 * 41
#define MCU_IMG "MCU.bin"              // 76 * 35

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

// event callbacks
static void ccard_single_on_click(lv_event_t *e);
static void ccard_multi_on_click(lv_event_t *e);

// 按键回调
static void ui_lobby_A_pressed_handler(input_event_t *e);

/**********************
 *  STATIC VARIABLES
 **********************/

static lobby_t lobby;

static lv_obj_t *dp_lobby = NULL;
static lv_obj_t *mode_root[MODE_CNT] = {NULL};
static lv_obj_t *ccard_single = NULL;
static lv_obj_t *ccard_multi = NULL;
static lv_obj_t *dev_imgs[INPUT_DEVICE_COUNT] = {NULL};

static lv_obj_t *character_img[MAX_PLAYER_COUNT] = {NULL}; // 显示选择的飞机图标，支持3个设备 3个玩家

// imgs
#ifdef SIMULATOR
static lv_img_dsc_t single_icon_dsc;
static lv_img_dsc_t multi_icon_dsc;
static lv_img_dsc_t keyboard_img_dsc;
static lv_img_dsc_t mcu_img_dsc;
static lv_img_dsc_t joystick_img_dsc;
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
    lv_obj_set_size(ccard_single, 300, 400);              // 宽300，高400
    lv_obj_align(ccard_single, LV_ALIGN_CENTER, -200, 0); // 居中显示
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
    lv_obj_set_size(ccard_multi, 300, 400);             // 宽300，高400
    lv_obj_align(ccard_multi, LV_ALIGN_CENTER, 200, 0); // 居中显示
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

    // 单人游戏界面
    // 提示标签
    lv_obj_t *single_info_label = lv_label_create(mode_root[MODE_CHOOSE_SINGLE]);
    lv_label_set_text(single_info_label, "Press A on the device you want to use.Press A again to start,B to cancel.");
    lv_obj_align(single_info_label, LV_ALIGN_CENTER, 0, 200);
    lv_obj_set_style_text_color(single_info_label, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(single_info_label, &lv_font_montserrat_14, LV_STATE_DEFAULT);
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
void ui_lobby_esc_behave(void)
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
}

/**
 * @brief 刷新lobby界面的元素
 */
void ui_lobby_flush(void)
{
    if (lobby.ui_dirty == false)
        return;
    switch (lobby.state)
    {
    case MODE_CHOOSE:
        if (lobby.navigate_index[0] == 0)
        {
            lv_obj_set_style_border_color(ccard_single, lv_palette_main(LV_PALETTE_BLUE), LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ccard_multi, lv_color_white(), LV_STATE_DEFAULT);
        }
        else
        {
            lv_obj_set_style_border_color(ccard_single, lv_color_white(), LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ccard_multi, lv_palette_main(LV_PALETTE_BLUE), LV_STATE_DEFAULT);
        }
        break;
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
    // 只有上次dir为0才能继续输入新的dir
    // ==========================================
    // 1. MODE_CHOOSE: 选择单人/多人
    // ==========================================
    if (lobby.state == MODE_CHOOSE)
    {
        int x = LOCAL->x();
        int dir = 0;
        if (x > JS_THRESHOLD)
            dir = 1;
        else if (x < -JS_THRESHOLD)
            dir = -1;
        if (last_dir[0] != 0 && dir != 0)
            return;
        last_dir[0] = dir;

        int current = lobby.navigate_index[0];
        int max_idx = 1; // 0: 单人, 1: 多人
        int new_index = (current + dir + (max_idx + 1)) % (max_idx + 1);
        if (new_index != current)
        {
            lobby.navigate_index[0] = new_index;
            lobby.ui_dirty = true;
        }
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
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

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
        if (lobby.navigate_index[0] == 0)
            ccard_single_on_click(NULL);
        else
            ccard_multi_on_click(NULL);
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
            CONSOLE_DEBUG("player_spawn: %p", p);
            event_dispatch(EVENT_GAME_START, NULL, NULL);
        }
    }
}
