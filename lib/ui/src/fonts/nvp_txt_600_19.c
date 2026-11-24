/*******************************************************************************
 * Size: 19 px
 * Bpp: 4
 * Opts: --no-compress --no-prefilter --bpp 4 --format lvgl --force-fast-kern-format --size 19 --font Barlow-SemiBold.ttf -r 0x31-0x33 --lv-font-name nvp_txt_600_19 -o nvp_txt_600_19.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef NVP_TXT_600_19
#define NVP_TXT_600_19 1
#endif

#if NVP_TXT_600_19

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0031 "1" */
    0x3, 0xaf, 0xf5, 0x1f, 0xff, 0xf6, 0x1a, 0x5d,
    0xf6, 0x0, 0xd, 0xf6, 0x0, 0xd, 0xf6, 0x0,
    0xd, 0xf6, 0x0, 0xd, 0xf6, 0x0, 0xd, 0xf6,
    0x0, 0xd, 0xf6, 0x0, 0xd, 0xf6, 0x0, 0xd,
    0xf6, 0x0, 0xd, 0xf6, 0x0, 0xd, 0xf5,

    /* U+0032 "2" */
    0x0, 0x5c, 0xff, 0xc6, 0x0, 0x6, 0xff, 0xff,
    0xff, 0x80, 0xe, 0xf9, 0x0, 0x7f, 0xf1, 0x1f,
    0xf2, 0x0, 0xf, 0xf3, 0x4, 0x40, 0x0, 0x3f,
    0xf1, 0x0, 0x0, 0x0, 0xcf, 0xa0, 0x0, 0x0,
    0xa, 0xfe, 0x10, 0x0, 0x0, 0x8f, 0xf3, 0x0,
    0x0, 0x6, 0xff, 0x40, 0x0, 0x0, 0x5f, 0xf5,
    0x0, 0x0, 0x4, 0xff, 0x60, 0x0, 0x0, 0xf,
    0xff, 0xff, 0xff, 0xf8, 0xf, 0xff, 0xff, 0xff,
    0xf8,

    /* U+0033 "3" */
    0xf, 0xff, 0xff, 0xff, 0xf0, 0xe, 0xee, 0xee,
    0xff, 0xf0, 0x0, 0x0, 0x8, 0xff, 0x70, 0x0,
    0x0, 0x9f, 0xf7, 0x0, 0x0, 0xa, 0xff, 0x70,
    0x0, 0x0, 0x1e, 0xff, 0xfa, 0x0, 0x0, 0x3,
    0x43, 0xdf, 0x90, 0x0, 0x0, 0x0, 0x4f, 0xf0,
    0x4, 0x40, 0x0, 0x2f, 0xf1, 0x3f, 0xf1, 0x0,
    0x3f, 0xf0, 0xe, 0xf9, 0x0, 0xbf, 0xc0, 0x5,
    0xff, 0xff, 0xff, 0x40, 0x0, 0x4c, 0xfe, 0xb3,
    0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 107, .box_w = 6, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 39, .adv_w = 168, .box_w = 10, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 104, .adv_w = 161, .box_w = 10, .box_h = 13, .ofs_x = 0, .ofs_y = 0}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 49, .range_length = 3, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};

/*-----------------
 *    KERNING
 *----------------*/


/*Map glyph_ids to kern left classes*/
static const uint8_t kern_left_class_mapping[] =
{
    0, 1, 2, 3
};

/*Map glyph_ids to kern right classes*/
static const uint8_t kern_right_class_mapping[] =
{
    0, 1, 0, 2
};

/*Kern values between classes*/
static const int8_t kern_class_values[] =
{
    2, 0, -3, -3, -1, 0
};


/*Collect the kern class' data in one place*/
static const lv_font_fmt_txt_kern_classes_t kern_classes =
{
    .class_pair_values   = kern_class_values,
    .left_class_mapping  = kern_left_class_mapping,
    .right_class_mapping = kern_right_class_mapping,
    .left_class_cnt      = 3,
    .right_class_cnt     = 2,
};

/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = &kern_classes,
    .kern_scale = 16,
    .cmap_num = 1,
    .bpp = 4,
    .kern_classes = 1,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t nvp_txt_600_19 = {
#else
lv_font_t nvp_txt_600_19 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 13,          /*The maximum line height required by the font*/
    .base_line = 0,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if NVP_TXT_600_19*/

