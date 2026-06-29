# Автоматический поиск всех файлов .c в папке main
file(GLOB_RECURSE app_sources "${CMAKE_SOURCE_DIR}/main/*.c")

# Автоматический поиск всех папок для заголовочных файлов в include
file(GLOB_RECURSE inc_dirs LIST_DIRECTORIES true "${CMAKE_SOURCE_DIR}/include/*")

# Очистка списка, оставляем только директории
set(filtered_inc_dirs "")
foreach(inc_dir ${inc_dirs})
    if(IS_DIRECTORY ${inc_dir})
        list(APPEND filtered_inc_dirs ${inc_dir})
    endif()
endforeach()

# Регистрируем компонент с корректным списком требований, включая mdns и драйверы экрана
idf_component_register(SRCS ${app_sources} "vendor/m5gfx_wrapper.cpp"
                       INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/include"
                       PRIV_REQUIRES mdns lvgl_esp32_drivers
                       REQUIRES bt nvs_flash driver esp_http_server json esp_http_client mbedtls fatfs sdmmc wpa_supplicant
)

# Отключаем строгость компилятора для папки main
target_compile_options(${COMPONENT_LIB} PRIVATE "-w")
