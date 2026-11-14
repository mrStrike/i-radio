// Здесь задаю конфигурацию LVGL для моего радио.
// Цвет RGB565 занимает 16 бит; пул объектов размером 512 КиБ размещаю в PSRAM.
// Дополнительный пул регистрирую в gui_task; значения памяти здесь не меняю.
// LV_TICK_CUSTOM = 0: время передаю вручную через lv_tick_inc в GUI.
#ifndef LV_CONF_H
#define LV_CONF_H
#define LV_COLOR_DEPTH 16
/* Все экраны и строки списка с отдельными чекбоксами: запас в PSRAM. */
#define LV_MEM_SIZE (512U * 1024U)
/* КРИТИЧНО: размещаем пул LVGL в PSRAM, а не в дефицитной внутренней DRAM */
#define LV_MEM_POOL_INCLUDE "esp_heap_caps.h"
#define LV_MEM_POOL_ALLOC(sz) heap_caps_malloc(sz, MALLOC_CAP_SPIRAM)
#define LV_TICK_CUSTOM 0
#endif
