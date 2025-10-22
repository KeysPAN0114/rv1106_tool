#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <string.h>

#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 320

// RGB565 颜色定义
#define RGB565_BLACK   0x0000
#define RGB565_WHITE   0xFFFF
#define RGB565_RED     0xF800
#define RGB565_GREEN   0x07E0
#define RGB565_BLUE    0x001F
#define RGB565_YELLOW  0xFFE0
#define RGB565_MAGENTA 0xF81F
#define RGB565_CYAN    0x07FF

// BGR565 颜色定义（备用）
#define BGR565_RED     0x001F
#define BGR565_GREEN   0x07E0
#define BGR565_BLUE    0xF800
#define BGR565_MAGENTA 0xF81F

struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
char *fbp = 0;
long screensize = 0;

void print_pixel_format() {
    printf("=== Pixel Format Analysis ===\n");
    printf("Bits per pixel: %d\n", vinfo.bits_per_pixel);
    printf("Red:   offset=%d, length=%d, msb_right=%d\n", 
           vinfo.red.offset, vinfo.red.length, vinfo.red.msb_right);
    printf("Green: offset=%d, length=%d, msb_right=%d\n", 
           vinfo.green.offset, vinfo.green.length, vinfo.green.msb_right);
    printf("Blue:  offset=%d, length=%d, msb_right=%d\n", 
           vinfo.blue.offset, vinfo.blue.length, vinfo.blue.msb_right);
    printf("=============================\n");
}

u_int16_t rgb_to_bgr(u_int16_t rgb_color) {
    u_int16_t r = (rgb_color & 0xF800) >> 11;
    u_int16_t g = (rgb_color & 0x07E0) >> 5;
    u_int16_t b = (rgb_color & 0x001F);
    return (b << 11) | (g << 5) | r;
}

void draw_pixel(int x, int y, u_int16_t color, int use_bgr) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
    
    long location = (x + vinfo.xoffset) + (y + vinfo.yoffset) * finfo.line_length / 2;
    u_int16_t final_color = use_bgr ? rgb_to_bgr(color) : color;
    
    *((u_int16_t*)(fbp + location * 2)) = final_color;
}

void fill_screen(u_int16_t color, int use_bgr) {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            draw_pixel(x, y, color, use_bgr);
        }
    }
}

void test_colors(int use_bgr) {
    printf("Testing colors (mode: %s)\n", use_bgr ? "BGR" : "RGB");
    
    int strip_width = SCREEN_WIDTH / 4;
    
    // 测试红色
    printf("Red (0x%04X)...\n", RGB565_RED);
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < strip_width; x++) {
            draw_pixel(x, y, RGB565_RED, use_bgr);
        }
    }
    sleep(2);
    
    // 测试绿色
    printf("Green (0x%04X)...\n", RGB565_GREEN);
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = strip_width; x < strip_width*2; x++) {
            draw_pixel(x, y, RGB565_GREEN, use_bgr);
        }
    }
    sleep(2);
    
    // 测试蓝色
    printf("Blue (0x%04X)...\n", RGB565_BLUE);
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = strip_width*2; x < strip_width*3; x++) {
            draw_pixel(x, y, RGB565_BLUE, use_bgr);
        }
    }
    sleep(2);
    
    // 测试品红色
    printf("Magenta (0x%04X)...\n", RGB565_MAGENTA);
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = strip_width*3; x < SCREEN_WIDTH; x++) {
            draw_pixel(x, y, RGB565_MAGENTA, use_bgr);
        }
    }
    sleep(2);
}

int main() {
    int fb_fd = open("/dev/fb0", O_RDWR);
    if (fb_fd == -1) {
        perror("Cannot open framebuffer");
        return 1;
    }

    if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
        perror("Error reading variable info");
        close(fb_fd);
        return 1;
    }

    if (ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo) == -1) {
        perror("Error reading fixed info");
        close(fb_fd);
        return 1;
    }

    // 打印像素格式信息
    print_pixel_format();

    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
    fbp = mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    if ((int)fbp == -1) {
        perror("Failed to map framebuffer");
        close(fb_fd);
        return 1;
    }

    printf("Starting color tests...\n");
    
    // 先测试RGB模式
    printf("\n=== Testing RGB mode ===\n");
    test_colors(0);
    
    // 清屏
    fill_screen(RGB565_BLACK, 0);
    sleep(1);
    
    // 然后测试BGR模式
    printf("\n=== Testing BGR mode ===\n");
    test_colors(1);
    
    // 清屏结束
    fill_screen(RGB565_BLACK, 0);
    
    munmap(fbp, screensize);
    close(fb_fd);
    return 0;
}