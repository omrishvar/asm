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

#define SOURCE_FILE_EXTENSION ".as"

static FILE * g_phSourceFile = NULL;
static int g_nLineNumber = 0;

BOOL LINESTR_Open(const char * szFileName) {
    // adding the source file extension
    char * szFullFileName = NULL;
    szFullFileName = malloc(strlen(szFileName) + strlen(SOURCE_FILE_EXTENSION) + 1); // +1 for NULL terminator
    if (NULL == szFullFileName) {
        return FALSE;
    }
    strcpy(szFullFileName, szFileName);
    strcat(szFullFileName, SOURCE_FILE_EXTENSION);
    g_phSourceFile = fopen(szFullFileName,"r");
    if (g_phSourceFile == NULL) {
        // TODO: Error handling
        printf("ERROR, The file is null\n");
        free(szFullFileName);
        return FALSE;
    }
    free(szFullFileName);
    g_nLineNumber = 1;
    return TRUE;
}

BOOL LINESTR_GetNextLine(PLINESTR_LINE * pptLine) {
    PLINESTR_LINE ptLine = NULL;
    if (NULL == g_phSourceFile) {
        // no open file
         // TODO: Error handling
        printf("LINESTR_GetNextLine called, but no file is open\n");
        return FALSE;
    }

    // alloc a new LINESTR_LINE structure
    ptLine = malloc(sizeof(*ptLine));
    if (NULL == ptLine) {
        // TODO: error handling
        printf("malloc failed\n");
        return FALSE;
    }
    ptLine->nLineNumber = g_nLineNumber;
    if (NULL == fgets(ptLine->szLine, sizeof(ptLine->szLine), g_phSourceFile)) {
        // end of file
        printf("EOF");//TODO REMOVE
        free(ptLine);
        return FALSE;
    }

    ptLine->szLine[sizeof(ptLine->szLine)-1] = '\0';
    // fgets doesn't delete the '\n' from the string.
    if (ptLine->szLine[strlen(ptLine->szLine)-1] == '\n') {
        ptLine->szLine[strlen(ptLine->szLine)-1] = '\0';
    }
    g_nLineNumber++;

    *pptLine = ptLine;
    return TRUE;
}

void LINESTR_FreeLine(PLINESTR_LINE ptLine) {
    if (NULL != ptLine) {
        free(ptLine);
    }
}

void LINESTR_Close() {
    if (NULL != g_phSourceFile) {
        fclose(g_phSourceFile);
        g_phSourceFile = NULL;
    }
}
