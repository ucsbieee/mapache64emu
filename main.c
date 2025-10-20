#include <vrEmu6502.h>
#include <raylib.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>

//locations of controller registers in main memory
#define CONTROLLER_1 0x7002
#define CONTROLLER_2 0x7003

//bit masks for controller buttons
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
float scaleFactor = 3;
Image renderImage;
Texture renderTexture;

uint8_t memory[0x10000];
Color colors[8][4];
pattern_t font[128];

void monitor(VrEmu6502 *cpu);

uint8_t memRead(uint16_t addr, bool isDbg) {
    (void)isDbg;
    return memory[addr];
}

void memWrite(uint16_t addr, uint8_t val) {
    memory[addr] = val;
}

//draw an 8x8 tile to renderImage, usable for background, foreground, and text
void drawTile(uint8_t x, uint8_t y, pattern_t pattern, uint8_t tileColor, bool hflip, bool vflip) {
    for(int i = 0; i < 8; i++){
        for(int j = 0; j < 8; j++){
            //get shade from pattern (bit wizardry)
            uint8_t pixelShade = (pattern[i*2 + (j >> 2)] >> (3-j%4)*2) % 4;
            if(pixelShade == 0) continue; //handle transparency
            Color pixelColor = colors[tileColor][pixelShade];
            //decide where to draw pixel based on flip flags
            ImageDrawPixel(&renderImage, hflip ? x+(7-j) : x+j, vflip ? y+(7-i) : y+i, pixelColor);
        }
    }
}

//render screen to renderImage based on VRAM content
void renderFromVRAM(void){
    uint8_t bgColor0 = memory[BG_PAL] & 0b00000111;
    uint8_t bgColor1 = (memory[BG_PAL] >> 3) & 0b00000111;

    //render background from nametable and background patterns
    uint8_t (*nametable)[32] = (uint8_t (*)[32])(memory+NTBL);
    pattern_t *bgPatternTable = (pattern_t *)(memory+PMB);
    for(int i = 0; i < 30; i++){
        for(int j = 0; j < 32; j++){
            uint8_t tileColor = (nametable[i][j] & 0b10000000) ? bgColor1 : bgColor0;
            bool hflip = (nametable[i][j] & 0b01000000) != 0;
            bool vflip = (nametable[i][j] & 0b00100000) != 0;
            drawTile(j*8, i*8, bgPatternTable[nametable[i][j] & 0b00011111], tileColor, hflip, vflip);
        }
    }



    //render objects from object memory and foreground patterns
    object_t *objectTable = (object_t *)(memory+OBM);
    pattern_t *fgPatternTable = (pattern_t *)(memory+PMF);

    for(int i = 63; i >= 0; i--){
        object_t obj = objectTable[i];
        bool hflip = (obj.pattern_config & 0b01000000) != 0;
        bool vflip = (obj.pattern_config & 0b00100000) != 0;
        drawTile(obj.xpos, obj.ypos, fgPatternTable[obj.pattern_config & 0b00011111], obj.color & 0b00000111, hflip, vflip);
    }

    //render text from text table and font file
    uint8_t (*texttable)[32] = (uint8_t (*)[32])(memory + TXBL);
    for(int i = 0; i < 30; i++){
        for(int j = 0; j < 32; j++){
            uint8_t tileColor = (texttable[i][j] & 0b10000000) != 0 ? 0b00000111 : 0b00000000; //choose color based on high bit of character
            drawTile(j*8, i*8, font[texttable[i][j] & 0b01111111], tileColor, false, false);
        }
    }

}

//update controller state
//TODO: player 2
void getInput(void) {
    uint8_t input = 0;
    input += CONTROLLER_A_MASK * IsKeyDown(KEY_Z);
    input += CONTROLLER_B_MASK * IsKeyDown(KEY_X);
    input += CONTROLLER_START_MASK * IsKeyDown(KEY_ENTER);
    input += CONTROLLER_SELECT_MASK * IsKeyDown(KEY_TAB);
    input += CONTROLLER_UP_MASK * IsKeyDown(KEY_UP);
    input += CONTROLLER_DOWN_MASK * IsKeyDown(KEY_DOWN);
    input += CONTROLLER_LEFT_MASK * IsKeyDown(KEY_LEFT);
    input += CONTROLLER_RIGHT_MASK * IsKeyDown(KEY_RIGHT);
    memory[CONTROLLER_1] = input;
}

void updateScreen(void) {
    BeginDrawing();
    //black out screen
    ImageClearBackground(&renderImage, BLACK);

    renderFromVRAM();
    getInput();
                
    UpdateTexture(renderTexture, renderImage.data);
    DrawTextureEx(renderTexture, (Vector2){0,0}, 0.0, scaleFactor, WHITE);

    EndDrawing();
}

//load font from file as patterns
int getFont(const char *filename) {
    Image fontImage = LoadImage(filename);
    if(!IsImageValid(fontImage)){
        fprintf(stderr, "Failed to read font.png");
        return -1;
    }
    
    for(int tile = 0; tile < 128; tile++){
        int startX = tile*8 % fontImage.width;
        int startY = (tile*8 / fontImage.width) * 8;

        for(int i = 0; i < 8; i++) {
            for(int j = 0; j < 8; j++) {
                Color fontColor = GetImageColor(fontImage, startX+j, startY+i);
                uint8_t pixelColor = (fontColor.r == 255 && fontColor.g == 255 && fontColor.b == 255) ? 0b00000011 : 0b00000000;
                font[tile][i*2 + (j >> 2)] |= pixelColor << ((3-j%4)*2);
            }
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    //read memory image file
    if(argc < 2) {
        fprintf(stderr, "Missing memory image file argument\n");
        return 1;
    }
    FILE *image = fopen(argv[1], "rb");
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

    //read font file
    if(getFont("font.png")) {
        fprintf(stderr, "Failed to read font.png\n");
        return 1;
    }

    //initialize cpu
    VrEmu6502 *vr6502 = vrEmu6502New(CPU_65C02, memRead, memWrite);
    if(!vr6502) {
        fprintf(stderr, "Failed to create CPU emulation\n");
        return 1;
    }

    InitWindow(width*scaleFactor, height*scaleFactor, "emu");
    SetTargetFPS(60);
    //prevent esc from exiting
    SetExitKey(KEY_NULL);

    //initialize rendering
    renderImage = GenImageColor(width, height, BLACK);
    renderTexture = LoadTextureFromImage(renderImage);


    //set up color palette
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

    //game loop
    while(!WindowShouldClose()) {
        if(vrEmu6502GetOpcodeCycle(vr6502) != 0) {
            vrEmu6502InstCycle(vr6502);
            continue;
        }

        switch(vrEmu6502GetCurrentOpcode(vr6502)) {
            case 0xdb: //catch stop instruction to switch to asm monitor
                monitor(vr6502);
                break;

            case 0xcb: //catch vblank interrupt to update screen
                updateScreen();

                //catch escape key to switch to asm monitor
                if(IsKeyDown(KEY_ESCAPE)) {
                    monitor(vr6502);
                }

                break;

            default:
                break;
        }
        vrEmu6502InstCycle(vr6502);
    }
    return 0;
}
