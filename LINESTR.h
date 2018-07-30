/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   LINESTR.h
 * Author: doron276
 *
 * Created on 30 יולי 2018, 10:48
 */

#ifndef LINESTR_H
#define LINESTR_H

#include "global.h"

#define LINESTR_MAX_LINE_LENGTH 81 // including NULL 


typedef struct LINESTR_LINE {
    char szLine[LINESTR_MAX_LINE_LENGTH];
    int nLineNumber; // rows number of the file
} LINESTR_LINE, *PLINESTR_LINE;

BOOL LINESTR_Open(const char * szFilenName);

BOOL LINESTR_GetNextLine(PLINESTR_LINE * pptLine);

void LINESTR_FreeLine(PLINESTR_LINE ptLine);

void LINESTR_Close();
#endif /* LINESTR_H */
