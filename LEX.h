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
#include "linestr.h"

typedef enum LEX_TOKEN_KIND {
    LEX_TOKEN_KIND_LABEL, //
    LEX_TOKEN_KIND_WORD, //
    LEX_TOKEN_KIND_NUMBER,  //
    LEX_TOKEN_KIND_OPCODE,  //
    LEX_TOKEN_KIND_REGISTER, //
    LEX_TOKEN_KIND_DIRECTIVE, //
    LEX_TOKEN_KIND_STRING,  //
    LEX_TOKEN_KIND_SPECIAL,  //
    LEX_TOKEN_KIND_REMARK,  //
} LEX_TOKEN_KIND;

typedef union LEX_TOKEN_VALUE {
    char * szStr;
    int nNumber;
    GLOB_OPCODE eOpcode;
    GLOB_DIRECTIVE eDiretive;
    char cChar;
} LEX_TOKEN_VALUE;

typedef enum LEX_TOKEN_FLAGS {
    LEX_TOKEN_FLAGS_FIRST_TOKEN_IN_LINE = 1,
    LEX_TOKEN_FLAGS_NO_SPACE_FROM_PREV_TOKEN = 2,
} LEX_TOKEN_FLAGS;

typedef struct LEX_TOKEN {
    LEX_TOKEN_KIND eKind;
    int nColumn;
    LEX_TOKEN_FLAGS eFlags;
    LEX_TOKEN_VALUE uValue;
} LEX_TOKEN, *PLEX_TOKEN;

GLOB_ERROR LEX_Open(const char * szFileName);

GLOB_ERROR LEX_ReadNextToken(PLEX_TOKEN * pptToken);

GLOB_ERROR LEX_GetCurrentPosition(PLINESTR_LINE * pptLine);

GLOB_ERROR LEX_MoveToNextLine();

void LEX_FreeToken(PLEX_TOKEN ptToken);

void LEX_Close();

#endif /* LEX_H */
