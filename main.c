#include "vrEmu6502.h"
#include "raylib.h"
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <unistd.h>

//locations of controller registers in main memory
#define CONTROLLER_1 0x7002
#define CONTROLLER_2 0x7003

#define CONTROLLER_A_MASK 0x80
#define CONTROLLER_B_MASK 0x40
#define CONTROLLER_UP_MASK 0x08
#define CONTROLLER_DOWN_MASK 0x04
#define CONTROLLER_LEFT_MASK 0x02
#define CONTROLLER_RIGHT_MASK 0x01
#define CONTROLLER_START_MASK 0x10
#define CONTROLLER_SELECT_MASK 0x20

//VRAM locations in main memory
#define PMF 0x4000
#define PMB 0x4200
#define NTBL 0x4400
#define BG_PAL 0x47c0
#define OBM 0x4800
#define TXBL 0x4900

typedef struct{
    uint8_t xpos;
    uint8_t ypos;
    uint8_t pattern_config;
    uint8_t color;
} object_t;

typedef uint8_t pattern_t[16];

int width = 256;
int height = 240;
Image renderImage;

uint8_t memory[0x10000];

uint8_t memRead(uint16_t addr, bool isDbg) {
    return memory[addr];
}

void memWrite(uint16_t addr, uint8_t val) {
    memory[addr] = val;
}

Color colors[8][4];

void drawTile(uint8_t x, uint8_t y, pattern_t pattern, uint8_t tileColor) {
    for(int i = 0; i < 8; i++){
        for(int j = 0; j < 8; j++){
            uint8_t pixelShade = (pattern[i*2 + (j >> 2)] >> (j%4)*2) % 4;
            Color pixelColor = colors[tileColor][pixelShade];
            ImageDrawPixel(&renderImage, x+j, y+i, pixelColor);
        }
    }
}

void renderFromVRAM(void){
    uint8_t bgColor0 = memory[BG_PAL] & 0b00000111;
    uint8_t bgColor1 = (memory[BG_PAL] >> 3) & 0b00000111;

    uint8_t (*nametable)[32] = (uint8_t (*)[32])(memory+NTBL);
    pattern_t *bgPatternTable = (pattern_t *)(memory+PMB);
    for(int i = 0; i < 30; i++){
        for(int j = 0; j < 32; j++){
            uint8_t tileColor = (nametable[i][j] & 0b10000000) ? bgColor1 : bgColor0;
            drawTile(j*8, i*8, bgPatternTable[nametable[i][j] & 0b00011111], tileColor);
        }
    }

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


    InitWindow(width, height, "emu");
    SetTargetFPS(60);

    renderImage = GenImageColor(width, height, BLACK);
    Texture renderTexture = LoadTextureFromImage(renderImage);


    //set up color palette parametrically
    colors[0][3] = (Color){0,0,0,255};
    colors[1][3] = (Color){0,0,255,255};
    colors[2][3] = (Color){0,255,0,255};
    colors[3][3] = (Color){0,255,255,255};
    colors[4][3] = (Color){255,0,0,255};
    colors[5][3] = (Color){255,0,255,255};
    colors[6][3] = (Color){255,255,0,255};
    colors[7][3] = (Color){255,255,255,255};

    for(int i = 0; i < 8; i++) {
        for(int j = 0; j < 3; j++){
            colors[i][j].r = colors[i][3].r / 3 * j;
            colors[i][j].g = colors[i][3].g / 3 * j;
            colors[i][j].b = colors[i][3].b / 3 * j;
            colors[i][j].a = 255;
        }
    }

    while(!WindowShouldClose()) {
        if(vrEmu6502GetOpcodeCycle(vr6502) == 0) {
            uint16_t pc = vrEmu6502GetCurrentOpcodeAddr(vr6502);
            switch(vrEmu6502GetCurrentOpcode(vr6502)) {
                case 0xdb:  
                    printf("CPU stopped\n");
                    return 0;
                case 0xcb: //catch vblank interrupt to update screen
                    
                    BeginDrawing();

                    renderFromVRAM();
                    //getInput();
                    
                    UpdateTexture(renderTexture, renderImage.data);
                    DrawTexture(renderTexture, 0, 0, WHITE);

                    EndDrawing();


                    break;
                default:
                    break;
            }
        }
        vrEmu6502InstCycle(vr6502);
    }
    FILE *vramdump = fopen("vram.bin", "w");
    fwrite(memory+0x4000, 1, 0x1000, vramdump);
    fclose(vramdump);
    return 0;
}
