/* This is an implementation of wcwidth()  (defined in IEEE Std 1002.1-2001)
 * for Unicode.
 *
 * http://www.opengroup.org/onlinepubs/007904975/functions/wcwidth.html
 *
 * description:
 * In fixed-width output devices, Latin characters all occupy a single "cell"
 * position of equal width, whereas ideographic CJK characters occupy two such
 * cells. Interoperability between terminal-line applications and
 * (teletype-style) character terminals using the UTF-8 encoding requires
 * agreement on which character should advance the cursor by how many cell
 * positions.
 *
 * http://www.unicode.org/unicode/reports/tr11/
 *
 * Markus Kuhn -- 2007-05-26 (Unicode 5.0)
 *
 * Permission to use, copy, modify, and distribute this software
 * for any purpose and without fee is hereby granted. The author
 * disclaims all warranties with regard to this software.
 *
 * Latest version: http://www.cl.cam.ac.uk/~mgk25/ucs/wcwidth.c
 */

#include <stdint.h>
#include <string.h>
#include <linecook/xwcwidth9.h>

struct xwc9_ival {
  uint16_t first;
  uint16_t last;
};

static const struct xwc9_ival combining1[] = {
  {0x0300, 0x036f}, /* 0 prefix */
  {0x0483, 0x0489},
  {0x0591, 0x05bd},
  {0x05bf, 0x05bf},
  {0x05c1, 0x05c2},
  {0x05c4, 0x05c5},
  {0x05c7, 0x05c7},
  {0x0610, 0x061a},
  {0x064b, 0x065f},
  {0x0670, 0x0670},
  {0x06d6, 0x06dc},
  {0x06df, 0x06e4},
  {0x06e7, 0x06e8},
  {0x06ea, 0x06ed},
  {0x0711, 0x0711},
  {0x0730, 0x074a},
  {0x07a6, 0x07b0},
  {0x07eb, 0x07f3},
  {0x0816, 0x0819},
  {0x081b, 0x0823},
  {0x0825, 0x0827},
  {0x0829, 0x082d},
  {0x0859, 0x085b},
  {0x08d4, 0x08e1},
  {0x08e3, 0x0903},
  {0x093a, 0x093c},
  {0x093e, 0x094f},
  {0x0951, 0x0957},
  {0x0962, 0x0963},
  {0x0981, 0x0983},
  {0x09bc, 0x09bc},
  {0x09be, 0x09c4},
  {0x09c7, 0x09c8},
  {0x09cb, 0x09cd},
  {0x09d7, 0x09d7},
  {0x09e2, 0x09e3},
  {0x0a01, 0x0a03},
  {0x0a3c, 0x0a3c},
  {0x0a3e, 0x0a42},
  {0x0a47, 0x0a48},
  {0x0a4b, 0x0a4d},
  {0x0a51, 0x0a51},
  {0x0a70, 0x0a71},
  {0x0a75, 0x0a75},
  {0x0a81, 0x0a83},
  {0x0abc, 0x0abc},
  {0x0abe, 0x0ac5},
  {0x0ac7, 0x0ac9},
  {0x0acb, 0x0acd},
  {0x0ae2, 0x0ae3},
  {0x0b01, 0x0b03},
  {0x0b3c, 0x0b3c},
  {0x0b3e, 0x0b44},
  {0x0b47, 0x0b48},
  {0x0b4b, 0x0b4d},
  {0x0b56, 0x0b57},
  {0x0b62, 0x0b63},
  {0x0b82, 0x0b82},
  {0x0bbe, 0x0bc2},
  {0x0bc6, 0x0bc8},
  {0x0bca, 0x0bcd},
  {0x0bd7, 0x0bd7},
  {0x0c00, 0x0c03},
  {0x0c3e, 0x0c44},
  {0x0c46, 0x0c48},
  {0x0c4a, 0x0c4d},
  {0x0c55, 0x0c56},
  {0x0c62, 0x0c63},
  {0x0c81, 0x0c83},
  {0x0cbc, 0x0cbc},
  {0x0cbe, 0x0cc4},
  {0x0cc6, 0x0cc8},
  {0x0cca, 0x0ccd},
  {0x0cd5, 0x0cd6},
  {0x0ce2, 0x0ce3},
  {0x0d01, 0x0d03},
  {0x0d3e, 0x0d44},
  {0x0d46, 0x0d48},
  {0x0d4a, 0x0d4d},
  {0x0d57, 0x0d57},
  {0x0d62, 0x0d63},
  {0x0d82, 0x0d83},
  {0x0dca, 0x0dca},
  {0x0dcf, 0x0dd4},
  {0x0dd6, 0x0dd6},
  {0x0dd8, 0x0ddf},
  {0x0df2, 0x0df3},
  {0x0e31, 0x0e31},
  {0x0e34, 0x0e3a},
  {0x0e47, 0x0e4e},
  {0x0eb1, 0x0eb1},
  {0x0eb4, 0x0eb9},
  {0x0ebb, 0x0ebc},
  {0x0ec8, 0x0ecd},
  {0x0f18, 0x0f19},
  {0x0f35, 0x0f35},
  {0x0f37, 0x0f37},
  {0x0f39, 0x0f39},
  {0x0f3e, 0x0f3f},
  {0x0f71, 0x0f84},
  {0x0f86, 0x0f87},
  {0x0f8d, 0x0f97},
  {0x0f99, 0x0fbc},
  {0x0fc6, 0x0fc6},
  {0x102b, 0x103e},
  {0x1056, 0x1059},
  {0x105e, 0x1060},
  {0x1062, 0x1064},
  {0x1067, 0x106d},
  {0x1071, 0x1074},
  {0x1082, 0x108d},
  {0x108f, 0x108f},
  {0x109a, 0x109d},
  {0x135d, 0x135f},
  {0x1712, 0x1714},
  {0x1732, 0x1734},
  {0x1752, 0x1753},
  {0x1772, 0x1773},
  {0x17b4, 0x17d3},
  {0x17dd, 0x17dd},
  {0x180b, 0x180d},
  {0x1885, 0x1886},
  {0x18a9, 0x18a9},
  {0x1920, 0x192b},
  {0x1930, 0x193b},
  {0x1a17, 0x1a1b},
  {0x1a55, 0x1a5e},
  {0x1a60, 0x1a7c},
  {0x1a7f, 0x1a7f},
  {0x1ab0, 0x1abe},
  {0x1b00, 0x1b04},
  {0x1b34, 0x1b44},
  {0x1b6b, 0x1b73},
  {0x1b80, 0x1b82},
  {0x1ba1, 0x1bad},
  {0x1be6, 0x1bf3},
  {0x1c24, 0x1c37},
  {0x1cd0, 0x1cd2},
  {0x1cd4, 0x1ce8},
  {0x1ced, 0x1ced},
  {0x1cf2, 0x1cf4},
  {0x1cf8, 0x1cf9},
  {0x1dc0, 0x1df5},
  {0x1dfb, 0x1dff},
  {0x20d0, 0x20f0},
  {0x2cef, 0x2cf1},
  {0x2d7f, 0x2d7f},
  {0x2de0, 0x2dff},
  {0x302a, 0x302f},
  {0x3099, 0x309a},
  {0xa66f, 0xa672},
  {0xa674, 0xa67d},
  {0xa69e, 0xa69f},
  {0xa6f0, 0xa6f1},
  {0xa802, 0xa802},
  {0xa806, 0xa806},
  {0xa80b, 0xa80b},
  {0xa823, 0xa827},
  {0xa880, 0xa881},
  {0xa8b4, 0xa8c5},
  {0xa8e0, 0xa8f1},
  {0xa926, 0xa92d},
  {0xa947, 0xa953},
  {0xa980, 0xa983},
  {0xa9b3, 0xa9c0},
  {0xa9e5, 0xa9e5},
  {0xaa29, 0xaa36},
  {0xaa43, 0xaa43},
  {0xaa4c, 0xaa4d},
  {0xaa7b, 0xaa7d},
  {0xaab0, 0xaab0},
  {0xaab2, 0xaab4},
  {0xaab7, 0xaab8},
  {0xaabe, 0xaabf},
  {0xaac1, 0xaac1},
  {0xaaeb, 0xaaef},
  {0xaaf5, 0xaaf6},
  {0xabe3, 0xabea},
  {0xabec, 0xabed},
  {0xfb1e, 0xfb1e},
  {0xfe00, 0xfe0f},
  {0xfe20, 0xfe2f},
};

static const struct xwc9_ival combining2[] = {
  {0x01fd, 0x01fd}, /* 0x1 prefix */
  {0x02e0, 0x02e0},
  {0x0376, 0x037a},
  {0x0a01, 0x0a03},
  {0x0a05, 0x0a06},
  {0x0a0c, 0x0a0f},
  {0x0a38, 0x0a3a},
  {0x0a3f, 0x0a3f},
  {0x0ae5, 0x0ae6},
  {0x1000, 0x1002},
  {0x1038, 0x1046},
  {0x107f, 0x1082},
  {0x10b0, 0x10ba},
  {0x1100, 0x1102},
  {0x1127, 0x1134},
  {0x1173, 0x1173},
  {0x1180, 0x1182},
  {0x11b3, 0x11c0},
  {0x11ca, 0x11cc},
  {0x122c, 0x1237},
  {0x123e, 0x123e},
  {0x12df, 0x12ea},
  {0x1300, 0x1303},
  {0x133c, 0x133c},
  {0x133e, 0x1344},
  {0x1347, 0x1348},
  {0x134b, 0x134d},
  {0x1357, 0x1357},
  {0x1362, 0x1363},
  {0x1366, 0x136c},
  {0x1370, 0x1374},
  {0x1435, 0x1446},
  {0x14b0, 0x14c3},
  {0x15af, 0x15b5},
  {0x15b8, 0x15c0},
  {0x15dc, 0x15dd},
  {0x1630, 0x1640},
  {0x16ab, 0x16b7},
  {0x171d, 0x172b},
  {0x1c2f, 0x1c36},
  {0x1c38, 0x1c3f},
  {0x1c92, 0x1ca7},
  {0x1ca9, 0x1cb6},
  {0x6af0, 0x6af4},
  {0x6b30, 0x6b36},
  {0x6f51, 0x6f7e},
  {0x6f8f, 0x6f92},
  {0xbc9d, 0xbc9e},
  {0xd165, 0xd169},
  {0xd16d, 0xd172},
  {0xd17b, 0xd182},
  {0xd185, 0xd18b},
  {0xd1aa, 0xd1ad},
  {0xd242, 0xd244},
  {0xda00, 0xda36},
  {0xda3b, 0xda6c},
  {0xda75, 0xda75},
  {0xda84, 0xda84},
  {0xda9b, 0xda9f},
  {0xdaa1, 0xdaaf},
  {0xe000, 0xe006},
  {0xe008, 0xe018},
  {0xe01b, 0xe021},
  {0xe023, 0xe024},
  {0xe026, 0xe02a},
  {0xe8d0, 0xe8d6},
  {0xe944, 0xe94a},
};

static const struct xwc9_ival combining3[] = {
  {0x0100, 0x01ef}, /* 0xe prefix */
};

static const xwc9_ival nonprint[] = {
  {0x0000, 0x001f}, /* 0 prefix */
  {0x007f, 0x009f},
  {0x00ad, 0x00ad},
  {0x070f, 0x070f},
  {0x180b, 0x180e},
  {0x200b, 0x200f},
  {0x2028, 0x2029},
  {0x202a, 0x202e},
  {0x206a, 0x206f},
  {0xd800, 0xdfff},
  {0xfeff, 0xfeff},
  {0xfff9, 0xfffb},
  {0xfffe, 0xffff},
};

static const xwc9_ival doublewidth1[] = {
  {0x1100, 0x115f}, /* 0 prefix */
  {0x231a, 0x231b},
  {0x2329, 0x232a},
  {0x23e9, 0x23ec},
  {0x23f0, 0x23f0},
  {0x23f3, 0x23f3},
  {0x25fd, 0x25fe},
  {0x2614, 0x2615},
  {0x2648, 0x2653},
  {0x267f, 0x267f},
  {0x2693, 0x2693},
  {0x26a1, 0x26a1},
  {0x26aa, 0x26ab},
  {0x26bd, 0x26be},
  {0x26c4, 0x26c5},
  {0x26ce, 0x26ce},
  {0x26d4, 0x26d4},
  {0x26ea, 0x26ea},
  {0x26f2, 0x26f3},
  {0x26f5, 0x26f5},
  {0x26fa, 0x26fa},
  {0x26fd, 0x26fd},
  {0x2705, 0x2705},
  {0x270a, 0x270b},
  {0x2728, 0x2728},
  {0x274c, 0x274c},
  {0x274e, 0x274e},
  {0x2753, 0x2755},
  {0x2757, 0x2757},
  {0x2795, 0x2797},
  {0x27b0, 0x27b0},
  {0x27bf, 0x27bf},
  {0x2b1b, 0x2b1c},
  {0x2b50, 0x2b50},
  {0x2b55, 0x2b55},
  {0x2e80, 0x2e99},
  {0x2e9b, 0x2ef3},
  {0x2f00, 0x2fd5},
  {0x2ff0, 0x2ffb},
  {0x3000, 0x303e},
  {0x3041, 0x3096},
  {0x3099, 0x30ff},
  {0x3105, 0x312d},
  {0x3131, 0x318e},
  {0x3190, 0x31ba},
  {0x31c0, 0x31e3},
  {0x31f0, 0x321e},
  {0x3220, 0x3247},
  {0x3250, 0x32fe},
  {0x3300, 0x4dbf},
  {0x4e00, 0xa48c},
  {0xa490, 0xa4c6},
  {0xa960, 0xa97c},
  {0xac00, 0xd7a3},
  {0xf900, 0xfaff},
  {0xfe10, 0xfe19},
  {0xfe30, 0xfe52},
  {0xfe54, 0xfe66},
  {0xfe68, 0xfe6b},
  {0xff01, 0xff60},
  {0xffe0, 0xffe6},
};

static const xwc9_ival doublewidth2[] = {
  {0x6fe0, 0x6fe0}, /* 0x1 prefix */
  {0x7000, 0x87ec},
  {0x8800, 0x8af2},
  {0xb000, 0xb001},
  {0xf004, 0xf004},
  {0xf0cf, 0xf0cf},
  {0xf18e, 0xf18e},
  {0xf191, 0xf19a},
  {0xf200, 0xf202},
  {0xf210, 0xf23b},
  {0xf240, 0xf248},
  {0xf250, 0xf251},
  {0xf300, 0xf320},
  {0xf32d, 0xf335},
  {0xf337, 0xf37c},
  {0xf37e, 0xf393},
  {0xf3a0, 0xf3ca},
  {0xf3cf, 0xf3d3},
  {0xf3e0, 0xf3f0},
  {0xf3f4, 0xf3f4},
  {0xf3f8, 0xf43e},
  {0xf440, 0xf440},
  {0xf442, 0xf4fc},
  {0xf4ff, 0xf53d},
  {0xf54b, 0xf54e},
  {0xf550, 0xf567},
  {0xf57a, 0xf57a},
  {0xf595, 0xf596},
  {0xf5a4, 0xf5a4},
  {0xf5fb, 0xf64f},
  {0xf680, 0xf6c5},
  {0xf6cc, 0xf6cc},
  {0xf6d0, 0xf6d2},
  {0xf6eb, 0xf6ec},
  {0xf6f4, 0xf6f6},
  {0xf910, 0xf91e},
  {0xf920, 0xf927},
  {0xf930, 0xf930},
  {0xf933, 0xf93e},
  {0xf940, 0xf94b},
  {0xf950, 0xf95e},
  {0xf980, 0xf991},
  {0xf9c0, 0xf9c0},
};
static const xwc9_ival doublewidth3[] = {
  {0x0000, 0xfffd}, /* 0x2 prefix */
};
static const xwc9_ival doublewidth4[] = {
  {0x0000, 0xfffd}, /* 0x3 prfix */
};
static const xwc9_ival emoji[] = {
  {0xf1e6, 0xf1ff}, /* 0x1 prefix */
  {0xf321, 0xf321},
  {0xf324, 0xf32c},
  {0xf336, 0xf336},
  {0xf37d, 0xf37d},
  {0xf396, 0xf397},
  {0xf399, 0xf39b},
  {0xf39e, 0xf39f},
  {0xf3cb, 0xf3ce},
  {0xf3d4, 0xf3df},
  {0xf3f3, 0xf3f5},
  {0xf3f7, 0xf3f7},
  {0xf43f, 0xf43f},
  {0xf441, 0xf441},
  {0xf4fd, 0xf4fd},
  {0xf549, 0xf54a},
  {0xf56f, 0xf570},
  {0xf573, 0xf579},
  {0xf587, 0xf587},
  {0xf58a, 0xf58d},
  {0xf590, 0xf590},
  {0xf5a5, 0xf5a5},
  {0xf5a8, 0xf5a8},
  {0xf5b1, 0xf5b2},
  {0xf5bc, 0xf5bc},
  {0xf5c2, 0xf5c4},
  {0xf5d1, 0xf5d3},
  {0xf5dc, 0xf5de},
  {0xf5e1, 0xf5e1},
  {0xf5e3, 0xf5e3},
  {0xf5e8, 0xf5e8},
  {0xf5ef, 0xf5ef},
  {0xf5f3, 0xf5f3},
  {0xf5fa, 0xf5fa},
  {0xf6cb, 0xf6cf},
  {0xf6e0, 0xf6e5},
  {0xf6e9, 0xf6e9},
  {0xf6f0, 0xf6f0},
  {0xf6f3, 0xf6f3},
};

static const struct xwc9_ival ambiguous1[] = {
  {0x00a1, 0x00a1}, /* 0 prefix */
  {0x00a4, 0x00a4},
  {0x00a7, 0x00a8},
  {0x00aa, 0x00aa},
  {0x00ad, 0x00ae},
  {0x00b0, 0x00b4},
  {0x00b6, 0x00ba},
  {0x00bc, 0x00bf},
  {0x00c6, 0x00c6},
  {0x00d0, 0x00d0},
  {0x00d7, 0x00d8},
  {0x00de, 0x00e1},
  {0x00e6, 0x00e6},
  {0x00e8, 0x00ea},
  {0x00ec, 0x00ed},
  {0x00f0, 0x00f0},
  {0x00f2, 0x00f3},
  {0x00f7, 0x00fa},
  {0x00fc, 0x00fc},
  {0x00fe, 0x00fe},
  {0x0101, 0x0101},
  {0x0111, 0x0111},
  {0x0113, 0x0113},
  {0x011b, 0x011b},
  {0x0126, 0x0127},
  {0x012b, 0x012b},
  {0x0131, 0x0133},
  {0x0138, 0x0138},
  {0x013f, 0x0142},
  {0x0144, 0x0144},
  {0x0148, 0x014b},
  {0x014d, 0x014d},
  {0x0152, 0x0153},
  {0x0166, 0x0167},
  {0x016b, 0x016b},
  {0x01ce, 0x01ce},
  {0x01d0, 0x01d0},
  {0x01d2, 0x01d2},
  {0x01d4, 0x01d4},
  {0x01d6, 0x01d6},
  {0x01d8, 0x01d8},
  {0x01da, 0x01da},
  {0x01dc, 0x01dc},
  {0x0251, 0x0251},
  {0x0261, 0x0261},
  {0x02c4, 0x02c4},
  {0x02c7, 0x02c7},
  {0x02c9, 0x02cb},
  {0x02cd, 0x02cd},
  {0x02d0, 0x02d0},
  {0x02d8, 0x02db},
  {0x02dd, 0x02dd},
  {0x02df, 0x02df},
  {0x0300, 0x036f},
  {0x0391, 0x03a1},
  {0x03a3, 0x03a9},
  {0x03b1, 0x03c1},
  {0x03c3, 0x03c9},
  {0x0401, 0x0401},
  {0x0410, 0x044f},
  {0x0451, 0x0451},
  {0x2010, 0x2010},
  {0x2013, 0x2016},
  {0x2018, 0x2019},
  {0x201c, 0x201d},
  {0x2020, 0x2022},
  {0x2024, 0x2027},
  {0x2030, 0x2030},
  {0x2032, 0x2033},
  {0x2035, 0x2035},
  {0x203b, 0x203b},
  {0x203e, 0x203e},
  {0x2074, 0x2074},
  {0x207f, 0x207f},
  {0x2081, 0x2084},
  {0x20ac, 0x20ac},
  {0x2103, 0x2103},
  {0x2105, 0x2105},
  {0x2109, 0x2109},
  {0x2113, 0x2113},
  {0x2116, 0x2116},
  {0x2121, 0x2122},
  {0x2126, 0x2126},
  {0x212b, 0x212b},
  {0x2153, 0x2154},
  {0x215b, 0x215e},
  {0x2160, 0x216b},
  {0x2170, 0x2179},
  {0x2189, 0x2189},
  {0x2190, 0x2199},
  {0x21b8, 0x21b9},
  {0x21d2, 0x21d2},
  {0x21d4, 0x21d4},
  {0x21e7, 0x21e7},
  {0x2200, 0x2200},
  {0x2202, 0x2203},
  {0x2207, 0x2208},
  {0x220b, 0x220b},
  {0x220f, 0x220f},
  {0x2211, 0x2211},
  {0x2215, 0x2215},
  {0x221a, 0x221a},
  {0x221d, 0x2220},
  {0x2223, 0x2223},
  {0x2225, 0x2225},
  {0x2227, 0x222c},
  {0x222e, 0x222e},
  {0x2234, 0x2237},
  {0x223c, 0x223d},
  {0x2248, 0x2248},
  {0x224c, 0x224c},
  {0x2252, 0x2252},
  {0x2260, 0x2261},
  {0x2264, 0x2267},
  {0x226a, 0x226b},
  {0x226e, 0x226f},
  {0x2282, 0x2283},
  {0x2286, 0x2287},
  {0x2295, 0x2295},
  {0x2299, 0x2299},
  {0x22a5, 0x22a5},
  {0x22bf, 0x22bf},
  {0x2312, 0x2312},
  {0x2460, 0x24e9},
  {0x24eb, 0x254b},
  {0x2550, 0x2573},
  {0x2580, 0x258f},
  {0x2592, 0x2595},
  {0x25a0, 0x25a1},
  {0x25a3, 0x25a9},
  {0x25b2, 0x25b3},
  {0x25b6, 0x25b7},
  {0x25bc, 0x25bd},
  {0x25c0, 0x25c1},
  {0x25c6, 0x25c8},
  {0x25cb, 0x25cb},
  {0x25ce, 0x25d1},
  {0x25e2, 0x25e5},
  {0x25ef, 0x25ef},
  {0x2605, 0x2606},
  {0x2609, 0x2609},
  {0x260e, 0x260f},
  {0x261c, 0x261c},
  {0x261e, 0x261e},
  {0x2640, 0x2640},
  {0x2642, 0x2642},
  {0x2660, 0x2661},
  {0x2663, 0x2665},
  {0x2667, 0x266a},
  {0x266c, 0x266d},
  {0x266f, 0x266f},
  {0x269e, 0x269f},
  {0x26bf, 0x26bf},
  {0x26c6, 0x26cd},
  {0x26cf, 0x26d3},
  {0x26d5, 0x26e1},
  {0x26e3, 0x26e3},
  {0x26e8, 0x26e9},
  {0x26eb, 0x26f1},
  {0x26f4, 0x26f4},
  {0x26f6, 0x26f9},
  {0x26fb, 0x26fc},
  {0x26fe, 0x26ff},
  {0x273d, 0x273d},
  {0x2776, 0x277f},
  {0x2b56, 0x2b59},
  {0x3248, 0x324f},
  {0xe000, 0xf8ff},
  {0xfe00, 0xfe0f},
  {0xfffd, 0xfffd},
};

static const struct xwc9_ival ambiguous2[] = {
  {0xf100, 0xf10a}, /* 0x1 prefix */
  {0xf110, 0xf12d},
  {0xf130, 0xf169},
  {0xf170, 0xf18d},
  {0xf18f, 0xf190},
  {0xf19b, 0xf1ac},
};

static const struct xwc9_ival ambiguous3[] = {
  {0x0100, 0x01ef}, /* 0xe prefix */
};

static const struct xwc9_ival ambiguous4[] = {
  {0x0000, 0xfffd}, /* 0xf prefix */
};

static const struct xwc9_ival ambiguous5[] = {
  {0x0000, 0xfffd}, /* 0x10 prefix */
};

/* ambiguous: uniset +WIDTH-A -cat=Me -cat=Mn -cat=Cf c
 * combining: uniset +cat=Me +cat=Mn +cat=Cf -00AD +1160-11FF +200B c
 */

struct xwc9_base {
  int               width; /* width of char, default if none match is 1 */
  char32_t          low;   /* >= low  <= low + 0x10000 */
  size_t            arsz;  /* size of interval ar[] */
  const xwc9_ival * ar;    /* the intervals */
};
#define IVAL(arr) sizeof( arr ) / sizeof( arr[ 0 ] ), arr

static const xwc9_base base[] = {
  { 0, 0,        IVAL( nonprint ) },
  { 2, 0,        IVAL( doublewidth1 ) },
  { 0, 0,        IVAL( combining1 ) },
  { 1, 0,        IVAL( ambiguous1 ) },
  { 2, 0x10000,  IVAL( doublewidth2 ) },
  { 2, 0x10000,  IVAL( emoji ) },
  { 0, 0x10000,  IVAL( combining2 ) },
  { 1, 0x10000,  IVAL( ambiguous2 ) },
  { 2, 0x20000,  IVAL( doublewidth3 ) },
  { 2, 0x30000,  IVAL( doublewidth4 ) },
  { 0, 0xe0000,  IVAL( combining3 ) },
  { 1, 0xe0000,  IVAL( ambiguous3 ) },
  { 2, 0xf0000,  IVAL( ambiguous4 ) },
  { 2, 0x100000, IVAL( ambiguous5 ) },
};

extern "C"
int
xwcwidth9( char32_t c )
{
  /* index base[] by bits after 16  0x1 == 0x10000, 0x2 == 0x20000 .. */
  for ( size_t i = 0; i < sizeof( base ) / sizeof( base[ 0 ] ); i++ ) {
    if ( c < base[ i ].low )    /* other tables out of range */
      return 1;                 /* default 1 */
    if ( c < base[ i ].low + 0x10000 ) { /* c >= low && c < high */
      const uint16_t    v  = (uint16_t) ( c - base[ i ].low );
      const xwc9_ival * ar = base[ i ].ar;
      const size_t      sz = base[ i ].arsz;
      for ( size_t j = 0; ; ) {
        if ( v < ar[ j ].first )
          break;
        if ( v <= ar[ j ].last ) /* first <= c <= last */
          return base[ i ].width;
        if ( ++j == sz )
          break;
      }
    }
  }
  if ( c > 0x10ffff )
    return -1;
  return 1;
}

