/*******************************************************************************
 * Size: 12 px
 * Bpp: 1
 * Opts: --bpp 1 --size 12 --no-compress --stride 1 --align 1 --font DejaVuSans.ttf --range 32-127,1024-1279,1105,1025 --format lvgl -o dejavu_16.c
 ******************************************************************************/

#ifdef __has_include
    #if __has_include("lvgl.h")
        #ifndef LV_LVGL_H_INCLUDE_SIMPLE
            #define LV_LVGL_H_INCLUDE_SIMPLE
        #endif
    #endif
#endif

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
    #include "lvgl.h"
#else
    #include "lvgl/lvgl.h"
#endif



#ifndef DEJAVU_16
#define DEJAVU_16 1
#endif

#if DEJAVU_16

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xfc, 0x80,

    /* U+0022 "\"" */
    0xb6, 0x80,

    /* U+0023 "#" */
    0x12, 0x12, 0x14, 0x7f, 0x24, 0xfe, 0x28, 0x48,
    0x48,

    /* U+0024 "$" */
    0x20, 0x87, 0xe9, 0xa3, 0x83, 0xc9, 0x27, 0xe2,
    0x8,

    /* U+0025 "%" */
    0x61, 0x24, 0x89, 0x22, 0x50, 0x69, 0x82, 0x91,
    0x24, 0x49, 0x21, 0x80,

    /* U+0026 "&" */
    0x38, 0x40, 0x40, 0x60, 0xd3, 0x9a, 0x8e, 0xc6,
    0x7b,

    /* U+0027 "'" */
    0xe0,

    /* U+0028 "(" */
    0x4a, 0x49, 0x24, 0x89, 0x0,

    /* U+0029 ")" */
    0x49, 0x12, 0x49, 0x29, 0x0,

    /* U+002A "*" */
    0x25, 0x5d, 0xf2, 0x0,

    /* U+002B "+" */
    0x10, 0x10, 0x10, 0x10, 0xff, 0x10, 0x10, 0x10,

    /* U+002C "," */
    0xe0,

    /* U+002D "-" */
    0xe0,

    /* U+002E "." */
    0x80,

    /* U+002F "/" */
    0x11, 0x22, 0x24, 0x44, 0x48, 0x80,

    /* U+0030 "0" */
    0x79, 0x28, 0x61, 0x86, 0x18, 0x52, 0x78,

    /* U+0031 "1" */
    0xe1, 0x8, 0x42, 0x10, 0x84, 0xf8,

    /* U+0032 "2" */
    0xf4, 0x42, 0x11, 0x11, 0x98, 0xf8,

    /* U+0033 "3" */
    0x78, 0x10, 0x4f, 0x8, 0x10, 0x63, 0xf8,

    /* U+0034 "4" */
    0x18, 0x62, 0x9a, 0x4a, 0x2f, 0xc2, 0x8,

    /* U+0035 "5" */
    0x79, 0x4, 0x1e, 0xc, 0x10, 0x63, 0xf8,

    /* U+0036 "6" */
    0x3d, 0x8, 0x2e, 0xce, 0x18, 0x53, 0x78,

    /* U+0037 "7" */
    0xfc, 0x30, 0x86, 0x10, 0x43, 0x8, 0x60,

    /* U+0038 "8" */
    0x7a, 0x18, 0x61, 0x7a, 0x38, 0x61, 0x78,

    /* U+0039 "9" */
    0x7b, 0x28, 0x61, 0xcd, 0xd0, 0x42, 0xf0,

    /* U+003A ":" */
    0x84,

    /* U+003B ";" */
    0x87,

    /* U+003C "<" */
    0x7, 0x1c, 0xe0, 0xe0, 0x38, 0x7,

    /* U+003D "=" */
    0xff, 0x0, 0x0, 0xff,

    /* U+003E ">" */
    0xc0, 0x38, 0xe, 0x7, 0x38, 0xc0,

    /* U+003F "?" */
    0xe1, 0x13, 0x64, 0x40, 0x40,

    /* U+0040 "@" */
    0x1e, 0x8, 0x64, 0xa, 0x3d, 0x91, 0x64, 0x59,
    0x16, 0x3e, 0x40, 0x8, 0x41, 0xe0,

    /* U+0041 "A" */
    0x18, 0x18, 0x3c, 0x24, 0x24, 0x62, 0x7e, 0x43,
    0xc1,

    /* U+0042 "B" */
    0xfa, 0x18, 0x61, 0xfa, 0x18, 0x61, 0xf8,

    /* U+0043 "C" */
    0x3c, 0x86, 0x4, 0x8, 0x10, 0x20, 0x21, 0x3c,

    /* U+0044 "D" */
    0xf9, 0xa, 0xc, 0x18, 0x30, 0x60, 0xc2, 0xf8,

    /* U+0045 "E" */
    0xfc, 0x21, 0xf, 0xc2, 0x10, 0xf8,

    /* U+0046 "F" */
    0xfc, 0x21, 0xf, 0x42, 0x10, 0x80,

    /* U+0047 "G" */
    0x3e, 0x41, 0x80, 0x80, 0x87, 0x81, 0x81, 0x41,
    0x3e,

    /* U+0048 "H" */
    0x83, 0x6, 0xc, 0x1f, 0xf0, 0x60, 0xc1, 0x82,

    /* U+0049 "I" */
    0xff, 0x80,

    /* U+004A "J" */
    0x24, 0x92, 0x49, 0x24, 0xe0,

    /* U+004B "K" */
    0x8d, 0x32, 0xc7, 0xc, 0x14, 0x24, 0x44, 0x84,

    /* U+004C "L" */
    0x84, 0x21, 0x8, 0x42, 0x10, 0xf8,

    /* U+004D "M" */
    0xc3, 0xc3, 0xa5, 0xa5, 0xa5, 0x99, 0x99, 0x81,
    0x81,

    /* U+004E "N" */
    0xc3, 0x86, 0x8d, 0x99, 0x33, 0x62, 0xc3, 0x86,

    /* U+004F "O" */
    0x3c, 0x42, 0x81, 0x81, 0x81, 0x81, 0x81, 0x42,
    0x3c,

    /* U+0050 "P" */
    0xfa, 0x18, 0x61, 0xfa, 0x8, 0x20, 0x80,

    /* U+0051 "Q" */
    0x3c, 0x42, 0x81, 0x81, 0x81, 0x81, 0x81, 0x42,
    0x3c, 0x4, 0x2,

    /* U+0052 "R" */
    0xf9, 0xa, 0x14, 0x2f, 0x91, 0x21, 0x42, 0x82,

    /* U+0053 "S" */
    0x7e, 0x18, 0x30, 0x78, 0x10, 0x61, 0xf8,

    /* U+0054 "T" */
    0xfe, 0x20, 0x40, 0x81, 0x2, 0x4, 0x8, 0x10,

    /* U+0055 "U" */
    0x83, 0x6, 0xc, 0x18, 0x30, 0x60, 0xe3, 0x7c,

    /* U+0056 "V" */
    0xc1, 0x43, 0x42, 0x62, 0x24, 0x24, 0x1c, 0x18,
    0x18,

    /* U+0057 "W" */
    0x46, 0x28, 0xc5, 0x18, 0xa5, 0x36, 0x94, 0x52,
    0x8a, 0x51, 0x8e, 0x30, 0x80,

    /* U+0058 "X" */
    0x42, 0x26, 0x34, 0x18, 0x18, 0x1c, 0x24, 0x62,
    0x43,

    /* U+0059 "Y" */
    0x86, 0x88, 0xa1, 0x41, 0x2, 0x4, 0x8, 0x10,

    /* U+005A "Z" */
    0xfe, 0xc, 0x30, 0x41, 0x4, 0x18, 0x60, 0xfe,

    /* U+005B "[" */
    0xea, 0xaa, 0xac,

    /* U+005C "\\" */
    0x88, 0x44, 0x42, 0x22, 0x21, 0x10,

    /* U+005D "]" */
    0xd5, 0x55, 0x5c,

    /* U+005E "^" */
    0x18, 0x1c, 0x24, 0x42,

    /* U+005F "_" */
    0xfc,

    /* U+0060 "`" */
    0x89, 0x0,

    /* U+0061 "a" */
    0xfa, 0x30, 0x5f, 0x86, 0x37, 0x40,

    /* U+0062 "b" */
    0x82, 0x8, 0x2e, 0xce, 0x18, 0x61, 0xce, 0xe0,

    /* U+0063 "c" */
    0x7e, 0x21, 0x8, 0x61, 0xe0,

    /* U+0064 "d" */
    0x4, 0x10, 0x5d, 0xce, 0x18, 0x61, 0xcd, 0xd0,

    /* U+0065 "e" */
    0x7b, 0x38, 0x7f, 0x83, 0x7, 0xc0,

    /* U+0066 "f" */
    0x34, 0x4f, 0x44, 0x44, 0x44,

    /* U+0067 "g" */
    0x77, 0x38, 0x61, 0x87, 0x37, 0x41, 0xd, 0xe0,

    /* U+0068 "h" */
    0x84, 0x21, 0x6c, 0xc6, 0x31, 0x8c, 0x40,

    /* U+0069 "i" */
    0x9f, 0xc0,

    /* U+006A "j" */
    0x41, 0x55, 0x55, 0x80,

    /* U+006B "k" */
    0x82, 0x8, 0x22, 0x92, 0x8c, 0x28, 0x92, 0x20,

    /* U+006C "l" */
    0xff, 0xc0,

    /* U+006D "m" */
    0xb3, 0x66, 0x62, 0x31, 0x18, 0x8c, 0x46, 0x22,

    /* U+006E "n" */
    0xb6, 0x63, 0x18, 0xc6, 0x20,

    /* U+006F "o" */
    0x7b, 0x38, 0x61, 0x87, 0x37, 0x80,

    /* U+0070 "p" */
    0xbb, 0x38, 0x61, 0x87, 0x3b, 0xa0, 0x82, 0x0,

    /* U+0071 "q" */
    0x77, 0x38, 0x61, 0x87, 0x37, 0x41, 0x4, 0x10,

    /* U+0072 "r" */
    0xbc, 0x88, 0x88, 0x80,

    /* U+0073 "s" */
    0x7c, 0x20, 0xe0, 0x87, 0xc0,

    /* U+0074 "t" */
    0x44, 0xf4, 0x44, 0x44, 0x70,

    /* U+0075 "u" */
    0x8c, 0x63, 0x18, 0xcd, 0xa0,

    /* U+0076 "v" */
    0x46, 0x89, 0x11, 0x62, 0x85, 0x4, 0x0,

    /* U+0077 "w" */
    0x4c, 0xa6, 0x53, 0x2a, 0xa3, 0x31, 0x98, 0xcc,

    /* U+0078 "x" */
    0x44, 0x58, 0xe0, 0x82, 0x8d, 0x91, 0x0,

    /* U+0079 "y" */
    0x46, 0x89, 0x11, 0x42, 0x85, 0x4, 0x8, 0x30,
    0xc0,

    /* U+007A "z" */
    0xfc, 0x21, 0x8, 0x63, 0xf, 0xc0,

    /* U+007B "{" */
    0x39, 0x8, 0x42, 0x60, 0x84, 0x21, 0x8, 0x70,

    /* U+007C "|" */
    0xff, 0xf8,

    /* U+007D "}" */
    0xe1, 0x8, 0x42, 0xc, 0x84, 0x21, 0x9, 0xc0,

    /* U+007E "~" */
    0x71, 0x8e,

    /* U+0400 "Ѐ" */
    0x21, 0x1, 0xf8, 0x42, 0x1f, 0x84, 0x21, 0xf0,

    /* U+0401 "Ё" */
    0x50, 0x3f, 0x8, 0x43, 0xf0, 0x84, 0x3e,

    /* U+0402 "Ђ" */
    0xfe, 0x10, 0x10, 0x10, 0x1e, 0x11, 0x11, 0x11,
    0x11, 0x1, 0x1, 0x2,

    /* U+0403 "Ѓ" */
    0x11, 0x1, 0xf8, 0x42, 0x10, 0x84, 0x21, 0x0,

    /* U+0404 "Є" */
    0x3c, 0x87, 0x4, 0xf, 0xd0, 0x30, 0x21, 0x3c,

    /* U+0405 "Ѕ" */
    0x7e, 0x18, 0x30, 0x78, 0x10, 0x61, 0xf8,

    /* U+0406 "І" */
    0xff, 0x80,

    /* U+0407 "Ї" */
    0xa1, 0x24, 0x92, 0x49, 0x0,

    /* U+0408 "Ј" */
    0x24, 0x92, 0x49, 0x24, 0xe0,

    /* U+0409 "Љ" */
    0x3e, 0x2, 0x20, 0x22, 0x2, 0x20, 0x23, 0xe2,
    0x21, 0x22, 0x14, 0x21, 0x83, 0xe0,

    /* U+040A "Њ" */
    0x84, 0x21, 0x8, 0x42, 0x10, 0xff, 0xa1, 0x18,
    0x46, 0x11, 0x87, 0x80,

    /* U+040B "Ћ" */
    0xfe, 0x8, 0x4, 0x2, 0x1, 0xf0, 0x84, 0x42,
    0x21, 0x10, 0x80,

    /* U+040C "Ќ" */
    0x10, 0x20, 0x4, 0x28, 0x92, 0x28, 0x68, 0xd9,
    0x12, 0x14, 0x30,

    /* U+040D "Ѝ" */
    0x20, 0x20, 0x4, 0x38, 0x71, 0x66, 0xc9, 0xb3,
    0x47, 0xe, 0x10,

    /* U+040E "Ў" */
    0x0, 0x70, 0x6, 0x34, 0x48, 0x8b, 0x14, 0x38,
    0x20, 0x43, 0x0,

    /* U+040F "Џ" */
    0x83, 0x6, 0xc, 0x18, 0x30, 0x60, 0xc1, 0xfe,
    0x20, 0x40,

    /* U+0410 "А" */
    0x18, 0x18, 0x3c, 0x24, 0x24, 0x62, 0x7e, 0x43,
    0xc1,

    /* U+0411 "Б" */
    0xfe, 0x8, 0x20, 0xfa, 0x18, 0x61, 0xf8,

    /* U+0412 "В" */
    0xfa, 0x18, 0x61, 0xfa, 0x18, 0x61, 0xf8,

    /* U+0413 "Г" */
    0xfc, 0x21, 0x8, 0x42, 0x10, 0x80,

    /* U+0414 "Д" */
    0x3e, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x42,
    0xff, 0x81, 0x81,

    /* U+0415 "Е" */
    0xfc, 0x21, 0xf, 0xc2, 0x10, 0xf8,

    /* U+0416 "Ж" */
    0x42, 0x11, 0x11, 0x4, 0x90, 0x15, 0x1, 0x74,
    0x9, 0xa0, 0x88, 0x8c, 0x46, 0x42, 0x10,

    /* U+0417 "З" */
    0xf8, 0x10, 0x5e, 0xc, 0x10, 0x63, 0xf8,

    /* U+0418 "И" */
    0x87, 0xe, 0x2c, 0xd9, 0x36, 0x68, 0xe1, 0xc2,

    /* U+0419 "Й" */
    0x44, 0x70, 0x4, 0x38, 0x71, 0x66, 0xc9, 0xb3,
    0x47, 0xe, 0x10,

    /* U+041A "К" */
    0x85, 0x12, 0x45, 0xd, 0x1b, 0x22, 0x42, 0x86,

    /* U+041B "Л" */
    0x3e, 0x44, 0x89, 0x12, 0x24, 0x48, 0xa1, 0x82,

    /* U+041C "М" */
    0xc3, 0xc3, 0xa5, 0xa5, 0xa5, 0x99, 0x99, 0x81,
    0x81,

    /* U+041D "Н" */
    0x83, 0x6, 0xc, 0x1f, 0xf0, 0x60, 0xc1, 0x82,

    /* U+041E "О" */
    0x3c, 0x42, 0x81, 0x81, 0x81, 0x81, 0x81, 0x42,
    0x3c,

    /* U+041F "П" */
    0xff, 0x6, 0xc, 0x18, 0x30, 0x60, 0xc1, 0x82,

    /* U+0420 "Р" */
    0xfa, 0x18, 0x61, 0xfa, 0x8, 0x20, 0x80,

    /* U+0421 "С" */
    0x3c, 0x86, 0x4, 0x8, 0x10, 0x20, 0x21, 0x3c,

    /* U+0422 "Т" */
    0xfe, 0x20, 0x40, 0x81, 0x2, 0x4, 0x8, 0x10,

    /* U+0423 "У" */
    0xc6, 0x89, 0x11, 0x62, 0x87, 0x4, 0x8, 0x60,

    /* U+0424 "Ф" */
    0x8, 0x1f, 0x32, 0x51, 0x18, 0x8c, 0x45, 0x24,
    0x7c, 0x8, 0x0,

    /* U+0425 "Х" */
    0x42, 0x26, 0x34, 0x18, 0x18, 0x1c, 0x24, 0x62,
    0x43,

    /* U+0426 "Ц" */
    0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82,
    0xff, 0x1, 0x1,

    /* U+0427 "Ч" */
    0x86, 0x18, 0x61, 0x7c, 0x10, 0x41, 0x4,

    /* U+0428 "Ш" */
    0x84, 0x30, 0x86, 0x10, 0xc2, 0x18, 0x43, 0x8,
    0x61, 0xc, 0x21, 0xff, 0xe0,

    /* U+0429 "Щ" */
    0x84, 0x50, 0x8a, 0x11, 0x42, 0x28, 0x45, 0x8,
    0xa1, 0x14, 0x22, 0xff, 0xe0, 0x4, 0x0, 0x80,

    /* U+042A "Ъ" */
    0xf0, 0x8, 0x4, 0x2, 0x1, 0xf0, 0x84, 0x42,
    0x21, 0x1f, 0x0,

    /* U+042B "Ы" */
    0x81, 0x81, 0x81, 0x81, 0xf9, 0x85, 0x85, 0x85,
    0xf9,

    /* U+042C "Ь" */
    0x82, 0x8, 0x20, 0xfa, 0x18, 0x61, 0xf8,

    /* U+042D "Э" */
    0x79, 0x8, 0x18, 0x17, 0xe0, 0x41, 0xc2, 0x78,

    /* U+042E "Ю" */
    0x87, 0x91, 0xa, 0x40, 0xc8, 0x1f, 0x3, 0x20,
    0x64, 0xc, 0x42, 0x87, 0x80,

    /* U+042F "Я" */
    0x7e, 0x18, 0x61, 0x7c, 0x94, 0x71, 0x84,

    /* U+0430 "а" */
    0xfa, 0x30, 0x5f, 0x86, 0x37, 0x40,

    /* U+0431 "б" */
    0x39, 0x8, 0x3e, 0xce, 0x18, 0x61, 0xcd, 0xe0,

    /* U+0432 "в" */
    0xf4, 0x63, 0xe8, 0xc7, 0xc0,

    /* U+0433 "г" */
    0xfc, 0x21, 0x8, 0x42, 0x0,

    /* U+0434 "д" */
    0x3c, 0x48, 0x91, 0x22, 0x48, 0xbf, 0xc1,

    /* U+0435 "е" */
    0x7b, 0x38, 0x7f, 0x83, 0x7, 0xc0,

    /* U+0436 "ж" */
    0xc9, 0xa4, 0x8a, 0x87, 0xc4, 0x96, 0x4e, 0x22,

    /* U+0437 "з" */
    0xf0, 0x42, 0xe0, 0x87, 0xc0,

    /* U+0438 "и" */
    0x8e, 0x39, 0x6d, 0xa7, 0x1c, 0x40,

    /* U+0439 "й" */
    0x49, 0xe0, 0x23, 0x8e, 0x5b, 0x69, 0xc7, 0x10,

    /* U+043A "к" */
    0x8a, 0x4a, 0x38, 0x92, 0x68, 0x80,

    /* U+043B "л" */
    0x3c, 0x92, 0x49, 0x25, 0x9c, 0x40,

    /* U+043C "м" */
    0xc7, 0x8e, 0xad, 0x5a, 0xb2, 0x60, 0x80,

    /* U+043D "н" */
    0x86, 0x18, 0x7f, 0x86, 0x18, 0x40,

    /* U+043E "о" */
    0x7b, 0x38, 0x61, 0x87, 0x37, 0x80,

    /* U+043F "п" */
    0xfe, 0x18, 0x61, 0x86, 0x18, 0x40,

    /* U+0440 "р" */
    0xbb, 0x38, 0x61, 0x87, 0x3b, 0xa0, 0x82, 0x0,

    /* U+0441 "с" */
    0x7e, 0x21, 0x8, 0x61, 0xe0,

    /* U+0442 "т" */
    0xfe, 0x20, 0x40, 0x81, 0x2, 0x4, 0x0,

    /* U+0443 "у" */
    0x46, 0x89, 0x11, 0x42, 0x85, 0x4, 0x8, 0x30,
    0xc0,

    /* U+0444 "ф" */
    0x8, 0x4, 0x1f, 0xd9, 0x38, 0x8c, 0x46, 0x23,
    0x93, 0x7f, 0x4, 0x2, 0x1, 0x0,

    /* U+0445 "х" */
    0x44, 0x58, 0xe0, 0x82, 0x8d, 0x91, 0x0,

    /* U+0446 "ц" */
    0x85, 0xa, 0x14, 0x28, 0x50, 0xbf, 0x81,

    /* U+0447 "ч" */
    0x8c, 0x62, 0xf0, 0x84, 0x20,

    /* U+0448 "ш" */
    0x88, 0xc4, 0x62, 0x31, 0x18, 0x8c, 0x47, 0xfe,

    /* U+0449 "щ" */
    0x88, 0xa2, 0x28, 0x8a, 0x22, 0x88, 0xa2, 0x2f,
    0xfc, 0x1,

    /* U+044A "ъ" */
    0xe0, 0x40, 0x81, 0xe2, 0x24, 0x4f, 0x0,

    /* U+044B "ы" */
    0x83, 0x6, 0xf, 0x98, 0xb1, 0x7c, 0x80,

    /* U+044C "ь" */
    0x84, 0x21, 0xe8, 0xc7, 0xc0,

    /* U+044D "э" */
    0xf0, 0xc2, 0xf0, 0x8f, 0xc0,

    /* U+044E "ю" */
    0x9e, 0xb3, 0xa1, 0xe1, 0xa1, 0xb3, 0x9e,

    /* U+044F "я" */
    0x7c, 0x62, 0xf4, 0xe6, 0x20,

    /* U+0450 "ѐ" */
    0x40, 0x80, 0x1e, 0xce, 0x1f, 0xe0, 0xc1, 0xf0,

    /* U+0451 "ё" */
    0x28, 0x0, 0x1e, 0xce, 0x1f, 0xe0, 0xc1, 0xf0,

    /* U+0452 "ђ" */
    0x41, 0xf, 0x90, 0x59, 0x94, 0x51, 0x44, 0x20,
    0x80,

    /* U+0453 "ѓ" */
    0x11, 0x1, 0xf8, 0x42, 0x10, 0x84, 0x0,

    /* U+0454 "є" */
    0x7e, 0x21, 0xe8, 0x61, 0xe0,

    /* U+0455 "ѕ" */
    0x7c, 0x20, 0xe0, 0x87, 0xc0,

    /* U+0456 "і" */
    0xbf, 0x80,

    /* U+0457 "ї" */
    0xa1, 0x24, 0x92, 0x40,

    /* U+0458 "ј" */
    0x45, 0x55, 0x56,

    /* U+0459 "љ" */
    0x3c, 0x9, 0x2, 0x40, 0x9e, 0x24, 0x51, 0x1c,
    0x78,

    /* U+045A "њ" */
    0x88, 0x44, 0x22, 0x1f, 0xe8, 0x8c, 0x46, 0x3c,

    /* U+045B "ћ" */
    0x41, 0xf, 0x90, 0x59, 0x94, 0x51, 0x44,

    /* U+045C "ќ" */
    0x10, 0x80, 0x22, 0x92, 0x8e, 0x24, 0x9a, 0x20,

    /* U+045D "ѝ" */
    0x40, 0x80, 0x23, 0x8e, 0x5b, 0x69, 0xc7, 0x10,

    /* U+045E "ў" */
    0x48, 0xc8, 0x61, 0x49, 0x27, 0x8c, 0x30, 0xc2,
    0x30,

    /* U+045F "џ" */
    0x86, 0x18, 0x61, 0x86, 0x1f, 0xc8, 0x20,

    /* U+0460 "Ѡ" */
    0x41, 0x20, 0xa0, 0x30, 0x18, 0x8c, 0x46, 0x23,
    0xab, 0x77, 0x0,

    /* U+0461 "ѡ" */
    0x42, 0x81, 0x81, 0x89, 0x89, 0xd9, 0x76,

    /* U+0462 "Ѣ" */
    0x20, 0xfc, 0x20, 0x20, 0x3e, 0x21, 0x21, 0x21,
    0x3e,

    /* U+0463 "ѣ" */
    0x20, 0x40, 0x87, 0xc2, 0x4, 0xf, 0x11, 0x22,
    0x78,

    /* U+0464 "Ѥ" */
    0x87, 0xa2, 0x19, 0x2, 0x40, 0xff, 0x24, 0x9,
    0x2, 0x21, 0x87, 0x80,

    /* U+0465 "ѥ" */
    0x9f, 0x62, 0x87, 0xea, 0x16, 0x27, 0x80,

    /* U+0466 "Ѧ" */
    0x4, 0x1, 0x40, 0x28, 0xd, 0x81, 0x10, 0x7f,
    0x9, 0x23, 0x26, 0x44, 0x40,

    /* U+0467 "ѧ" */
    0x8, 0xa, 0x5, 0x4, 0x47, 0xe2, 0x4b, 0x26,

    /* U+0468 "Ѩ" */
    0x81, 0x4, 0x14, 0x20, 0xa1, 0xd, 0x8f, 0xc4,
    0x47, 0xf2, 0x64, 0x92, 0x26, 0xb1, 0x10,

    /* U+0469 "ѩ" */
    0x82, 0x10, 0xa2, 0x34, 0x7c, 0x49, 0xfd, 0x24,
    0xa8, 0x98,

    /* U+046A "Ѫ" */
    0x7f, 0xb1, 0x88, 0x86, 0xc1, 0xc3, 0xf9, 0x25,
    0x11, 0x88, 0x80,

    /* U+046B "ѫ" */
    0xfe, 0x89, 0xb1, 0xc7, 0xd2, 0x64, 0x80,

    /* U+046C "Ѭ" */
    0x9f, 0xe2, 0x28, 0xda, 0x14, 0xff, 0x23, 0xe9,
    0xaa, 0x49, 0x92, 0x40,

    /* U+046D "ѭ" */
    0xbf, 0x92, 0x9a, 0xfc, 0x9e, 0xa5, 0xa5,

    /* U+046E "Ѯ" */
    0x48, 0xc3, 0x3f, 0x4, 0x13, 0x81, 0x4, 0x1f,
    0xa0, 0x81, 0xe0,

    /* U+046F "ѯ" */
    0x52, 0x98, 0xb0, 0x98, 0x21, 0xf4, 0x3c,

    /* U+0470 "Ѱ" */
    0x88, 0xd2, 0x64, 0x91, 0x24, 0x2a, 0xa, 0x83,
    0xe0, 0x20, 0x8, 0x0,

    /* U+0471 "ѱ" */
    0x8, 0x2, 0x8, 0x8d, 0x26, 0x49, 0x12, 0x42,
    0xa0, 0xa8, 0x3e, 0x2, 0x0, 0x80, 0x20,

    /* U+0472 "Ѳ" */
    0x3c, 0x42, 0x81, 0xff, 0x81, 0x81, 0xc3, 0x42,
    0x3c,

    /* U+0473 "ѳ" */
    0x7b, 0x38, 0x7f, 0x87, 0x37, 0x80,

    /* U+0474 "Ѵ" */
    0x81, 0xe1, 0x10, 0x88, 0xc2, 0x41, 0x20, 0xa0,
    0x30, 0x18, 0x0,

    /* U+0475 "ѵ" */
    0xc3, 0x44, 0x44, 0x2c, 0x28, 0x28, 0x10,

    /* U+0476 "Ѷ" */
    0x28, 0x0, 0x20, 0x78, 0x44, 0x22, 0x30, 0x90,
    0x48, 0x28, 0xc, 0x6, 0x0,

    /* U+0477 "ѷ" */
    0x28, 0x14, 0x0, 0xc3, 0x44, 0x44, 0x2c, 0x28,
    0x28, 0x10,

    /* U+0478 "Ѹ" */
    0x70, 0x14, 0x8, 0xc6, 0x29, 0x8a, 0xa2, 0xe8,
    0x99, 0x44, 0x71, 0x0, 0xc0, 0x20, 0x18,

    /* U+0479 "ѹ" */
    0x74, 0x77, 0x28, 0xaa, 0x2e, 0x8b, 0x36, 0x47,
    0x30, 0x8, 0x2, 0x3, 0x0,

    /* U+047A "Ѻ" */
    0x3f, 0x13, 0x28, 0x6, 0x1, 0x80, 0x60, 0x18,
    0x5, 0x32, 0x3f, 0x0,

    /* U+047B "ѻ" */
    0x7e, 0xc3, 0x81, 0x81, 0x81, 0xc3, 0x7e,

    /* U+047C "Ѽ" */
    0x7, 0x0, 0x64, 0x0, 0x80, 0x4, 0x4, 0x21,
    0x20, 0xa, 0x0, 0x30, 0x1, 0x80, 0xc, 0x38,
    0x62, 0x22, 0xe0, 0xe0,

    /* U+047D "ѽ" */
    0xe, 0x7, 0x80, 0x43, 0x22, 0x80, 0x60, 0x18,
    0x6, 0x79, 0x73, 0x80,

    /* U+047E "Ѿ" */
    0x3e, 0x4, 0x0, 0x8, 0x24, 0x14, 0x6, 0x3,
    0x11, 0x88, 0xc4, 0x75, 0x6e, 0xe0,

    /* U+047F "ѿ" */
    0x3e, 0x28, 0x42, 0x81, 0x81, 0x89, 0x89, 0xd9,
    0x76,

    /* U+0480 "Ҁ" */
    0x3c, 0x86, 0x4, 0x8, 0x10, 0x20, 0x20, 0x3c,
    0x8, 0x10, 0x20,

    /* U+0481 "ҁ" */
    0x7e, 0x21, 0x8, 0x61, 0xe1, 0x8, 0x40,

    /* U+0482 "҂" */
    0x0, 0xe1, 0x9c, 0x71, 0x40, 0x0,

    /* U+0483 "҃" */
    0x1f, 0xe0,

    /* U+0484 "҄" */
    0x79,

    /* U+0485 "҅" */
    0xe0,

    /* U+0486 "҆" */
    0xe0,

    /* U+0487 "҇" */
    0x7c, 0x43, 0xc0,

    /* U+0488 "҈" */
    0x0, 0xc0, 0x0, 0x90, 0xf, 0x7, 0x84, 0x82,
    0x40, 0x0, 0x0, 0x0, 0x3, 0xc0, 0x1f, 0x20,
    0x9, 0x0, 0x0, 0x0, 0x0, 0x6, 0x3, 0x4,
    0x82, 0x40, 0xc, 0x0, 0x9, 0x0,

    /* U+0489 "҉" */
    0x1, 0x0, 0x1, 0x0, 0x11, 0x0, 0x10, 0x1c,
    0x0, 0x10, 0x0, 0x0, 0x0, 0x0, 0xe0, 0x6,
    0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x10, 0x18,
    0x30, 0x8, 0x1, 0x8, 0x1, 0x0, 0x2, 0x0,

    /* U+048A "Ҋ" */
    0x38, 0x0, 0x86, 0x86, 0x8a, 0x9a, 0x92, 0xb2,
    0xa2, 0xc2, 0xc3, 0x1, 0x2, 0x2,

    /* U+048B "ҋ" */
    0x48, 0x62, 0x34, 0x69, 0x56, 0xa9, 0x62, 0xc6,
    0x4, 0x10, 0x20,

    /* U+048C "Ҍ" */
    0x41, 0xc1, 0x2, 0x7, 0xc8, 0x50, 0xa1, 0x7c,

    /* U+048D "ҍ" */
    0x41, 0xe, 0x10, 0x41, 0xe4, 0x51, 0x78,

    /* U+048E "Ҏ" */
    0xfa, 0x19, 0x63, 0xfe, 0x8, 0x20, 0x80,

    /* U+048F "ҏ" */
    0xbb, 0x38, 0x61, 0x97, 0x7b, 0xa1, 0x82, 0x0,

    /* U+0490 "Ґ" */
    0x8, 0x7f, 0x8, 0x42, 0x10, 0x84, 0x20,

    /* U+0491 "ґ" */
    0x8, 0x7f, 0x8, 0x42, 0x10, 0x80,

    /* U+0492 "Ғ" */
    0x3e, 0x40, 0x81, 0xf, 0xc4, 0x8, 0x10, 0x20,

    /* U+0493 "ғ" */
    0x3c, 0x82, 0x3e, 0x20, 0x82, 0x0,

    /* U+0494 "Ҕ" */
    0xfe, 0x8, 0x20, 0xfa, 0x18, 0x61, 0x84, 0x10,
    0x46,

    /* U+0495 "ҕ" */
    0xfc, 0x21, 0xf8, 0xc6, 0x21, 0x9, 0x80,

    /* U+0496 "Җ" */
    0x42, 0x11, 0x11, 0x4, 0x90, 0x15, 0x1, 0x7c,
    0xb, 0xb0, 0x88, 0x8c, 0x42, 0x42, 0x18, 0x0,
    0x40, 0x2,

    /* U+0497 "җ" */
    0xc8, 0x92, 0x42, 0xa0, 0xfc, 0x49, 0x32, 0x28,
    0x8c, 0x1,

    /* U+0498 "Ҙ" */
    0xf8, 0x10, 0x41, 0x78, 0x10, 0x43, 0xf8, 0x81,
    0x18,

    /* U+0499 "ҙ" */
    0xf0, 0x42, 0xe0, 0x87, 0xc4, 0x23, 0x0,

    /* U+049A "Қ" */
    0x85, 0x12, 0x45, 0xd, 0x1b, 0x22, 0x42, 0x86,
    0x4, 0x8,

    /* U+049B "қ" */
    0x8a, 0x4a, 0x3c, 0x92, 0x28, 0xc1,

    /* U+049C "Ҝ" */
    0x87, 0x5a, 0xe5, 0x8f, 0x95, 0x2b, 0x42, 0x82,

    /* U+049D "ҝ" */
    0x9b, 0xce, 0x38, 0xd2, 0x68, 0x80,

    /* U+049E "Ҟ" */
    0x42, 0xe4, 0x48, 0x50, 0x68, 0x4c, 0x44, 0x42,
    0x43,

    /* U+049F "ҟ" */
    0xf0, 0x81, 0x12, 0x45, 0xe, 0x12, 0x26, 0x44,

    /* U+04A0 "Ҡ" */
    0xf0, 0x84, 0x41, 0x20, 0x50, 0x1a, 0x6, 0xc1,
    0x10, 0x42, 0x10, 0xc0,

    /* U+04A1 "ҡ" */
    0xe2, 0x24, 0x28, 0x38, 0x24, 0x26, 0x22,

    /* U+04A2 "Ң" */
    0x82, 0x82, 0x82, 0x82, 0xfe, 0x82, 0x82, 0x82,
    0x83, 0x1, 0x1,

    /* U+04A3 "ң" */
    0x85, 0xa, 0x17, 0xe8, 0x50, 0xa1, 0x81,

    /* U+04A4 "Ҥ" */
    0x83, 0xf0, 0x42, 0x8, 0x41, 0xf, 0xe1, 0x4,
    0x20, 0x84, 0x10, 0x82, 0x0,

    /* U+04A5 "ҥ" */
    0x87, 0xc2, 0x21, 0x1f, 0x88, 0x44, 0x22, 0x10,

    /* U+04A6 "Ҧ" */
    0xfe, 0x10, 0x42, 0x8, 0x41, 0x8, 0x3d, 0x4,
    0x60, 0x8c, 0x11, 0x82, 0x20, 0x4, 0x0, 0x80,
    0x20,

    /* U+04A7 "ҧ" */
    0xfc, 0x21, 0x8, 0x42, 0x1f, 0x84, 0x61, 0x18,
    0x44, 0x1, 0x0, 0x40, 0x60,

    /* U+04A8 "Ҩ" */
    0x0, 0x1d, 0xc4, 0xda, 0x22, 0x88, 0xa2, 0x28,
    0x8b, 0x14, 0x67, 0xf, 0xf0,

    /* U+04A9 "ҩ" */
    0x6c, 0xd2, 0x92, 0x92, 0x9e, 0xcc, 0x7f,

    /* U+04AA "Ҫ" */
    0x3c, 0x86, 0x4, 0x8, 0x10, 0x20, 0x21, 0x3c,
    0x10, 0x20, 0xc0,

    /* U+04AB "ҫ" */
    0x7e, 0x21, 0x8, 0x61, 0xe2, 0x11, 0x80,

    /* U+04AC "Ҭ" */
    0xfe, 0x20, 0x40, 0x81, 0x2, 0x4, 0x8, 0x18,
    0x10, 0x20,

    /* U+04AD "ҭ" */
    0xfc, 0x41, 0x4, 0x10, 0x41, 0x82,

    /* U+04AE "Ү" */
    0x86, 0x88, 0xa1, 0x41, 0x2, 0x4, 0x8, 0x10,

    /* U+04AF "ү" */
    0x44, 0x89, 0x11, 0x42, 0x85, 0x4, 0x8, 0x10,
    0x20,

    /* U+04B0 "Ұ" */
    0x42, 0x22, 0x34, 0x14, 0x8, 0x8, 0x3c, 0x8,
    0x8,

    /* U+04B1 "ұ" */
    0x85, 0x14, 0x9a, 0x28, 0xa1, 0x4, 0x78, 0x40,

    /* U+04B2 "Ҳ" */
    0xc6, 0x88, 0xa1, 0xc1, 0x5, 0x1b, 0x22, 0x82,
    0x4, 0x8,

    /* U+04B3 "ҳ" */
    0xcd, 0x23, 0xc, 0x71, 0x28, 0xc1,

    /* U+04B4 "Ҵ" */
    0xfe, 0x42, 0x8, 0x41, 0x8, 0x21, 0x4, 0x20,
    0x84, 0x10, 0x82, 0x1f, 0xe0, 0x4, 0x0, 0x80,

    /* U+04B5 "ҵ" */
    0xfd, 0x8, 0x84, 0x42, 0x21, 0x10, 0x88, 0x7e,
    0x1,

    /* U+04B6 "Ҷ" */
    0x85, 0xa, 0x14, 0x27, 0xc0, 0x81, 0x2, 0x6,
    0x4, 0x8,

    /* U+04B7 "ҷ" */
    0x8a, 0x28, 0x9e, 0x8, 0x20, 0xc1,

    /* U+04B8 "Ҹ" */
    0x86, 0x19, 0x75, 0x7c, 0x51, 0x41, 0x4,

    /* U+04B9 "ҹ" */
    0x8c, 0x6a, 0xf2, 0x84, 0x20,

    /* U+04BA "Һ" */
    0x82, 0x8, 0x20, 0xfa, 0x18, 0x61, 0x84,

    /* U+04BB "һ" */
    0x84, 0x2d, 0x98, 0xc6, 0x31, 0x88,

    /* U+04BC "Ҽ" */
    0x7, 0x81, 0x1a, 0x41, 0x3f, 0xe1, 0x0, 0x20,
    0x6, 0x0, 0x42, 0x7, 0x80,

    /* U+04BD "ҽ" */
    0x1e, 0xb3, 0xa1, 0x7f, 0x20, 0x30, 0x1f,

    /* U+04BE "Ҿ" */
    0x7, 0x81, 0x1a, 0x41, 0x48, 0x27, 0xfc, 0x20,
    0x4, 0x0, 0x42, 0x7, 0x80, 0x40, 0x8, 0x1,
    0x0,

    /* U+04BF "ҿ" */
    0x1e, 0xb3, 0xa1, 0x7f, 0x20, 0x30, 0x1f, 0x4,
    0x4,

    /* U+04C0 "Ӏ" */
    0xff, 0x80,

    /* U+04C1 "Ӂ" */
    0x4, 0x0, 0x38, 0x0, 0x0, 0x84, 0x22, 0x22,
    0x9, 0x20, 0x2a, 0x2, 0xe8, 0x13, 0x41, 0x11,
    0x18, 0x8c, 0x84, 0x20,

    /* U+04C2 "ӂ" */
    0x12, 0x3, 0x0, 0x3, 0x22, 0x69, 0xa, 0xc3,
    0xf1, 0x26, 0x48, 0xa2, 0x10,

    /* U+04C3 "Ӄ" */
    0x85, 0x12, 0x45, 0xf, 0x90, 0xa1, 0x42, 0x84,
    0x8, 0x10, 0xc0,

    /* U+04C4 "ӄ" */
    0x8a, 0x4a, 0x3e, 0x8a, 0x28, 0x82, 0x8, 0xc0,

    /* U+04C5 "Ӆ" */
    0x3e, 0x22, 0x22, 0x22, 0x22, 0x22, 0x62, 0x42,
    0x83, 0x1, 0x2, 0x2,

    /* U+04C6 "ӆ" */
    0x3c, 0x48, 0x91, 0x22, 0x48, 0xb1, 0x81, 0x4,
    0x8,

    /* U+04C7 "Ӈ" */
    0x83, 0x6, 0xc, 0x1f, 0xf0, 0x60, 0xc1, 0x82,
    0x4, 0x8, 0x60,

    /* U+04C8 "ӈ" */
    0x86, 0x18, 0x7f, 0x86, 0x18, 0x41, 0x4, 0x60,

    /* U+04C9 "Ӊ" */
    0x82, 0x82, 0x82, 0x82, 0xfe, 0x82, 0x82, 0x82,
    0x83, 0x1, 0x2, 0x2,

    /* U+04CA "ӊ" */
    0x85, 0xa, 0x17, 0xe8, 0x50, 0xa1, 0x81, 0x4,
    0x8,

    /* U+04CB "Ӌ" */
    0x86, 0x18, 0x61, 0x7c, 0x10, 0x41, 0xc, 0x20,
    0x80,

    /* U+04CC "ӌ" */
    0x8c, 0x62, 0xf0, 0x84, 0x62,

    /* U+04CD "Ӎ" */
    0xc3, 0x61, 0xa9, 0x54, 0xaa, 0x54, 0xca, 0x65,
    0x2, 0x81, 0x80, 0x40, 0x40, 0x40,

    /* U+04CE "ӎ" */
    0xc6, 0xc6, 0xaa, 0xaa, 0xaa, 0x92, 0x83, 0x1,
    0x2, 0x2,

    /* U+04CF "ӏ" */
    0xff, 0x80,

    /* U+04D0 "Ӑ" */
    0x24, 0x18, 0x0, 0x18, 0x18, 0x2c, 0x24, 0x64,
    0x46, 0x7e, 0xc3, 0x81,

    /* U+04D1 "ӑ" */
    0x49, 0xe0, 0x1e, 0xc, 0x17, 0xe1, 0x8d, 0xd0,

    /* U+04D2 "Ӓ" */
    0x28, 0x0, 0x41, 0x42, 0x85, 0x1b, 0x22, 0x7d,
    0x8e, 0x8,

    /* U+04D3 "ӓ" */
    0x50, 0xf, 0xa3, 0x5, 0xf8, 0x63, 0x74,

    /* U+04D4 "Ӕ" */
    0x1f, 0xc5, 0x3, 0x40, 0x90, 0x27, 0xd9, 0x7,
    0xc3, 0x10, 0x87, 0xc0,

    /* U+04D5 "ӕ" */
    0xf3, 0x83, 0x37, 0x86, 0x3f, 0x88, 0x27, 0x7,
    0x7c,

    /* U+04D6 "Ӗ" */
    0x3, 0x81, 0xf8, 0x42, 0x1f, 0x84, 0x21, 0xf0,

    /* U+04D7 "ӗ" */
    0x48, 0xc0, 0x1e, 0xce, 0x1f, 0xe0, 0xc1, 0xf0,

    /* U+04D8 "Ә" */
    0x7c, 0x82, 0x1, 0x1, 0xff, 0x81, 0x81, 0x42,
    0x3c,

    /* U+04D9 "ә" */
    0xf8, 0x30, 0x7f, 0x87, 0x37, 0x80,

    /* U+04DA "Ӛ" */
    0x24, 0x0, 0x7c, 0x82, 0x1, 0x1, 0xff, 0x81,
    0x81, 0x42, 0x3c,

    /* U+04DB "ӛ" */
    0x50, 0x0, 0x3e, 0xc, 0x1f, 0xe1, 0xcd, 0xe0,

    /* U+04DC "Ӝ" */
    0x5, 0x0, 0x0, 0x62, 0x32, 0x22, 0x12, 0x40,
    0xa8, 0x1f, 0xc1, 0x24, 0x22, 0x26, 0x23, 0x42,
    0x10,

    /* U+04DD "ӝ" */
    0x14, 0x0, 0x32, 0x4d, 0x62, 0xa1, 0xf1, 0x24,
    0x92, 0x88, 0x80,

    /* U+04DE "Ӟ" */
    0x50, 0xf, 0x81, 0x5, 0xe0, 0xc1, 0x4, 0x3f,
    0x0,

    /* U+04DF "ӟ" */
    0x50, 0x1, 0xe0, 0x85, 0xc1, 0xf, 0x80,

    /* U+04E0 "Ӡ" */
    0xfc, 0x21, 0xe, 0xc, 0x10, 0x63, 0xf8,

    /* U+04E1 "ӡ" */
    0x7c, 0x21, 0xc, 0x38, 0x30, 0x41, 0x8f, 0xe0,

    /* U+04E2 "Ӣ" */
    0x78, 0x2, 0x1c, 0x38, 0xb3, 0x64, 0xd9, 0xa3,
    0x87, 0x8,

    /* U+04E3 "ӣ" */
    0x78, 0x8, 0xe3, 0x96, 0xda, 0x71, 0xc4,

    /* U+04E4 "Ӥ" */
    0x28, 0x2, 0x1c, 0x78, 0xb3, 0x64, 0xd9, 0xa3,
    0xc7, 0x8,

    /* U+04E5 "ӥ" */
    0x50, 0x9, 0xe7, 0x96, 0xda, 0x71, 0xc4,

    /* U+04E6 "Ӧ" */
    0x24, 0x0, 0x3c, 0x42, 0x81, 0x81, 0x81, 0x81,
    0x81, 0x42, 0x3c,

    /* U+04E7 "ӧ" */
    0x48, 0x0, 0x1e, 0xce, 0x18, 0x61, 0xcd, 0xe0,

    /* U+04E8 "Ө" */
    0x3c, 0x42, 0x81, 0xff, 0x81, 0x81, 0xc3, 0x42,
    0x3c,

    /* U+04E9 "ө" */
    0x7b, 0x38, 0x7f, 0x87, 0x37, 0x80,

    /* U+04EA "Ӫ" */
    0x24, 0x0, 0x3c, 0x42, 0x81, 0xff, 0x81, 0x81,
    0xc3, 0x42, 0x3c,

    /* U+04EB "ӫ" */
    0x48, 0x0, 0x1e, 0xce, 0x1f, 0xe1, 0xcd, 0xe0,

    /* U+04EC "Ӭ" */
    0x50, 0x1, 0xe4, 0x60, 0x60, 0x5f, 0x81, 0x7,
    0x19, 0xe0,

    /* U+04ED "ӭ" */
    0x50, 0x0, 0x3c, 0x88, 0x37, 0xc3, 0xf, 0xe0,

    /* U+04EE "Ӯ" */
    0x38, 0x3, 0x1a, 0x24, 0x45, 0x8a, 0x1c, 0x10,
    0x21, 0x80,

    /* U+04EF "ӯ" */
    0x38, 0x1, 0x1a, 0x24, 0x45, 0xa, 0x14, 0x10,
    0x20, 0xc3, 0x0,

    /* U+04F0 "Ӱ" */
    0x28, 0x3, 0x1a, 0x24, 0x45, 0xa, 0x1c, 0x10,
    0x21, 0x80,

    /* U+04F1 "ӱ" */
    0x50, 0x23, 0x19, 0xa9, 0x4a, 0x21, 0x19, 0x80,

    /* U+04F2 "Ӳ" */
    0x28, 0x3, 0x1a, 0x24, 0x45, 0x8a, 0x1c, 0x10,
    0x21, 0x80,

    /* U+04F3 "ӳ" */
    0x14, 0x51, 0x1a, 0x24, 0x45, 0xa, 0x14, 0x10,
    0x20, 0xc3, 0x0,

    /* U+04F4 "Ӵ" */
    0x28, 0x8, 0x61, 0x86, 0x17, 0xc1, 0x4, 0x10,
    0x40,

    /* U+04F5 "ӵ" */
    0x50, 0x23, 0x18, 0xbc, 0x21, 0x8,

    /* U+04F6 "Ӷ" */
    0xfc, 0x21, 0x8, 0x42, 0x10, 0xc2, 0x10,

    /* U+04F7 "ӷ" */
    0xf8, 0x88, 0x88, 0xc4,

    /* U+04F8 "Ӹ" */
    0x14, 0x0, 0x81, 0x81, 0x81, 0x81, 0xf9, 0x85,
    0x85, 0x85, 0xf9,

    /* U+04F9 "ӹ" */
    0x28, 0x2, 0xc, 0x18, 0x3e, 0x62, 0xc5, 0xf2,

    /* U+04FA "Ӻ" */
    0x3e, 0x40, 0x81, 0xf, 0xc4, 0x8, 0x10, 0x38,
    0x10, 0x21, 0x80,

    /* U+04FB "ӻ" */
    0x3c, 0x82, 0x3e, 0x20, 0x83, 0x82, 0x8, 0xc0,

    /* U+04FC "Ӽ" */
    0xc6, 0x88, 0xa1, 0xc3, 0x5, 0x12, 0x22, 0x86,
    0x4, 0x18, 0x60,

    /* U+04FD "ӽ" */
    0xc5, 0x23, 0xc, 0x31, 0x2c, 0x41, 0x4, 0x60,

    /* U+04FE "Ӿ" */
    0x42, 0x26, 0x34, 0x18, 0x7e, 0x18, 0x24, 0x62,
    0x43,

    /* U+04FF "ӿ" */
    0x44, 0x58, 0xa3, 0xe2, 0x8d, 0x91, 0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 61, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 77, .box_w = 1, .box_h = 9, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 3, .adv_w = 88, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 5, .adv_w = 161, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 14, .adv_w = 122, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 23, .adv_w = 182, .box_w = 10, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 35, .adv_w = 150, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 44, .adv_w = 53, .box_w = 1, .box_h = 3, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 45, .adv_w = 75, .box_w = 3, .box_h = 11, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 50, .adv_w = 75, .box_w = 3, .box_h = 11, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 55, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 59, .adv_w = 161, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 67, .adv_w = 61, .box_w = 1, .box_h = 3, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 68, .adv_w = 69, .box_w = 3, .box_h = 1, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 69, .adv_w = 61, .box_w = 1, .box_h = 1, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 70, .adv_w = 65, .box_w = 4, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 76, .adv_w = 122, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 83, .adv_w = 122, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 89, .adv_w = 122, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 95, .adv_w = 122, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 102, .adv_w = 122, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 109, .adv_w = 122, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 116, .adv_w = 122, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 123, .adv_w = 122, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 130, .adv_w = 122, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 137, .adv_w = 122, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 144, .adv_w = 65, .box_w = 1, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 145, .adv_w = 65, .box_w = 1, .box_h = 8, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 146, .adv_w = 161, .box_w = 8, .box_h = 6, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 152, .adv_w = 161, .box_w = 8, .box_h = 4, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 156, .adv_w = 161, .box_w = 8, .box_h = 6, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 162, .adv_w = 102, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 167, .adv_w = 192, .box_w = 10, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 181, .adv_w = 131, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 190, .adv_w = 132, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 197, .adv_w = 134, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 205, .adv_w = 148, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 213, .adv_w = 121, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 219, .adv_w = 110, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 225, .adv_w = 149, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 234, .adv_w = 144, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 242, .adv_w = 57, .box_w = 1, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 244, .adv_w = 57, .box_w = 3, .box_h = 12, .ofs_x = -1, .ofs_y = -3},
    {.bitmap_index = 249, .adv_w = 126, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 257, .adv_w = 107, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 263, .adv_w = 166, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 272, .adv_w = 144, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 280, .adv_w = 151, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 289, .adv_w = 116, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 296, .adv_w = 151, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 307, .adv_w = 133, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 315, .adv_w = 122, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 322, .adv_w = 117, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 330, .adv_w = 141, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 338, .adv_w = 131, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 347, .adv_w = 190, .box_w = 11, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 360, .adv_w = 132, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 369, .adv_w = 117, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 377, .adv_w = 132, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 385, .adv_w = 75, .box_w = 2, .box_h = 11, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 388, .adv_w = 65, .box_w = 4, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 394, .adv_w = 75, .box_w = 2, .box_h = 11, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 397, .adv_w = 161, .box_w = 8, .box_h = 4, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 401, .adv_w = 96, .box_w = 6, .box_h = 1, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 402, .adv_w = 96, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 404, .adv_w = 118, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 410, .adv_w = 122, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 418, .adv_w = 106, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 423, .adv_w = 122, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 431, .adv_w = 118, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 437, .adv_w = 68, .box_w = 4, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 442, .adv_w = 122, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 450, .adv_w = 122, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 457, .adv_w = 53, .box_w = 1, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 459, .adv_w = 53, .box_w = 2, .box_h = 13, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 463, .adv_w = 111, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 471, .adv_w = 53, .box_w = 1, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 473, .adv_w = 187, .box_w = 9, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 481, .adv_w = 122, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 486, .adv_w = 117, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 492, .adv_w = 122, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 500, .adv_w = 122, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 508, .adv_w = 79, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 512, .adv_w = 100, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 517, .adv_w = 75, .box_w = 4, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 522, .adv_w = 122, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 527, .adv_w = 114, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 534, .adv_w = 157, .box_w = 9, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 542, .adv_w = 114, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 549, .adv_w = 114, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 558, .adv_w = 101, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 564, .adv_w = 122, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 572, .adv_w = 65, .box_w = 1, .box_h = 13, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 574, .adv_w = 122, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 582, .adv_w = 161, .box_w = 8, .box_h = 2, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 584, .adv_w = 121, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 592, .adv_w = 121, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 599, .adv_w = 151, .box_w = 8, .box_h = 12, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 611, .adv_w = 117, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 619, .adv_w = 134, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 627, .adv_w = 122, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 634, .adv_w = 57, .box_w = 1, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 636, .adv_w = 57, .box_w = 3, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 641, .adv_w = 57, .box_w = 3, .box_h = 12, .ofs_x = -1, .ofs_y = -3},
    {.bitmap_index = 646, .adv_w = 210, .box_w = 12, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 660, .adv_w = 201, .box_w = 10, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 672, .adv_w = 151, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 683, .adv_w = 136, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 694, .adv_w = 144, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 705, .adv_w = 117, .box_w = 7, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 716, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 726, .adv_w = 131, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 735, .adv_w = 132, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 742, .adv_w = 132, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 749, .adv_w = 117, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 755, .adv_w = 150, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 766, .adv_w = 121, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 772, .adv_w = 207, .box_w = 13, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 787, .adv_w = 123, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 794, .adv_w = 144, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 802, .adv_w = 144, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 813, .adv_w = 136, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 821, .adv_w = 144, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 829, .adv_w = 166, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 838, .adv_w = 144, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 846, .adv_w = 151, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 855, .adv_w = 144, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 863, .adv_w = 116, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 870, .adv_w = 134, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 878, .adv_w = 117, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 886, .adv_w = 117, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 894, .adv_w = 165, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 905, .adv_w = 132, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 914, .adv_w = 149, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 925, .adv_w = 132, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 932, .adv_w = 205, .box_w = 11, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 945, .adv_w = 210, .box_w = 11, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 961, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 972, .adv_w = 169, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 981, .adv_w = 132, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 988, .adv_w = 134, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 996, .adv_w = 207, .box_w = 11, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1009, .adv_w = 133, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1016, .adv_w = 118, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1022, .adv_w = 118, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1030, .adv_w = 113, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1035, .adv_w = 101, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1040, .adv_w = 133, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1047, .adv_w = 118, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1053, .adv_w = 173, .box_w = 9, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1061, .adv_w = 102, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1066, .adv_w = 125, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1072, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1080, .adv_w = 116, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1086, .adv_w = 123, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1092, .adv_w = 145, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1099, .adv_w = 126, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1105, .adv_w = 117, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1111, .adv_w = 126, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1117, .adv_w = 122, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1125, .adv_w = 106, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1130, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1137, .adv_w = 114, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 1146, .adv_w = 164, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1160, .adv_w = 114, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1167, .adv_w = 131, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1174, .adv_w = 113, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1179, .adv_w = 176, .box_w = 9, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1187, .adv_w = 181, .box_w = 10, .box_h = 8, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1197, .adv_w = 136, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1204, .adv_w = 152, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1211, .adv_w = 113, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1216, .adv_w = 105, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1221, .adv_w = 162, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1228, .adv_w = 116, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1233, .adv_w = 118, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1241, .adv_w = 118, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1249, .adv_w = 120, .box_w = 6, .box_h = 12, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 1258, .adv_w = 101, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1265, .adv_w = 105, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1270, .adv_w = 100, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1275, .adv_w = 53, .box_w = 1, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1277, .adv_w = 53, .box_w = 3, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1281, .adv_w = 53, .box_w = 2, .box_h = 12, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 1284, .adv_w = 173, .box_w = 10, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1293, .adv_w = 173, .box_w = 9, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1301, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1308, .adv_w = 116, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1316, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1324, .adv_w = 114, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1333, .adv_w = 126, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1340, .adv_w = 179, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1351, .adv_w = 161, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1358, .adv_w = 148, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1367, .adv_w = 129, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1376, .adv_w = 181, .box_w = 10, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1388, .adv_w = 144, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1395, .adv_w = 169, .box_w = 11, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1408, .adv_w = 150, .box_w = 9, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1416, .adv_w = 223, .box_w = 13, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1431, .adv_w = 192, .box_w = 11, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1441, .adv_w = 151, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1452, .adv_w = 117, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1459, .adv_w = 197, .box_w = 10, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1471, .adv_w = 158, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1478, .adv_w = 122, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1489, .adv_w = 104, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1496, .adv_w = 164, .box_w = 10, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1508, .adv_w = 168, .box_w = 10, .box_h = 12, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 1523, .adv_w = 151, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1532, .adv_w = 117, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1538, .adv_w = 150, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1549, .adv_w = 128, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1556, .adv_w = 150, .box_w = 9, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1569, .adv_w = 128, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1579, .adv_w = 191, .box_w = 10, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1594, .adv_w = 174, .box_w = 10, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1607, .adv_w = 183, .box_w = 10, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1619, .adv_w = 146, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1626, .adv_w = 227, .box_w = 13, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1646, .adv_w = 197, .box_w = 10, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1658, .adv_w = 179, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1672, .adv_w = 161, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1681, .adv_w = 134, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1692, .adv_w = 106, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1699, .adv_w = 96, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1705, .adv_w = 0, .box_w = 5, .box_h = 3, .ofs_x = -6, .ofs_y = 7},
    {.bitmap_index = 1707, .adv_w = 0, .box_w = 4, .box_h = 2, .ofs_x = -4, .ofs_y = 8},
    {.bitmap_index = 1708, .adv_w = 0, .box_w = 1, .box_h = 3, .ofs_x = -3, .ofs_y = 7},
    {.bitmap_index = 1709, .adv_w = 0, .box_w = 1, .box_h = 3, .ofs_x = -3, .ofs_y = 7},
    {.bitmap_index = 1710, .adv_w = 0, .box_w = 9, .box_h = 2, .ofs_x = -9, .ofs_y = 8},
    {.bitmap_index = 1713, .adv_w = 80, .box_w = 17, .box_h = 14, .ofs_x = -12, .ofs_y = -2},
    {.bitmap_index = 1743, .adv_w = 80, .box_w = 16, .box_h = 16, .ofs_x = -11, .ofs_y = -3},
    {.bitmap_index = 1775, .adv_w = 148, .box_w = 8, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1789, .adv_w = 130, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1800, .adv_w = 132, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1808, .adv_w = 113, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1815, .adv_w = 116, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1822, .adv_w = 122, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1830, .adv_w = 117, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1837, .adv_w = 101, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1843, .adv_w = 130, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1851, .adv_w = 113, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1857, .adv_w = 120, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1866, .adv_w = 102, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1873, .adv_w = 207, .box_w = 13, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1891, .adv_w = 173, .box_w = 10, .box_h = 8, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1901, .adv_w = 123, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1910, .adv_w = 102, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1917, .adv_w = 136, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1927, .adv_w = 116, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1933, .adv_w = 136, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1941, .adv_w = 116, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1947, .adv_w = 136, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1956, .adv_w = 116, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1964, .adv_w = 164, .box_w = 10, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1976, .adv_w = 160, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1983, .adv_w = 144, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1994, .adv_w = 127, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 2001, .adv_w = 195, .box_w = 11, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2014, .adv_w = 168, .box_w = 9, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2022, .adv_w = 208, .box_w = 11, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2039, .adv_w = 176, .box_w = 10, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2052, .adv_w = 169, .box_w = 10, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2065, .adv_w = 133, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2072, .adv_w = 134, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2083, .adv_w = 106, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2090, .adv_w = 117, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 2100, .adv_w = 112, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 2106, .adv_w = 117, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2114, .adv_w = 114, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 2123, .adv_w = 117, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2132, .adv_w = 114, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 2140, .adv_w = 132, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 2150, .adv_w = 114, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 2156, .adv_w = 179, .box_w = 11, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 2172, .adv_w = 155, .box_w = 9, .box_h = 8, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 2181, .adv_w = 132, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 2191, .adv_w = 113, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 2197, .adv_w = 132, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2204, .adv_w = 113, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2209, .adv_w = 132, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2216, .adv_w = 122, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2222, .adv_w = 181, .box_w = 11, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2235, .adv_w = 140, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2242, .adv_w = 181, .box_w = 11, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2259, .adv_w = 140, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 2268, .adv_w = 57, .box_w = 1, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2270, .adv_w = 207, .box_w = 13, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2290, .adv_w = 173, .box_w = 10, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2303, .adv_w = 126, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2314, .adv_w = 116, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2322, .adv_w = 149, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2334, .adv_w = 129, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2343, .adv_w = 144, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2354, .adv_w = 127, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2362, .adv_w = 149, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2374, .adv_w = 131, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2383, .adv_w = 132, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 2392, .adv_w = 113, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 2397, .adv_w = 170, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2411, .adv_w = 149, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2421, .adv_w = 53, .box_w = 1, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2423, .adv_w = 131, .box_w = 8, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2435, .adv_w = 118, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2443, .adv_w = 131, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2453, .adv_w = 118, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2460, .adv_w = 187, .box_w = 10, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2472, .adv_w = 189, .box_w = 10, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2481, .adv_w = 121, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2489, .adv_w = 118, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2497, .adv_w = 151, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2506, .adv_w = 118, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2512, .adv_w = 151, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2523, .adv_w = 118, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2531, .adv_w = 207, .box_w = 12, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2548, .adv_w = 173, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2559, .adv_w = 123, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2568, .adv_w = 102, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2575, .adv_w = 128, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2582, .adv_w = 111, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 2590, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2600, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2607, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2617, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2624, .adv_w = 151, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2635, .adv_w = 117, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2643, .adv_w = 151, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2652, .adv_w = 117, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2658, .adv_w = 151, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2669, .adv_w = 117, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2677, .adv_w = 134, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2687, .adv_w = 105, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2695, .adv_w = 117, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2705, .adv_w = 114, .box_w = 7, .box_h = 12, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 2716, .adv_w = 117, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2726, .adv_w = 114, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2734, .adv_w = 117, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2744, .adv_w = 114, .box_w = 7, .box_h = 12, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 2755, .adv_w = 132, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2764, .adv_w = 113, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2770, .adv_w = 117, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 2777, .adv_w = 101, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 2781, .adv_w = 169, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2792, .adv_w = 152, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2800, .adv_w = 130, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2811, .adv_w = 113, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2819, .adv_w = 132, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2830, .adv_w = 114, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 2838, .adv_w = 132, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2847, .adv_w = 114, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 1024, .range_length = 256, .glyph_id_start = 96,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
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
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 2,
    .bpp = 1,
    .kern_classes = 0,
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
const lv_font_t dejavu_16 = {
#else
lv_font_t dejavu_16 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 16,          /*The maximum line height required by the font*/
    .base_line = 3,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .static_bitmap = 0,
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if DEJAVU_16*/
