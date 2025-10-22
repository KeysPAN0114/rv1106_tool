#include "lvgl/lvgl.h"
#include "lv_demos/lv_demo.h"
#include "lv_drivers/display/fbdev.h"
// #include "lv_drivers/indev/evdev.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#define DISP_BUF_SIZE (240 * 320)

// 全局变量，用于存储三个色块对象
static lv_obj_t * red_obj;
static lv_obj_t * green_obj;
static lv_obj_t * blue_obj;

// 当前显示的颜色索引 (0: 红, 1: 绿, 2: 蓝)
static uint8_t current_color_index = 0;

// 定时器回调函数
static void color_switch_timer_cb(lv_timer_t * timer) {
    // 隐藏所有色块
    lv_obj_add_flag(red_obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(green_obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(blue_obj, LV_OBJ_FLAG_HIDDEN);

    // 根据当前索引显示对应的色块
    switch (current_color_index) {
        case 0:
            lv_obj_clear_flag(red_obj, LV_OBJ_FLAG_HIDDEN);
            printf("LVGL: Displaying RED\n");
            break;
        case 1:
            lv_obj_clear_flag(green_obj, LV_OBJ_FLAG_HIDDEN);
            printf("LVGL: Displaying GREEN\n");
            break;
        case 2:
            lv_obj_clear_flag(blue_obj, LV_OBJ_FLAG_HIDDEN);
            printf("LVGL: Displaying BLUE\n");
            break;
    }

    // 更新索引，准备下一次切换
    current_color_index = (current_color_index + 1) % 3;
}

// 创建UI的函数
void create_lvgl_color_cycle_ui(void) {
    // 获取当前活动的屏幕
    lv_obj_t * scr = lv_scr_act();

    // 创建三个全屏的色块对象
    red_obj = lv_obj_create(scr);
    green_obj = lv_obj_create(scr);
    blue_obj = lv_obj_create(scr);

    // 设置它们的大小为屏幕大小
    lv_obj_set_size(red_obj, LV_PCT(100), LV_PCT(100));
    lv_obj_set_size(green_obj, LV_PCT(100), LV_PCT(100));
    lv_obj_set_size(blue_obj, LV_PCT(100), LV_PCT(100));

    // 设置它们的背景颜色
    lv_obj_set_style_bg_color(red_obj, lv_color_hex(0xF80000), 0);
    lv_obj_set_style_bg_color(green_obj, lv_color_hex(0x00ff00), 0);
    lv_obj_set_style_bg_color(blue_obj, lv_color_hex(0x0000FF), 0);

    // 移除所有装饰（边框等）
    lv_obj_set_style_border_width(red_obj, 0, 0);
    lv_obj_set_style_border_width(green_obj, 0, 0);
    lv_obj_set_style_border_width(blue_obj, 0, 0);
    lv_obj_set_style_pad_all(red_obj, 0, 0);
    lv_obj_set_style_pad_all(green_obj, 0, 0);
    lv_obj_set_style_pad_all(blue_obj, 0, 0);

    // 初始状态下，先隐藏所有色块
    lv_obj_add_flag(red_obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(green_obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(blue_obj, LV_OBJ_FLAG_HIDDEN);

    // 创建一个LVGL定时器，每1000毫秒（1秒）调用一次回调函数
    lv_timer_create(color_switch_timer_cb, 2000, NULL);
    
    // 手动触发一次回调，让程序启动时立即显示第一个颜色（红色）
    color_switch_timer_cb(NULL);
}

int main(void)
{
    /*LittlevGL init*/
    lv_init();

    /*Linux frame buffer device init*/
    fbdev_init();

    /*A small buffer for LittlevGL to draw the screen's content*/
    static lv_color_t buf[DISP_BUF_SIZE];

    /*Initialize a descriptor for the buffer*/
    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, DISP_BUF_SIZE);

    /*Initialize and register a display driver*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf   = &disp_buf;
    disp_drv.flush_cb   = fbdev_flush;
    disp_drv.hor_res    = 240;
    disp_drv.ver_res    = 320;
    lv_disp_drv_register(&disp_drv);

    // evdev_init();
    // static lv_indev_drv_t indev_drv_1;
    // lv_indev_drv_init(&indev_drv_1); /*Basic initialization*/
    // indev_drv_1.type = LV_INDEV_TYPE_POINTER;

    /*This function will be called periodically (by the library) to get the mouse position and state*/
    // indev_drv_1.read_cb = evdev_read;
    // lv_indev_t *mouse_indev = lv_indev_drv_register(&indev_drv_1);


    /*Set a cursor for the mouse*/
    // LV_IMG_DECLARE(mouse_cursor_icon)
    // lv_obj_t * cursor_obj = lv_img_create(lv_scr_act()); /*Create an image object for the cursor */
    // lv_img_set_src(cursor_obj, &mouse_cursor_icon);           /*Set the image source*/
    // lv_indev_set_cursor(mouse_indev, cursor_obj);             /*Connect the image  object to the driver*/


    /*Create a Demo*/
    lv_demo_widgets();
    
    // 创建我们的循环颜色UI
    // create_lvgl_color_cycle_ui();

    /*Handle LitlevGL tasks (tickless mode)*/
    while(1) {
        lv_timer_handler();
        usleep(5000);
    }

    return 0;
}

/*Set in lv_conf.h as `LV_TICK_CUSTOM_SYS_TIME_EXPR`*/
uint32_t custom_tick_get(void)
{
    static uint64_t start_ms = 0;
    if(start_ms == 0) {
        struct timeval tv_start;
        gettimeofday(&tv_start, NULL);
        start_ms = (tv_start.tv_sec * 1000000 + tv_start.tv_usec) / 1000;
    }

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    uint64_t now_ms;
    now_ms = (tv_now.tv_sec * 1000000 + tv_now.tv_usec) / 1000;

    uint32_t time_ms = now_ms - start_ms;
    return time_ms;
}
