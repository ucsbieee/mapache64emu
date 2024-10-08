#include "vrEmu6502.h"
#include "raylib.h"
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <unistd.h>

uint8_t memory[0x10000];

uint8_t memRead(uint16_t addr, bool isDbg) {
    return memory[addr];
}

void memWrite(uint16_t addr, uint8_t val) {
    memory[addr] = val;
}

int main(int argc, char *argv[]) {
    if(argc < 2) {
        fprintf(stderr, "Missing memory image file argument\n");
        return 1;
    }
    FILE *image = fopen(argv[1], "r");
    if(!image) {
        fprintf(stderr, "Failed to open file \"%s\"\n", argv[1]);
        return 1;
    }

    for(int i = 0; i < 0x10000; i++) {
        int ch = fgetc(image);
        if(ch == EOF) {
            fprintf(stderr, "Image file \"%s\"\n not long enough\n", argv[1]);
            return 1;
        }
        memory[i] = ch;
    }
    fclose(image);

    VrEmu6502 *vr6502 = vrEmu6502New(CPU_65C02, memRead, memWrite);
    if(!vr6502) {
        fprintf(stderr, "Failed to create CPU emulation\n");
        return 1;
    }


    while(1) {
        if(vrEmu6502GetOpcodeCycle(vr6502) == 0) {
            uint16_t pc = vrEmu6502GetCurrentOpcodeAddr(vr6502);
            switch(vrEmu6502GetCurrentOpcode(vr6502)) {
                case 0xdb:  
                    printf("\nFinal instruction:\n");
                    return 0;
                case 0xcb:
                    printf("\nInterrupt hit");
                    FILE *vramdump = fopen("vram.bin", "w");
                    fwrite(memory+0x4000, 1, 0x1000, vramdump);
                    sleep(1);
                    break;
                default:
                    break;
            }
        }
        vrEmu6502InstCycle(vr6502);
    }
    return 0;
}
