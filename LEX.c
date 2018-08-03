/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   LEX.c
 * Author: doron276
 * 
 * Created on 30 יולי 2018, 14:31
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "LEX.h"
#include "LINESTR.h"
#include "global.h"

typedef enum LEX_STATUS {
    LEX_STATUS_NOT_OPENED,
    LEX_STATUS_READY_TO_READ_LINE,
    LEX_STATUS_IN_LINE,
    LEX_STATUS_END_OF_LINE,
} LEX_STATUS;

static const char * g_aszOpcodes[] = {"mov", "cmp", "add", "sub", "not", "clr",
    "lea", "inc", "dec", "jmp", "bne", "red", "prn", "jsr", "rts", "stop"};
static LEX_STATUS g_eStatus = LEX_STATUS_NOT_OPENED;
static PLINESTR_LINE g_ptCurrentLine = NULL;
static int g_nCurrentColumn = 0;
static int g_nCurrentLineLength = 0;

GLOB_ERROR LEX_Open(const char * szFileName){
    GLOB_ERROR eRetValue = GLOB_SUCCESS;
    if (LEX_STATUS_NOT_OPENED != g_eStatus) {
        // todo: error handling
        return GLOB_ERROR_INVALID_STATE;
    }
    eRetValue = LINESTR_Open(szFileName);
    if(eRetValue) {
        return eRetValue;
    }
    g_eStatus = LEX_STATUS_READY_TO_READ_LINE;
    eRetValue = GLOB_SUCCESS;
    return eRetValue;
}

GLOB_ERROR lex_ParseDirective(PLEX_TOKEN ptToken) {
    int nTokenLength = 0;
    g_nCurrentColumn++;
    while ((g_nCurrentColumn < g_nCurrentLineLength)
            && isalpha(g_ptCurrentLine->szLine[g_nCurrentColumn])) {
        g_nCurrentColumn++;
    }
    nTokenLength = g_nCurrentColumn - ptToken->nColumn;
    // todo: check of by 1
    if (0 == strncmp(".data", g_ptCurrentLine->szLine+ptToken->nColumn, nTokenLength)) {
        ptToken->uValue.eDiretive = GLOB_DIRECTIVE_DATA;
    } else if (0 == strncmp(".string", g_ptCurrentLine->szLine+ptToken->nColumn, nTokenLength)) {
        ptToken->uValue.eDiretive = GLOB_DIRECTIVE_STRING;
    } else if (0 == strncmp(".entry", g_ptCurrentLine->szLine+ptToken->nColumn, nTokenLength)) {
        ptToken->uValue.eDiretive = GLOB_DIRECTIVE_ENTRY;
    } else if (0 == strncmp(".extern", g_ptCurrentLine->szLine+ptToken->nColumn, nTokenLength)) {
        ptToken->uValue.eDiretive = GLOB_DIRECTIVE_EXTERN;
    } else {
        // unknown directive
        // todo error handling
        // move to end of line
        g_eStatus = LEX_STATUS_END_OF_LINE;
        return GLOB_ERROR_PARSING_FAILED;
    }
    ptToken->eKind = LEX_TOKEN_KIND_DIRECTIVE;
    return GLOB_SUCCESS;
}


GLOB_ERROR lex_ParseImmediateNumber(PLEX_TOKEN ptToken) {
    BOOL bFoundDigit = FALSE;
    BOOL bIsMinus = FALSE;
    int nValue = 0;
    g_nCurrentColumn++; // skip the '#'
    // the current char may be a digit, a plus or a minus sign
    bIsMinus = ('-' == g_ptCurrentLine->szLine[g_nCurrentColumn]);
    if (bIsMinus || ('+' == g_ptCurrentLine->szLine[g_nCurrentColumn])) {
        g_nCurrentColumn++;
    }
    while ((g_nCurrentColumn < g_nCurrentLineLength)
            && isdigit(g_ptCurrentLine->szLine[g_nCurrentColumn])) {
        bFoundDigit = TRUE;
        nValue = 10*nValue + g_ptCurrentLine->szLine[g_nCurrentColumn] - '0';
        g_nCurrentColumn++;
    }
    if (!bFoundDigit) {
        // must include at least one digit
        return GLOB_ERROR_PARSING_FAILED;
    }
    ptToken->eKind = LEX_TOKEN_KIND_NUMBER;
    ptToken->uValue.nNumber = bIsMinus ? -nValue : nValue;
    return GLOB_SUCCESS;
}


GLOB_ERROR lex_ParseString(PLEX_TOKEN ptToken) {
    g_nCurrentColumn++; // skip the '"'
    // the current char may be a digit, a plus or a minus sign
    while ((g_nCurrentColumn < g_nCurrentLineLength)
            && ('"' != g_ptCurrentLine->szLine[g_nCurrentColumn])) {
        g_nCurrentColumn++;
    }
    if (g_nCurrentColumn == g_nCurrentLineLength) {
        // didn't find closing '"'
        return GLOB_ERROR_PARSING_FAILED;
    }
    ptToken->uValue.szStr = malloc(g_nCurrentColumn - ptToken->nColumn - 1);
    if (NULL == ptToken->uValue.szStr) {
        return GLOB_ERROR_SYS_CALL_ERROR();
    }
    strncpy(ptToken->uValue.szStr, g_ptCurrentLine->szLine + ptToken-> nColumn + 1, g_nCurrentColumn - ptToken->nColumn - 2);
    ptToken->eKind = LEX_TOKEN_KIND_STRING;
    return GLOB_SUCCESS;
}

int lex_FindInStringsArray(const char ** paszStringsArray, int nArrayElements, const char * pszStr, int nStrLength) {
    for (int i = 0; i < nArrayElements; i++) {
        if (0 == strncmp(paszStringsArray[i], pszStr, nStrLength)) {
            return i;
        }
    }
    return -1;
}

GLOB_ERROR lex_ParseAlpha(PLEX_TOKEN ptToken) {
    BOOL bIsLabelDefinition = FALSE;
    int nIdentifierLength = 0;
    int nOpcode = -1;
    g_nCurrentColumn++; // skip the first char
    
    // next chars may be alpha or digits
    while ((g_nCurrentColumn < g_nCurrentLineLength)
            && (isalpha(g_ptCurrentLine->szLine[g_nCurrentColumn])
                ||isdigit(g_ptCurrentLine->szLine[g_nCurrentColumn]))) {
        g_nCurrentColumn++;
    }
    // check for the maximum length
    nIdentifierLength = g_nCurrentColumn - ptToken->nColumn;
    if (nIdentifierLength  > 31) {
        // todo error handling
        return GLOB_ERROR_TOO_LONG_LABEL;
    }
    bIsLabelDefinition =  ((g_nCurrentColumn < g_nCurrentLineLength)
            && ':' == g_ptCurrentLine->szLine[g_nCurrentColumn]);
    
    // check for opcode
    nOpcode = lex_FindInStringsArray(g_aszOpcodes, 16, g_ptCurrentLine->szLine + ptToken->nColumn, nIdentifierLength);
    if (-1 != nOpcode) { 
        if (bIsLabelDefinition) {
            // can't use the register name as a label
            // todo error handling
            return GLOB_ERROR_FORBIDDEN_IDENTIFIER;
        }
        ptToken->eKind = LEX_TOKEN_KIND_OPCODE;
        ptToken->uValue.eOpcode = nOpcode;
        return GLOB_SUCCESS;
    }
    
    // check for register
    if ((2 == nIdentifierLength)
            && ('0' <= g_ptCurrentLine->szLine[ptToken->nColumn+1])
            && ('7' >= g_ptCurrentLine->szLine[ptToken->nColumn+1])) { 
        if (bIsLabelDefinition) {
            // can't use the register name as a label
            // todo error handling
            return GLOB_ERROR_FORBIDDEN_IDENTIFIER;
        }
        ptToken->eKind = LEX_TOKEN_KIND_REGISTER;
        ptToken->uValue.nNumber = g_ptCurrentLine->szLine[ptToken->nColumn+1] - '0';
        return GLOB_SUCCESS;
    }
    
    // check for other forbidden identirifiers
    if ((0 == strncmp(g_ptCurrentLine->szLine + ptToken->nColumn, "data", nIdentifierLength))
        || (0 == strncmp(g_ptCurrentLine->szLine + ptToken->nColumn, "string", nIdentifierLength))
        || (0 == strncmp(g_ptCurrentLine->szLine + ptToken->nColumn, "extern", nIdentifierLength))
        || (0 == strncmp(g_ptCurrentLine->szLine + ptToken->nColumn, "entry", nIdentifierLength)))
    {
        // todo error handling
        return GLOB_ERROR_FORBIDDEN_IDENTIFIER;
    }
    
    ptToken->uValue.szStr = malloc(g_nCurrentColumn - ptToken->nColumn + 1);
    if (NULL == ptToken->uValue.szStr) {
        return GLOB_ERROR_SYS_CALL_ERROR();
    }
    strncpy(ptToken->uValue.szStr, ptToken->nColumn, g_nCurrentColumn - ptToken->nColumn);
    ptToken->eKind = bIsLabelDefinition ? LEX_TOKEN_KIND_LABEL : LEX_TOKEN_KIND_WORD;
    g_nCurrentColumn += bIsLabelDefinition; // skip ":" of label definition
    return GLOB_SUCCESS;
}
            
GLOB_ERROR LEX_ReadNextToken(PLEX_TOKEN * pptToken) {
    PLEX_TOKEN ptToken = NULL;
    BOOL bFirstToken = FALSE;
    BOOL bNoSpaceFromPrevToken = TRUE;
    char cCurrentChar = '\0';
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    // if at end of line, return
    if (LEX_STATUS_END_OF_LINE == g_eStatus) {
        return GLOB_ERROR_END_OF_LINE;
    }
    // if ready to read line, read it
    if (LEX_STATUS_READY_TO_READ_LINE == g_eStatus) {
        eRetValue = LINESTR_GetNextLine(&g_ptCurrentLine);
        if (eRetValue) {
            // including end of file
            return eRetValue;
        }
        g_nCurrentColumn = 0;
        g_nCurrentLineLength = strlen(g_ptCurrentLine->szLine);
        g_eStatus = LEX_STATUS_IN_LINE;
        bFirstToken = TRUE;
        bNoSpaceFromPrevToken = FALSE;
    }
    
    while (g_nCurrentColumn < g_nCurrentLineLength) {
        cCurrentChar = g_ptCurrentLine->szLine[g_nCurrentColumn];
        // if we found a token, break
        if ((' ' != cCurrentChar) && ('\t' != cCurrentChar)) {
            break;
        }
        // white space.
        bNoSpaceFromPrevToken = FALSE;
        g_nCurrentColumn++;
    }
    
    // check if we found a token or it is end of line
    if (g_nCurrentColumn == g_nCurrentLineLength)
    {
        // end of line
        g_eStatus = LEX_STATUS_END_OF_LINE;
        return GLOB_ERROR_END_OF_LINE;
    }
    
    // alloc token
    ptToken = malloc(sizeof(*ptToken));
    if (NULL == ptToken) {
        // todo error handling
        return GLOB_ERROR_SYS_CALL_ERROR();
    }
    
    ptToken->nColumn = g_nCurrentColumn;
    
    if (';' == cCurrentChar) {
        // remark.
        ptToken->eKind = LEX_TOKEN_KIND_REMARK;
        // remark has no value and it extends till end of line
        g_nCurrentColumn = g_nCurrentLineLength;
    } else if (NULL != strchr(",()", cCurrentChar)) {
        // special char
        ptToken->eKind = LEX_TOKEN_KIND_SPECIAL;
        ptToken->uValue.cChar = cCurrentChar;
        g_nCurrentColumn++;
    } else if ('.' == cCurrentChar) {
        eRetValue = lex_ParseDirective(ptToken);
        if (eRetValue) {
            free(ptToken);
            return eRetValue;
        }
    } else if ('#' == cCurrentChar) {
        eRetValue = lex_ParseImmediateNumber(ptToken);
        if (eRetValue) {
            free(ptToken);
            return eRetValue;
        }
    } else if ('"' == cCurrentChar) {
        eRetValue = lex_ParseString(ptToken);
        if (eRetValue) {
            free(ptToken);
            return eRetValue;
        }
    } else if (isalpha(cCurrentChar)) {
        eRetValue = lex_ParseAlpha(ptToken);
        if (eRetValue) {
            free(ptToken);
            return eRetValue;
        }
    } else {
        free(ptToken);
        return GLOB_ERROR_PARSING_FAILED;
    }

    // set shared fields
    ptToken->eFlags = 
            (bFirstToken ? LEX_TOKEN_FLAGS_FIRST_TOKEN_IN_LINE : 0)
            | (bNoSpaceFromPrevToken ? LEX_TOKEN_FLAGS_NO_SPACE_FROM_PREV_TOKEN : 0);
    *pptToken = ptToken;
    eRetValue = GLOB_SUCCESS;
    return eRetValue;
}

GLOB_ERROR LEX_GetCurrentPosition(PLINESTR_LINE * pptLine) {
    if (LEX_STATUS_IN_LINE != g_eStatus) {
        // todo error handling
        return GLOB_ERROR_INVALID_STATE;
    }
    *pptLine = g_ptCurrentLine;
    return GLOB_SUCCESS;
}

GLOB_ERROR LEX_MoveToNextLine(){
    if (LEX_STATUS_END_OF_LINE != g_eStatus) {
        // todo error handling
        return GLOB_ERROR_INVALID_STATE;
    }
    LINESTR_FreeLine(g_ptCurrentLine);
    g_ptCurrentLine = NULL;
    g_eStatus = LEX_STATUS_READY_TO_READ_LINE;
    return GLOB_SUCCESS;
}

void LEX_FreeToken(PLEX_TOKEN ptToken){
    if (NULL != ptToken) {
        if (LEX_TOKEN_KIND_STRING == ptToken->eKind) {
            free(ptToken->uValue.szStr);
        }
        free(ptToken);
    }
}

void LEX_Close() {
    if (LEX_STATUS_NOT_OPENED == g_eStatus) {
        return;
    }
    LINESTR_Close();
    g_eStatus = LEX_STATUS_NOT_OPENED;
    if (NULL != g_ptCurrentLine) {
        LINESTR_FreeLine(g_ptCurrentLine);
        g_ptCurrentLine = NULL;
    }
}
