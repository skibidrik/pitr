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

// Подключаем низкоуровневые регистры для отключения Brownout защиты
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

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

void app_main(void) {
  // ЧИТ-КОД: Полностью отключаем аппаратный Brownout детектор при старте
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

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

  // Фоновый цикл обновления графической библиотеки LVGL
  while(1) {
      vTaskDelay(pdMS_TO_TICKS(10));
      lv_timer_handler(); 
  }
}
