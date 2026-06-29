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

// Отдельная изолированная задача для обновления экрана
void lvgl_task(void *pvParameters) {
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(10));
        lv_timer_handler(); // Обновляем графику
    }
}

void app_main(void) {
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

  // Запускаем штатный дисплей-менеджер прошивки
  display_manager_init();

  // Создаем фоновую задачу для экрана, чтобы разгрузить основное ядро и убрать ошибку Watchdog
  xTaskCreatePinnedToCore(lvgl_task, "lvgl_task", 4096, NULL, 5, NULL, 1);
}
