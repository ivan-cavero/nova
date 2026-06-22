#ifndef IO_H
#define IO_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* VGA color indices (VGA text mode palette) */
#define CLR_BLACK       0
#define CLR_BLUE        1
#define CLR_GREEN       2
#define CLR_CYAN        3
#define CLR_RED         4
#define CLR_MAGENTA     5
#define CLR_BROWN       6
#define CLR_LGREY       7
#define CLR_DGREY       8
#define CLR_LBLUE       9
#define CLR_LGREEN      10
#define CLR_LCYAN       11
#define CLR_LRED        12
#define CLR_LMAGENTA    13
#define CLR_LBROWN      14
#define CLR_WHITE       15

/* VGA functions */
void vga_init(void);
void vga_putchar(char c);
void vga_writestr(const char *data);
void vga_write(const char *data, size_t n);
void vga_setcolor(uint8_t fg, uint8_t bg);
void vga_setfg(uint8_t fg);
void vga_setbg(uint8_t bg);
void vga_clear(void);

/* Serial functions */
void serial_init(void);
void serial_putchar(char c);
void serial_writestr(const char *data);
void serial_write(const char *data, size_t n);

/* Boot log types */
typedef enum {
    LOG_OK   = 0,
    LOG_INFO = 1,
    LOG_WARN = 2,
    LOG_FAIL = 3,
    LOG_STEP = 4,
} log_level_t;

typedef struct {
    int row;
    int col;
} log_entry_t;

/* Direct log (prints tag + msg + newline immediately) */
void log_ok(const char *msg);
void log_info(const char *msg);
void log_warn(const char *msg);
void log_fail(const char *msg);
void log_step(const char *msg);

/* Two-phase log: prints description, then result on same line */
log_entry_t log_begin(const char *desc);
void log_end(log_entry_t *e, bool success);

/* Init log system (VGA + serial + banner) */
void log_init(void);

#endif /* IO_H */