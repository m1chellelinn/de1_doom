#include "i_peripherals.h"
#include "doomdef.h"
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>


inline int convert_to_physical(int virtual_addr) {
    return virtual_addr - ((int)ddr_v_addr) + ddr_p_addr;
}

int open_physical (int fd) {
    if (fd == -1) // check if already open
    if ((fd = open( "/dev/mem", (O_RDWR | O_SYNC))) == -1) {
        printf ("ERROR: could not open \"/dev/mem\"...\n");
        return (-1);
    }
    return fd;
}


void close_physical (int fd) {
    close (fd);
}


void* map_physical(int fd, unsigned int base, unsigned int span) {
    void *virtual_base;
    // Get a mapping from physical addresses to virtual addresses
    virtual_base = mmap (NULL, span, (PROT_READ | PROT_WRITE), MAP_SHARED,
    fd, base);
    if (virtual_base == MAP_FAILED) {
        printf ("ERROR: mmap() failed...\n");
        close (fd);
        return (NULL);
    }
    return virtual_base;
}


int unmap_physical(void * virtual_base, unsigned int span) {
    if (munmap (virtual_base, span) != 0) {
        printf ("ERROR: munmap() failed...\n");
        return (-1);
    }
    return 0;
}


void HAL_I_FinishUpdate(byte *screens) {
    *(doom_ptr+1) = convert_to_physical(screens);
    *doom_ptr = CMD_I_FinishUpdate;
}


void HAL_I_SetPalette(byte* palette) {
    *(doom_ptr+1) = convert_to_physical(palette);
    *doom_ptr = CMD_I_SetPalette;
}


void HAL_V_DrawPatch(int x, int y, int scrn, void *screens, void *patch) {
    *(doom_ptr+1) = x;
    *(doom_ptr+2) = y;
    *(doom_ptr+3) = scrn;
    *(doom_ptr+4) = convert_to_physical(screens);
    *(doom_ptr+5) = convert_to_physical(patch);
    *doom_ptr = CMD_V_DrawPatch;
}


// TODO: We're currently checking if the FPGA can write to memory. 
// Ideally we should also check if it can read from memory.
void HAL_SelfCheck() {
    *(doom_ptr+1) = ddr_p_addr;
    *doom_ptr = CMD_V_Init; // Expect FPGA to line the address range ddr_p_addr to ddr_p_addr+20 with numbers 0, 1, 2, ..., 19.

    int i;
    for (i = 0; i < 20; i++) {
        if (*(ddr_v_addr + i) != i) {
            printf("Address 0x%x (physical addr 0x%x) "
                   "has value %d. Expected %d. \nExit.\n", 
                   ddr_v_addr+i, ddr_p_addr+i, *(ddr_v_addr + i), i);
            exit(1);
        }
    }
}


void HAL_Reset() {
    *doom_ptr = CMD_RESET;
}
