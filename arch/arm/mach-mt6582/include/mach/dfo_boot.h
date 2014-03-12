#ifndef DFO_BOOT_H
#define DFO_BOOT_H

#define ATAG_DFO_DATA 0x41000805
#define DFO_BOOT_COUNT 14

typedef struct
{
    char name[DFO_BOOT_COUNT][32];   // kernel dfo name array
    int value[DFO_BOOT_COUNT];       // kernel dfo value array
} tag_dfo_boot;

#endif
