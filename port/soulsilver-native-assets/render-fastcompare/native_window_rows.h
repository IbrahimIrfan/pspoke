#ifndef SS_NATIVE_WINDOW_ROWS_H
#define SS_NATIVE_WINDOW_ROWS_H
#include <stdint.h>
typedef struct SSNativeWindowRow {
    uint16_t reg[6]; /* WIN0H, WIN1H, WIN0V, WIN1V, WININ, WINOUT */
    uint16_t enable; /* DISPCNT window-enable bits */
} SSNativeWindowRow;
#endif
