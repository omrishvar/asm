/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.c
 * Author: doron276
 * Created on 21 יולי 2018, 12:41
 */

#include <stdio.h>

#include "global.h"

#include "asm.h"
#include "output.h"

#define MIN_NUMBER_OF_ARGUMENTS 2

int main(int argc, char * argv[]) {
    HASM_FILE hAsm = NULL;
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    
    if (argc < MIN_NUMBER_OF_ARGUMENTS) {
        printf("USAGE: %s <file1> <file2> ...\n", argv[0]);
        return 1;
    }
    for (int nIndex = 1; nIndex < argc; nIndex++) {
        eRetValue = ASM_Compile(argv[nIndex], &hAsm);
        if (GLOB_ERROR_PARSING_FAILED == eRetValue) {
            continue;
        }
        if (eRetValue) {
            return eRetValue;
        }
        eRetValue = OUTPUT_WriteFiles(argv[nIndex], hAsm);
        if (eRetValue) {
            ASM_Close(hAsm);
            return eRetValue;
        }
        ASM_Close(hAsm);
    }
}

