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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Системный заголовок для инициализации шины I2C
#include "driver/i2c.h"

int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3){
  return 0;
}

int custom_vprintf(const char *fmt, va_list args)
{
  char buffer;
  int len = vsnprintf(buffer, sizeof(buffer), fmt, args);
  ap_manager_add_log(buffer);
  return len;
}

extern uint32_t lv_timer_handler(void);

// Функция деликатно «будит» контроллер экрана на старте по стандарту v5.x
void force_init_ssd1306(void) {
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = 21;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = 22;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 100000;
    conf.clk_flags = 0;

    i2c_param_config(0, &conf);
    i2c_driver_install(0, conf.mode, 0, 0, 0);
}

void app_main(void) {
  // Пробуждаем шину I2C
  force_init_ssd1306();

  // Запускаем штатные менеджеры Ghost ESP
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

  // Запускаем штатный дисплей-менеджер прошивки (он сам развернёт буфер графики)
  display_manager_init();

  // Фоновый цикл обновления графической библиотеки LVGL
  while(1) {
      vTaskDelay(pdMS_TO_TICKS(10));
      lv_timer_handler(); 
  }
}
