/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   global.h
 * Author: doron276
 *
 * Created on 23 יולי 2018, 21:55
 */

#ifndef GLOBAL_H
#define GLOBAL_H
#define FALSE 0
#define TRUE 1

typedef int BOOL;

typedef enum GLOB_OPCODE {
    GLOB_OPCODE_MOV = 0,
    GLOB_OPCODE_CMP = 1,
    GLOB_OPCODE_ADD = 2,
    GLOB_OPCODE_SUB = 3,
    GLOB_OPCODE_NOT = 4,
    GLOB_OPCODE_CLR = 5,
    GLOB_OPCODE_LEA = 6,
    GLOB_OPCODE_INC = 7,
    GLOB_OPCODE_DEC = 8,
    GLOB_OPCODE_JMP = 9,
    GLOB_OPCODE_BNE = 10,
    GLOB_OPCODE_RED = 11,
    GLOB_OPCODE_PRN = 12,
    GLOB_OPCODE_JSR = 13,
    GLOB_OPCODE_RTS = 14,
    GLOB_OPCODE_STOP = 15,
} GLOB_OPCODE;

typedef enum GLOB_DIRECTIVE {
    GLOB_DIRECTIVE_DATA,
    GLOB_DIRECTIVE_STRING,
    GLOB_DIRECTIVE_ENTRY,
    GLOB_DIRECTIVE_EXTERN,
} GLOB_DIRECTIVE;

#endif /* GLOBAL_H */

