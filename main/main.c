#include "core/system_manager.h"
#include "core/serial_manager.h"
#include "core/commandline.h"
#include "managers/rgb_manager.h"
#include "managers/settings_manager.h"
#include "managers/wifi_manager.h"
#include "managers/ap_manager.h"
#include "managers/sd_card_manager.h"
#include "managers/display_manager.h"
#ifndef CONFIG_IDF_TARGET_ESP32S2
#include "managers/ble_manager.h"
#endif
#include <esp_log.h>

// Подключаем системные драйверы I2C для ручной принудительной инициализации дисплея
#include "driver/i2c.h"
#include "freertos/task.h"

#ifdef WITH_SCREEN
#include "managers/views/splash_screen.h"
#endif

// Конфигурация вашего экрана
#define I2C_MASTER_SDA_IO 33
#define I2C_MASTER_SCL_IO 32
#define I2C_MASTER_FREQ_HZ 400000
#define SSD1306_I2C_ADDRESS 0x3C

// Функция низкоуровневой отправки команды в SSD1306/SSD1315
void ssd1306_cmd(uint8_t cmd) {
    i2c_cmd_handle_t link = i2c_cmd_link_create();
    i2c_master_start(link);
    i2c_master_write_byte(link, (SSD1306_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(link, 0x00, true); // Control byte: кодовое слово команды
    i2c_master_write_byte(link, cmd, true);
    i2c_master_stop(link);
    i2c_master_cmd_begin(I2C_NUM_0, link, pdMS_TO_TICKS(10));
    i2c_cmd_link_delete(link);
}

// Принудительное включение экрана под стандарт фреймворка ESP-IDF v5.x
void force_init_ssd1306(void) {
    // Настраиваем шину I2C на ваших пинах D33 и D32 через правильную структуру i2c_config_t
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);

    // Базовая инициализация чипа SSD1306 / SSD1315 (Включение питания матрицы)
    ssd1306_cmd(0xAE); // Выключить дисплей
    ssd1306_cmd(0x8D); // Включить внутренний Charge Pump (накачку питания)
    ssd1306_cmd(0x14); 
    ssd1306_cmd(0xAF); // Включить дисплей обратно (экран должен подать признаки жизни)
}

int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3){
  return 0;
}

int custom_vprintf(const char *fmt, va_list args)
{
  char buffer[256];
  int len = vsnprintf(buffer, sizeof(buffer), fmt, args);

  ap_manager_add_log(buffer);

  return len;
}

void app_main(void) {
  // Насильно инициализируем ваш дисплей на пинах 33/32 при старте ESP32
  force_init_ssd1306();

  system_manager_init();
  serial_manager_init();
  wifi_manager_init();
#ifndef CONFIG_IDF_TARGET_ESP32S2
  ble_init();
#endif

#ifdef USB_MODULE
  wifi_manager_auto_deauth();
  return;
#endif

  command_init();
  register_commands();
  settings_init(&G_Settings);
  ap_manager_init();

  esp_err_t err = sd_card_init();

#ifdef WITH_SCREEN

#ifdef USE_JOYSTICK
  #define L_BTN 13
  #define C_BTN 34
  #define U_BTN 36
  #define R_BTN 39
  #define D_BTN 35

  joystick_init(&joysticks[0], L_BTN, HOLD_LIMIT, true);  
  joystick_init(&joysticks[1], C_BTN, HOLD_LIMIT, true);
  joystick_init(&joysticks[2], U_BTN, HOLD_LIMIT, true);
  joystick_init(&joysticks[3], R_BTN, HOLD_LIMIT, true);
  joystick_init(&joysticks[4], D_BTN, HOLD_LIMIT, true);
#endif

  display_manager_init();
  display_manager_switch_view(&splash_view);
#endif

#ifdef LED_DATA_PIN
  rgb_manager_init(&rgb_manager, LED_DATA_PIN, NUM_LEDS, LED_ORDER, LED_MODEL_WS2812, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC);

  if (settings_get_rgb_mode(&G_Settings) == 1)
  {
    xTaskCreate(rainbow_task, "Rainbow Task", 8192, &rgb_manager, 1, &rgb_effect_task_handle);
  }
#elif CONFIG_IDF_TARGET_ESP32S2
  rgb_manager_init(&rgb_manager, GPIO_NUM_NC, 1, LED_PIXEL_FORMAT_GRB, LED_MODEL_WS2812, 4, 5, 6);
  if (settings_get_rgb_mode(&G_Settings) == 1)
  {
  xTaskCreate(rainbow_task, "Rainbow Task", 8192, &rgb_manager, 1, &rgb_effect_task_handle);
  }
#endif
}
