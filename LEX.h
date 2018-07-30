/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   LEX.h
 * Author: doron276
 *
 * Created on 30 יולי 2018, 14:31
 */

#ifndef LEX_H
#define LEX_H

#include "global.h"

typedef enum LEX_TOKEN_KIND {
    LEX_TOKEN_KIND_LABEL,
    LEX_TOKEN_KIND_WORD,
    LEX_TOKEN_KIND_NUMBER,
    LEX_TOKEN_KIND_OPCODE,
    LEX_TOKEN_KIND_DIRECTIVE,
    LEX_TOKEN_KIND_STRING,
    LEX_TOKEN_KIND_DATA,
    LEX_TOKEN_KIND_SPECIAL, 
} LEX_TOKEN_KIND;

typedef union LEX_TOKEN_VALUE {
    char * szStr;
    int nNumber;
    GLOB_OPCODE eOpcode;
    GLOB_DIRECTIVE eDiretive;
    char cChar;
} LEX_TOKEN_VALUE;

typedef struct LEX_TOKEN {
    LEX_TOKEN_KIND eKIND;
    int nColmn;
    LEX_TOKEN_VALUE uValue;
} LEX_TOKEN, *PLEX_TOKEN;

BOOL LEX_Open(char * szFileName, FILE * ptFilePointer);

#endif /* LEX_H */
