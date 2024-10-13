#include <vrEmu6502.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define BUFSZ 50

typedef enum {
    HELP,
    STEP,
    FRAME,
    RUN,
    DUMP,
    QUIT
} command;

const char *commands[] = {
    "help",
    "step",
    "frame",
    "run",
    "dump",
    "quit"
};
#define COMNO 6

extern uint8_t memory[0x10000];

void updateScreen(void);

command lazyMatch(char *input) {
    bool dropCommand[COMNO] = {false};
    int possibleCommands = COMNO;
    for(int i = 0; input[i] && input[i] != '\n'; i++){
        command onlyCommand = HELP;
        for(int j = 0; j < COMNO; j++){
            //command has already been ruled out
            if(dropCommand[j]) {
                continue;
            }
            //command is still possible
            if(commands[j][i] && commands[j][i] == input[i]) {
                continue;
            }
            //command has been ruled out with new character
            dropCommand[j] = true;
            possibleCommands--;
        }

        //one possible command remains
        if(possibleCommands == 1) {
            for(int i = 0; i < COMNO; i++) {
                if(!dropCommand[i]) {
                    return (command)i;
                }
            }
        }
        //no command matches
        if(possibleCommands == 0) {
            return HELP;
        }
    }
    
    //search has not narrowed after reading entire string
    return HELP;
}

void stepcpu(VrEmu6502 *cpu) {
    do{
        vrEmu6502InstCycle(cpu);
    } while(vrEmu6502GetOpcodeCycle(cpu) != 0);
}

void monitor(VrEmu6502 *cpu) {
    printf("CPU stopped\n");
    static const char* const labelMap[0x10000] = {0};
    static char instructionbuffer[BUFSZ];
    static char inputbuffer[BUFSZ];
    while(true){
        uint16_t address = vrEmu6502GetPC(cpu);
        uint16_t throwaway;
        vrEmu6502DisassembleInstruction(cpu, address, BUFSZ, instructionbuffer, &throwaway, labelMap);
        uint8_t a, x, y, flags, sp;
        a = vrEmu6502GetAcc(cpu);
        x = vrEmu6502GetX(cpu);
        y = vrEmu6502GetY(cpu);
        flags = vrEmu6502GetStatus(cpu);
        sp = vrEmu6502GetStackPointer(cpu);

        //print instruction and prompt user
        printf("%.4x %s\n"
               "A=%.2x X=%.2x Y=%.2x FLAGS=%.2x SP= %.2x\n\n"
               ">", address, instructionbuffer, a, x, y, flags, sp);
        fgets(inputbuffer, BUFSZ, stdin);

        switch(lazyMatch(inputbuffer)){
            case RUN:
                return;

            case STEP:
                stepcpu(cpu);
                updateScreen();
                break;

            case FRAME:
                do {
                    stepcpu(cpu);
                } while(vrEmu6502GetCurrentOpcode(cpu) != 0xcb);
                updateScreen();
                break;

            case DUMP:
                ;FILE* dumpfile = fopen("dump.bin", "wb");
                if(!dumpfile){
                    fprintf(stderr, "Failed to open file dump.bin\n");
                    break;
                }
                fwrite(memory, 1, 0x10000, dumpfile);
                fclose(dumpfile);
                break;
            
            case QUIT:
                exit(0);
                return;

            default:
                printf("Possible commands are:\n"
                       "help---------print this message\n"
                       "step---------step cpu one instruction\n"
                       "frame--------advance system one frame\n"
                       "run----------resume execution\n"
                       "dump---------write memory contents to dump.bin"
                       "quit---------exit emulation\n\n");
                break;
        }
    }
}
