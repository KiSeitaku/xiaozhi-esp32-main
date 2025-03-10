// 导入相关的头文件
#include "lcd_display.h"
#include <vector>
#include <font_awesome_symbols.h>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_lvgl_port.h>
#include "assets/lang_config.h"
#include "board.h"
// 定义标签
#define TAG "LcdDisplay"

// 声明字体
LV_FONT_DECLARE(font_awesome_30_4);

// SpiLcdDisplay类：用于控制SPI LCD显示器
SpiLcdDisplay::SpiLcdDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel,
                             int width, int height, int offset_x, int offset_y, bool mirror_x, bool mirror_y, bool swap_xy,
                             DisplayFonts fonts)
    : LcdDisplay(panel_io, panel, fonts) // 调用父类构造函数
{
    width_ = width;
    height_ = height;

    // 初始化显示器为黑色
    std::vector<uint16_t> buffer(width_, 0x0000); // 创建一个全黑的像素缓冲区
    for (int y = 0; y < height_; y++)
    {
        esp_lcd_panel_draw_bitmap(panel_, 0, y, width_, y + 1, buffer.data()); // 在每一行绘制黑色
    }

    // 启动显示器
    ESP_LOGI(TAG, "Turning display on");
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_, true));

    // 初始化LVGL库
    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();

    // 初始化LVGL端口配置
    ESP_LOGI(TAG, "Initialize LVGL port");
    lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    port_cfg.task_priority = 1;
    lvgl_port_init(&port_cfg);

    // 添加LCD显示屏
    ESP_LOGI(TAG, "Adding LCD screen");
    const lvgl_port_display_cfg_t display_cfg = {
        .io_handle = panel_io_,
        .panel_handle = panel_,
        .buffer_size = static_cast<uint32_t>(width_ * 10), // 缓冲区大小
        .double_buffer = false,                            // 禁用双缓冲
        .trans_size = 0,
        .hres = static_cast<uint32_t>(width_),
        .vres = static_cast<uint32_t>(height_),
        .monochrome = false,
        .rotation = {
            .swap_xy = swap_xy,   // 交换XY轴
            .mirror_x = mirror_x, // 水平镜像
            .mirror_y = mirror_y, // 垂直镜像
        },
        .color_format = LV_COLOR_FORMAT_RGB565, // 颜色格式
        .flags = {
            .buff_dma = 1,
            .buff_spiram = 0,
            .sw_rotate = 0,
            .swap_bytes = 1,
            .full_refresh = 0,
            .direct_mode = 0,
        },
    };

    // 添加显示设备
    display_ = lvgl_port_add_disp(&display_cfg);
    if (display_ == nullptr)
    {
        ESP_LOGE(TAG, "Failed to add display");
        return;
    }

    // 设置显示偏移
    if (offset_x != 0 || offset_y != 0)
    {
        lv_display_set_offset(display_, offset_x, offset_y);
    }

    // 设置UI
    SetupUI();
}

// RGB LCD显示类：用于控制RGB LCD显示器
RgbLcdDisplay::RgbLcdDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel,
                             int width, int height, int offset_x, int offset_y,
                             bool mirror_x, bool mirror_y, bool swap_xy,
                             DisplayFonts fonts)
    : LcdDisplay(panel_io, panel, fonts) // 调用父类构造函数
{
    width_ = width;
    height_ = height;

    // 初始化显示器为白色
    std::vector<uint16_t> buffer(width_, 0x0000); // 创建一个全白的像素缓冲区
    for (int y = 0; y < height_; y++)
    {
        esp_lcd_panel_draw_bitmap(panel_, 0, y, width_, y + 1, buffer.data()); // 在每一行绘制白色
    }

    // 初始化LVGL库
    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();

    // 初始化LVGL端口配置
    ESP_LOGI(TAG, "Initialize LVGL port");
    lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    port_cfg.task_priority = 1;
    lvgl_port_init(&port_cfg);

    // 添加RGB LCD显示屏
    ESP_LOGI(TAG, "Adding LCD screen");
    const lvgl_port_display_cfg_t display_cfg = {
        .io_handle = panel_io_,
        .panel_handle = panel_,
        .buffer_size = static_cast<uint32_t>(width_ * 10), // 缓冲区大小
        .double_buffer = true,                             // 启用双缓冲
        .hres = static_cast<uint32_t>(width_),
        .vres = static_cast<uint32_t>(height_),
        .rotation = {
            .swap_xy = swap_xy,   // 交换XY轴
            .mirror_x = mirror_x, // 水平镜像
            .mirror_y = mirror_y, // 垂直镜像
        },
        .flags = {
            .buff_dma = 1,
            .swap_bytes = 0,
            .full_refresh = 1,
            .direct_mode = 1,
        },
    };

    const lvgl_port_display_rgb_cfg_t rgb_cfg = {
        .flags = {
            .bb_mode = true,       // 启用边带模式
            .avoid_tearing = true, // 避免撕裂
        }};

    // 添加显示设备
    display_ = lvgl_port_add_disp_rgb(&display_cfg, &rgb_cfg);
    if (display_ == nullptr)
    {
        ESP_LOGE(TAG, "Failed to add RGB display");
        return;
    }

    // 设置显示偏移
    if (offset_x != 0 || offset_y != 0)
    {
        lv_display_set_offset(display_, offset_x, offset_y);
    }

    // 设置UI
    SetupUI();
}

// 析构函数，清理资源
LcdDisplay::~LcdDisplay()
{
    // 删除所有UI对象
    if (content_ != nullptr)
    {
        lv_obj_del(content_);
    }
    if (status_bar_ != nullptr)
    {
        lv_obj_del(status_bar_);
    }
    if (side_bar_ != nullptr)
    {
        lv_obj_del(side_bar_);
    }
    if (container_ != nullptr)
    {
        lv_obj_del(container_);
    }
    if (display_ != nullptr)
    {
        lv_display_delete(display_);
    }

    // 删除显示面板
    if (panel_ != nullptr)
    {
        esp_lcd_panel_del(panel_);
    }
    if (panel_io_ != nullptr)
    {
        esp_lcd_panel_io_del(panel_io_);
    }
}
// 锁定LVGL屏幕资源
bool LcdDisplay::Lock(int timeout_ms)
{
    return lvgl_port_lock(timeout_ms); // 尝试获取屏幕锁，超时时间为timeout_ms
}

void LcdDisplay::Unlock()
{
    lvgl_port_unlock(); // 释放屏幕锁
}

void LcdDisplay::SetupUI()
{
    DisplayLockGuard lock(this); // 确保UI初始化时的屏幕资源锁定

    auto screen = lv_screen_active(); // 获取当前屏幕
    // 设置字体颜色为白色，背景颜色为黑色
    lv_obj_set_style_text_font(screen, fonts_.text_font, 0);
    lv_obj_set_style_text_color(screen, lv_color_white(), 0); // 白色文字
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);   // 黑色背景

    /* 容器初始化 */
    container_ = lv_obj_create(screen);                    // 创建容器对象
    lv_obj_set_size(container_, LV_HOR_RES, LV_VER_RES);   // 设置容器大小为屏幕大小
    lv_obj_set_flex_flow(container_, LV_FLEX_FLOW_COLUMN); // 设置为垂直布局
    lv_obj_set_style_pad_all(container_, 0, 0);            // 设置容器内边距为0
    lv_obj_set_style_border_width(container_, 0, 0);       // 设置边框宽度为0
    lv_obj_set_style_pad_row(container_, 0, 0);            // 设置行间距为0

    /* 状态栏初始化 */
    status_bar_ = lv_obj_create(container_);                                 // 创建状态栏
    lv_obj_set_size(status_bar_, LV_HOR_RES, fonts_.text_font->line_height); // 设置状态栏高度
    lv_obj_set_style_radius(status_bar_, 0, 0);                              // 设置无圆角
    lv_obj_set_style_bg_color(status_bar_, lv_color_black(), 0);             // 设置背景为黑色
    lv_obj_set_style_text_color(status_bar_, lv_color_white(), 0);           // 设置文字颜色为白色

    /* 内容区域初始化 */
    content_ = lv_obj_create(container_);                       // 创建内容区域
    lv_obj_set_scrollbar_mode(content_, LV_SCROLLBAR_MODE_OFF); // 禁用滚动条
    lv_obj_set_style_radius(content_, 0, 0);                    // 设置无圆角
    lv_obj_set_width(content_, LV_HOR_RES);                     // 设置内容区域宽度为屏幕宽度
    lv_obj_set_flex_grow(content_, 1);                          // 使内容区域可以自动扩展
    lv_obj_set_style_bg_color(content_, lv_color_black(), 0);   // 设置背景为黑色

    lv_obj_set_flex_flow(content_, LV_FLEX_FLOW_COLUMN);                                                     // 垂直布局（从上到下）
    lv_obj_set_flex_align(content_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_EVENLY); // 子对象居中对齐，等距分布

    // 表情标签初始化
    emotion_label_ = lv_label_create(content_);
    lv_obj_set_style_text_font(emotion_label_, &font_awesome_30_4, 0); // 设置表情标签字体
    lv_label_set_text(emotion_label_, FONT_AWESOME_AI_CHIP);           // 设置默认表情为 "AI Chip"
    lv_obj_set_style_text_color(emotion_label_, lv_color_white(), 0);  // 设置文字颜色为白色

    // 聊天消息标签初始化
    chat_message_label_ = lv_label_create(content_);
    lv_label_set_text(chat_message_label_, "");                                // 初始化为空消息
    lv_obj_set_width(chat_message_label_, LV_HOR_RES * 0.9);                   // 限制宽度为屏幕宽度的 90%
    lv_label_set_long_mode(chat_message_label_, LV_LABEL_LONG_WRAP);           // 设置为自动换行模式
    lv_obj_set_style_text_align(chat_message_label_, LV_TEXT_ALIGN_CENTER, 0); // 设置文本居中对齐
    lv_obj_set_style_text_color(chat_message_label_, lv_color_white(), 0);     // 设置文字颜色为白色

    // 状态栏元素初始化
    lv_obj_set_flex_flow(status_bar_, LV_FLEX_FLOW_ROW); // 设置为水平布局
    lv_obj_set_style_pad_all(status_bar_, 0, 0);         // 设置状态栏内边距为0
    lv_obj_set_style_border_width(status_bar_, 0, 0);    // 设置无边框
    lv_obj_set_style_pad_column(status_bar_, 0, 0);      // 设置列间距为0
    lv_obj_set_style_pad_left(status_bar_, 2, 0);        // 设置左边距为2
    lv_obj_set_style_pad_right(status_bar_, 2, 0);       // 设置右边距为2

    // 网络状态标签
    network_label_ = lv_label_create(status_bar_);
    lv_label_set_text(network_label_, "");                            // 初始化为空
    lv_obj_set_style_text_font(network_label_, fonts_.icon_font, 0);  // 设置图标字体
    lv_obj_set_style_text_color(network_label_, lv_color_white(), 0); // 设置文字颜色为白色

    // 通知标签
    notification_label_ = lv_label_create(status_bar_);
    lv_obj_set_flex_grow(notification_label_, 1);                              // 使标签可以扩展
    lv_obj_set_style_text_align(notification_label_, LV_TEXT_ALIGN_CENTER, 0); // 设置文本居中对齐
    lv_label_set_text(notification_label_, "");                                // 初始化为空
    lv_obj_add_flag(notification_label_, LV_OBJ_FLAG_HIDDEN);                  // 初始状态为隐藏
    lv_obj_set_style_text_color(notification_label_, lv_color_white(), 0);     // 设置文字颜色为白色

    // 状态标签
    status_label_ = lv_label_create(status_bar_);
    lv_obj_set_flex_grow(status_label_, 1);                               // 使标签可以扩展
    lv_label_set_long_mode(status_label_, LV_LABEL_LONG_SCROLL_CIRCULAR); // 设置循环滚动模式
    lv_obj_set_style_text_align(status_label_, LV_TEXT_ALIGN_CENTER, 0);  // 设置文本居中对齐
    lv_label_set_text(status_label_, Lang::Strings::INITIALIZING);        // 设置默认文本为 "初始化中"
    lv_obj_set_style_text_color(status_label_, lv_color_white(), 0);      // 设置文字颜色为白色

    // 静音标签
    mute_label_ = lv_label_create(status_bar_);
    lv_label_set_text(mute_label_, "");                            // 初始化为空
    lv_obj_set_style_text_font(mute_label_, fonts_.icon_font, 0);  // 设置图标字体
    lv_obj_set_style_text_color(mute_label_, lv_color_white(), 0); // 设置文字颜色为白色

    // 电池标签
    battery_label_ = lv_label_create(status_bar_);
    lv_label_set_text(battery_label_, "");                            // 初始化为空
    lv_obj_set_style_text_font(battery_label_, fonts_.icon_font, 0);  // 设置图标字体
    lv_obj_set_style_text_color(battery_label_, lv_color_white(), 0); // 设置文字颜色为白色

    // 电池低电量弹窗
    low_battery_popup_ = lv_obj_create(screen);
    lv_obj_set_scrollbar_mode(low_battery_popup_, LV_SCROLLBAR_MODE_OFF);                     // 禁用滚动条
    lv_obj_set_size(low_battery_popup_, LV_HOR_RES * 0.9, fonts_.text_font->line_height * 2); // 设置大小
    lv_obj_align(low_battery_popup_, LV_ALIGN_BOTTOM_MID, 0, 0);                              // 设置底部对齐
    lv_obj_set_style_bg_color(low_battery_popup_, lv_color_white(), 0);                       // 设置背景为白色
    lv_obj_set_style_radius(low_battery_popup_, 10, 0);                                       // 设置圆角为10
    lv_obj_t *low_battery_label = lv_label_create(low_battery_popup_);
    lv_label_set_text(low_battery_label, Lang::Strings::BATTERY_NEED_CHARGE); // 设置电池低电量提示文字
    lv_obj_set_style_text_color(low_battery_label, lv_color_black(), 0);      // 设置文字为黑色
    lv_obj_center(low_battery_label);                                         // 文字居中
    lv_obj_add_flag(low_battery_popup_, LV_OBJ_FLAG_HIDDEN);                  // 初始状态为隐藏
}

void LcdDisplay::SetEmotion(const char *emotion)
{
    struct Emotion
    {
        const char *icon; // 表情图标
        const char *text; // 表情文本描述
    };

    static const std::vector<Emotion> emotions = {
        {"😶", "neutral"},
        {"🙂", "happy"},
        {"😆", "laughing"},
        {"😂", "funny"},
        {"😔", "sad"},
        {"😠", "angry"},
        {"😭", "crying"},
        {"😍", "loving"},
        {"😳", "embarrassed"},
        {"😯", "surprised"},
        {"😱", "shocked"},
        {"🤔", "thinking"},
        {"😉", "winking"},
        {"😎", "cool"},
        {"😌", "relaxed"},
        {"🤤", "delicious"},
        {"😘", "kissy"},
        {"😏", "confident"},
        {"😴", "sleepy"},
        {"😜", "silly"},
        {"🙄", "confused"},
    };

    // 查找匹配的表情
    std::string_view emotion_view(emotion);
    auto it = std::find_if(emotions.begin(), emotions.end(),
                           [&emotion_view](const Emotion &e)
                           { return e.text == emotion_view; });

    DisplayLockGuard lock(this); // 确保UI更新时的屏幕资源锁定
    if (emotion_label_ == nullptr)
        return;

    // 如果找到匹配的表情就显示对应图标，否则显示默认的neutral表情
    lv_obj_set_style_text_font(emotion_label_, fonts_.emoji_font, 0); // 设置为表情字体
    if (it != emotions.end())
    {
        lv_label_set_text(emotion_label_, it->icon); // 设置为找到的表情图标
    }
    else
    {
        lv_label_set_text(emotion_label_, "😶"); // 默认表情
    }
}

void LcdDisplay::SetIcon(const char *icon)
{
    DisplayLockGuard lock(this); // 确保UI更新时的屏幕资源锁定
    if (emotion_label_ == nullptr)
    {
        return;
    }
    lv_obj_set_style_text_font(emotion_label_, &font_awesome_30_4, 0); // 设置为图标字体
    lv_label_set_text(emotion_label_, icon);                           // 设置为指定的图标
}
