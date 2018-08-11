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
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "global.h"
#include "parser.h"
#include "symtable.h"
#include "compiler.h"

#include "LINESTR.h"
#include "LEX.h"
#include "asm.h"
#include "output.h"
int main() {
    HASM_FILE hAsm = NULL;
    GLOB_ERROR eRetValue;
    HMEMSTREAM hStream;
    int nCode;
    int nData;
    eRetValue = ASM_Compile("sample", &hAsm);
    if (eRetValue) {
        return eRetValue;
    }
    OUTPUT_WriteFiles("sample", hAsm);
    ASM_Close(hAsm);
}

