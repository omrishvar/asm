/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   LINESTR.c
 * Author: doron276
 * 
 * Created on 30 יולי 2018, 10:48
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "LINESTR.h"
#include "global.h"

#define MAX_LINE_LENGTH 81 // including NULL 



typedef struct LINESTR_LINE {
    char szLine[MAX_LINE_LENGTH];
    int nLineNumber; // rows number of the file
} LINESTR_LINE, *PLINESTR_LINE;


BOOL LINESTR_Open(const char * szFilenName, FILE * ptFilePointer) {
    
    ptFilePointer = fopen(szFilenName,"r");
    if (ptFilePointer == NULL) {
        printf("ERROR, The file is null\n");
        return FALSE;
    }
    return TRUE;
}

BOOL LINESTR_GetNextLine(PLINESTR_LINE * pptLine) {
    
    
    
        
        
    
}

void LINESTR_FreeLine(PLINESTR_LINE pLine) {
    return;
}

void LINESTR_Close() {
    
}