/**
 * @file lv_conf.h
 * LVGL 9.6.0 configuration for the Namaz Vakti Panel, shared by the device and the host UI
 * harness (env `sim`). Only the options that differ from LVGL's defaults are set here; every
 * other option takes its default from `lvgl/src/lv_conf_internal.h`.
 *
 * Names checked against `lv_conf_template.h` of lvgl 9.6.0 (research R9,
 * specs/001-silent-board-bringup). Included through `LV_CONF_INCLUDE_SIMPLE` and `-Iinclude`.
 */
#ifndef LV_CONF_H
#define LV_CONF_H

/* Colour: the panel is RGB565 (research R4). LVGL 9 uses a format, not LV_COLOR_DEPTH. */
#define LV_COLOR_FORMAT_DEFAULT LV_COLOR_FORMAT_RGB565

#if defined(NVP_SIM) && NVP_SIM
/* Host harness: the screenshot compare needs a few MB, so use the C library allocator. */
#define LV_USE_STDLIB_MALLOC LV_STDLIB_CLIB
#else
/* Device: LVGL's own pool, size in bytes (LVGL 9 convention). */
#define LV_USE_STDLIB_MALLOC LV_STDLIB_BUILTIN
#define LV_MEM_SIZE (64 * 1024)
#endif

/* No OS integration: LVGL runs on one core (core 1) from loop(). */
#define LV_USE_OS LV_OS_NONE

/* Sub-pixel coordinates for lines and triangles: the leader lines and the marker sit at fractional
 * x positions (02-ui-spec.md §3, X(t)); the ESP32-S3 has a single-precision FPU (spec 003). */
#define LV_USE_FLOAT 1
/* About 30 frames per second of UI refresh. */
#define LV_DEF_REFR_PERIOD 33

/* Fonts: engineering screens only use these two sizes. */
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14

#if defined(NVP_SIM) && NVP_SIM
/* Host harness: test display, screenshot compare to PNG (research R12). */
#define LV_USE_LOG 1
#define LV_USE_TEST 1
#define LV_USE_TEST_SCREENSHOT_COMPARE 1
/* Write the PNG when no reference exists; the harness deletes old snapshots first. */
#define LV_TEST_SCREENSHOT_CREATE_REFERENCE_IMAGE 1
#define LV_USE_LODEPNG 1
/* LVGL's LodePNG reads and writes through lv_fs: give it the C stdio driver as the default
 * drive, so plain relative paths such as out/snapshots/x.png work. */
#define LV_USE_FS_STDIO 1
#define LV_FS_STDIO_LETTER 'A'
#define LV_FS_STDIO_PATH ""
#define LV_FS_DEFAULT_DRIVER_LETTER 'A'
#else
/* Device: no LVGL logging. */
#define LV_USE_LOG 0
#endif

#endif /* LV_CONF_H */
