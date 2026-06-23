/* IO: VGA + SERIAL + BOOT LOG */
#include "io.h"
#include "ports.h"
#include "attributes.h"

/* ==================== SERIAL ==================== */
void serial_init(void) {
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x80);
    outb(0x3F8 + 0, 0x01);
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x03);
    outb(0x3F8 + 2, 0xC7);
    outb(0x3F8 + 4, 0x0B);
    io_wait();
}

void serial_putchar(char c) {
    while (!(inb(0x3F8 + 5) & 0x20)) halt();
    outb(0x3F8, (uint8_t)(unsigned char)c);
}

void serial_writestr(const char *data) {
    while (*data) {
        if (*data == '\n') serial_putchar('\r');
        serial_putchar(*data++);
    }
}

void serial_write(const char *data, size_t n) {
    for (size_t i = 0; i < n; i++) serial_putchar(data[i]);
}

/* ==================== VGA ==================== */
#define VGA_W 80
#define VGA_H 25
#define VGA_MEM ((volatile uint16_t *)0xB8000)

static int vga_row, vga_col;
static uint8_t vga_color;

static uint8_t vga_mkclr(uint8_t fg, uint8_t bg) { return fg | (bg << 4); }
static uint16_t vga_mke(unsigned char c, uint8_t clr) { return (uint16_t)c | ((uint16_t)clr << 8); }

static void vga_hide_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

void vga_init(void) {
    vga_color = vga_mkclr(CLR_LGREY, CLR_BLACK);
    vga_hide_cursor();
    vga_clear();
}

void vga_setcolor(uint8_t fg, uint8_t bg) {
    vga_color = vga_mkclr(fg, bg);
}

void vga_clear(void) {
    uint16_t blank = vga_mke(' ', vga_color);
    for (int i = 0; i < VGA_W * VGA_H; i++) VGA_MEM[i] = blank;
    vga_row = vga_col = 0;
}

static void vga_scroll(void) {
    for (int y = 1; y < VGA_H; y++)
        for (int x = 0; x < VGA_W; x++)
            VGA_MEM[(y-1)*VGA_W + x] = VGA_MEM[y*VGA_W + x];
    uint16_t blank = vga_mke(' ', vga_color);
    for (int x = 0; x < VGA_W; x++) VGA_MEM[(VGA_H-1)*VGA_W + x] = blank;
    vga_row = VGA_H - 1;
}

void vga_putchar(char c) {
    if (c == '\n') { vga_col = 0; if (++vga_row >= VGA_H) vga_scroll(); return; }
    if (c == '\r') { vga_col = 0; return; }
    if (c == '\t') { vga_putchar(' '); vga_putchar(' '); vga_putchar(' '); vga_putchar(' '); return; }
    VGA_MEM[vga_row*VGA_W + vga_col] = vga_mke((unsigned char)c, vga_color);
    if (++vga_col >= VGA_W) { vga_col = 0; if (++vga_row >= VGA_H) vga_scroll(); }
}

void vga_writestr(const char *data) {
    while (*data) vga_putchar(*data++);
}

void vga_write(const char *data, size_t n) {
    for (size_t i = 0; i < n; i++) vga_putchar(data[i]);
}

void vga_setfg(uint8_t fg) {
    vga_color = fg | (vga_color & 0xF0);
}

void vga_setbg(uint8_t bg) {
    vga_color = (vga_color & 0x0F) | (bg << 4);
}

/* ==================== BOOT LOG ==================== */
/* Tag symbols with fixed 7-char width */
#define TAG_OK   "  [OK] "
#define TAG_INFO " [..] "
#define TAG_WARN " [!] "
#define TAG_FAIL " [FAIL]"
#define TAG_STEP " [>>] "

static void log_tag_vga(log_level_t level) {
    uint8_t tag_color;
    switch (level) {
    case LOG_OK:   tag_color = CLR_LBLUE;  break;
    case LOG_INFO: tag_color = CLR_DGREY;  break;
    case LOG_WARN: tag_color = CLR_BROWN;  break;
    case LOG_FAIL: tag_color = CLR_RED;    break;
    case LOG_STEP: tag_color = CLR_MAGENTA; break;
    default:       tag_color = CLR_LGREY;  break;
    }

    vga_setcolor(tag_color, CLR_BLACK);
    vga_writestr("  ");
    vga_setcolor(CLR_WHITE, CLR_BLACK);
    vga_putchar('[');
    vga_setcolor(tag_color, CLR_BLACK);

    /* Tag content */
    switch (level) {
    case LOG_OK:   vga_writestr("OK"); break;
    case LOG_INFO: vga_writestr(".."); break;
    case LOG_WARN: vga_writestr("!");  break;
    case LOG_STEP: vga_writestr(">");  break;
    default:       vga_writestr("?") ; break;
    }

    vga_putchar(']');
    vga_setcolor(CLR_LGREY, CLR_BLACK);
}

static const char *log_tag_serial(log_level_t level) {
    switch (level) {
    case LOG_OK:   return "  [OK]  ";
    case LOG_INFO: return "  [..]  ";
    case LOG_WARN: return "  [!]   ";
    case LOG_FAIL: return " [FAIL] ";
    case LOG_STEP: return "  [>>]  ";
    default:       return "  [?]   ";
    }
}

void log_ok(const char *msg) {
    log_tag_vga(LOG_OK); vga_putchar(' '); vga_writestr(msg); vga_putchar('\n');
    serial_writestr(log_tag_serial(LOG_OK)); serial_writestr(msg); serial_putchar('\n');
}

void log_info(const char *msg) {
    log_tag_vga(LOG_INFO); vga_putchar(' '); vga_writestr(msg); vga_putchar('\n');
    serial_writestr(log_tag_serial(LOG_INFO)); serial_writestr(msg); serial_putchar('\n');
}

void log_warn(const char *msg) {
    log_tag_vga(LOG_WARN); vga_putchar(' '); vga_writestr(msg); vga_putchar('\n');
    serial_writestr(log_tag_serial(LOG_WARN)); serial_writestr(msg); serial_putchar('\n');
}

void log_fail(const char *msg) {
    log_tag_vga(LOG_FAIL); vga_putchar(' '); vga_writestr(msg); vga_putchar('\n');
    serial_writestr(log_tag_serial(LOG_FAIL)); serial_writestr(msg); serial_putchar('\n');
}

void log_step(const char *msg) {
    log_tag_vga(LOG_STEP); vga_putchar(' '); vga_writestr(msg); vga_putchar('\n');
    serial_writestr(log_tag_serial(LOG_STEP)); serial_writestr(msg); serial_putchar('\n');
}

/* ==================== TWO-PHASE LOG ==================== */

static size_t log_strlen(const char *s) {
    const char *p = s;
    while (*p) p++;
    return (size_t)(p - s);
}

log_entry_t log_begin(const char *desc) {
    log_entry_t e;

    /* VGA: print pending tag + description, pad with dots */
    log_tag_vga(LOG_INFO);
    vga_putchar(' ');
    vga_writestr(desc);
    size_t len = log_strlen(desc);
    for (size_t i = len; i < 56; i++) vga_putchar('.');
    e.row = vga_row;
    e.col = vga_col;

    /* Serial: print description, result will follow */
    serial_writestr(log_tag_serial(LOG_INFO));
    serial_writestr(" ");
    serial_writestr(desc);
    serial_writestr("...");

    return e;
}

void log_end(log_entry_t *e, bool success) {
    (void)e; /* Position saved for future enhancements */

    /* VGA: overwrite last 7 chars with result tag */
    vga_col -= 7;
    if (vga_col < 0) { vga_col = 0; }

    vga_setcolor(success ? CLR_LBLUE : CLR_LRED, CLR_BLACK);
    vga_putchar(' ');
    vga_putchar('[');
    if (success) { vga_writestr("OK"); }
    else         { vga_writestr("FAIL"); }
    vga_putchar(']');
    vga_setcolor(CLR_LGREY, CLR_BLACK);
    vga_putchar('\n');

    /* Serial: append result */
    serial_writestr(success ? " [OK]\n" : " [FAIL]\n");
}

/* ==================== BANNER ==================== */
#define BOX_H  0xCD
#define BOX_V  0xBA
#define BOX_TL 0xC9
#define BOX_TR 0xBB
#define BOX_BL 0xC8
#define BOX_BR 0xBC
#define BW 32

static void box_line(const char *text, int w) {
    vga_putchar((char)BOX_V);
    vga_putchar(' ');
    vga_writestr(text);
    size_t len = log_strlen(text);
    for (size_t i = len; i < (size_t)(w - 1); i++) vga_putchar(' ');
    vga_putchar((char)BOX_V);
    vga_putchar('\n');
}

void log_init(void) {
    vga_init();
    serial_init();

    int bw = BW;

    /* Top bar */
    vga_setcolor(CLR_LBLUE, CLR_BLACK);
    vga_putchar((char)BOX_TL);
    for (int i = 0; i < bw; i++) vga_putchar((char)BOX_H);
    vga_putchar((char)BOX_TR);
    vga_putchar('\n');

    /* Blank line */
    vga_putchar((char)BOX_V);
    for (int i = 0; i < bw; i++) vga_putchar(' ');
    vga_putchar((char)BOX_V);
    vga_putchar('\n');

    /* NOVA logo (each line exactly 31 chars) */
    vga_setcolor(CLR_LBLUE, CLR_BLACK);
    box_line("  #   #  ###   #   #    #   ", bw);
    box_line("  ##  # #   #  #   #  # #   ", bw);
    box_line("  # # # #   #   # #   #   # ", bw);
    box_line("  #  ## #   #    #    #####  ", bw);
    box_line("  #   #  ###     #    #   #  ", bw);

    /* Blank */
    vga_setcolor(CLR_WHITE, CLR_BLACK);
    vga_putchar((char)BOX_V);
    for (int i = 0; i < bw; i++) vga_putchar(' ');
    vga_putchar((char)BOX_V);
    vga_putchar('\n');

    /* Title */
    vga_setcolor(CLR_LBLUE, CLR_BLACK);
    box_line("  Nova v0.2 - x86 Edition   ", bw);
    vga_setcolor(CLR_WHITE, CLR_BLACK);

    /* Bottom */
    vga_putchar((char)BOX_BL);
    for (int i = 0; i < bw; i++) vga_putchar((char)BOX_H);
    vga_putchar((char)BOX_BR);
    vga_putchar('\n');
    vga_putchar('\n');

    /* Serial */
    serial_writestr("NOVA v0.2 - x86 Edition\n");
    serial_writestr("=======================\n\n");
}