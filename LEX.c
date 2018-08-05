/*****************************************************************************
 * File:    lex.c
 * Author:  Doron Shvartztuch
 * The LEX module is responsible for parsing the source file
 * It performs lexical parsing of the source file to tokens from different
 * types as described below.
 * 
 * Implementation:
 * TODO
 * File I/O operations are performed with standard C library functions.
 * The LINESTR_HANDLE contains the information for reading the next lines from
 * the source file and counting the rows.
 *****************************************************************************/

/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include "linestr.h"
#include "global.h"
#include "LEX.h"

/******************************************************************************
 * TYPEDEFS
 *****************************************************************************/
typedef enum LEX_STATUS {
    LEX_STATUS_READY_TO_READ_LINE,
    LEX_STATUS_IN_LINE,
    LEX_STATUS_END_OF_LINE,
} LEX_STATUS;

/* LEX_FILE is the struct behind the the HLEX_FILE.
 * It keeps the HLINESTR_FILE that read the file as well as other information
 * about the current parsing status  */
struct LEX_FILE {
    /* The status of the parser in this file.
     * See LEX_STATUS for possible values. */
    LEX_STATUS eStatus;
    
    /* Handle to the LINESTR "instance" that read
     * the lines of the source file.*/
    HLINESTR_FILE hSourceFile;
    
    /* The current line that we parse into tokens. */
    PLINESTR_LINE ptCurrentLine;
    
    /* The length of the line we parse */
    int nCurrentLineLength;
    
    /* The zero-based position of the parser in the current line. */
    int nCurrentColumn;     
};

/******************************************************************************
 * CONSTANTS
 *****************************************************************************/

/* This constant array contains the strings of the opcode in the language.
 * Note: The index of an opcode is equal to its GLOB_OPCODE value. */
static const char * g_aszOpcodes[] = {"mov", "cmp", "add", "sub", "not", "clr",
    "lea", "inc", "dec", "jmp", "bne", "red", "prn", "jsr", "rts", "stop"};

/******************************************************************************
 * EXTERNAL FUNCTIONS
 * ------------------
 * See function-level documentation in the header file
 *****************************************************************************/

/******************************************************************************
 * LEX_Open
 *****************************************************************************/
GLOB_ERROR LEX_Open(const char * szFileName, PHLEX_FILE phFile){
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    HLEX_FILE hFile = NULL;
    
    /* Check parameters */
    if (NULL == szFileName || NULL == phFile) {
        return GLOB_ERROR_INVALID_PARAMETERS;
    }
    
    /* Allocate the handle */
    hFile = malloc(sizeof(*hFile));
    if (NULL == hFile) {
        return GLOB_ERROR_SYS_CALL_ERROR();
    }
    
    /* Init fields */
    hFile->eStatus = LEX_STATUS_READY_TO_READ_LINE;
    hFile->ptCurrentLine = NULL;
    hFile->nCurrentColumn = 0;
    hFile->nCurrentLineLength = 0;
    hFile->hSourceFile = NULL;
    
    /* Open the source file */
    eRetValue = LINESTR_Open(szFileName, &hFile->hSourceFile);
    if(eRetValue) {
        free(hFile);
        return eRetValue;
    }
    
    /* Set out parameter upon success */
    *phFile = hFile;
    return GLOB_SUCCESS;
}

GLOB_ERROR lex_ParseDirective(HLEX_FILE hFile, PLEX_TOKEN ptToken) {
    int nTokenLength = 0;
    hFile->nCurrentColumn++;
    while ((hFile->nCurrentColumn < hFile->nCurrentLineLength)
            && isalpha(hFile->ptCurrentLine->szLine[hFile->nCurrentColumn])) {
        hFile->nCurrentColumn++;
    }
    nTokenLength = hFile->nCurrentColumn - ptToken->nColumn;
    // todo: check of by 1
    if (0 == strncmp(".data", hFile->ptCurrentLine->szLine+ptToken->nColumn, nTokenLength)) {
        ptToken->uValue.eDiretive = GLOB_DIRECTIVE_DATA;
    } else if (0 == strncmp(".string", hFile->ptCurrentLine->szLine+ptToken->nColumn, nTokenLength)) {
        ptToken->uValue.eDiretive = GLOB_DIRECTIVE_STRING;
    } else if (0 == strncmp(".entry", hFile->ptCurrentLine->szLine+ptToken->nColumn, nTokenLength)) {
        ptToken->uValue.eDiretive = GLOB_DIRECTIVE_ENTRY;
    } else if (0 == strncmp(".extern", hFile->ptCurrentLine->szLine+ptToken->nColumn, nTokenLength)) {
        ptToken->uValue.eDiretive = GLOB_DIRECTIVE_EXTERN;
    } else {
        // unknown directive
        // todo error handling
        // move to end of line
        hFile->eStatus = LEX_STATUS_END_OF_LINE;
        return GLOB_ERROR_PARSING_FAILED;
    }
    ptToken->eKind = LEX_TOKEN_KIND_DIRECTIVE;
    return GLOB_SUCCESS;
}


GLOB_ERROR lex_ParseImmediateNumber(HLEX_FILE hFile, PLEX_TOKEN ptToken) {
    BOOL bFoundDigit = FALSE;
    BOOL bIsMinus = FALSE;
    int nValue = 0;
    hFile->nCurrentColumn++; // skip the '#'
    // the current char may be a digit, a plus or a minus sign
    bIsMinus = ('-' == hFile->ptCurrentLine->szLine[hFile->nCurrentColumn]);
    if (bIsMinus || ('+' == hFile->ptCurrentLine->szLine[hFile->nCurrentColumn])) {
        hFile->nCurrentColumn++;
    }
    while ((hFile->nCurrentColumn < hFile->nCurrentLineLength)
            && isdigit(hFile->ptCurrentLine->szLine[hFile->nCurrentColumn])) {
        bFoundDigit = TRUE;
        nValue = 10*nValue + hFile->ptCurrentLine->szLine[hFile->nCurrentColumn] - '0';
        hFile->nCurrentColumn++;
    }
    if (!bFoundDigit) {
        // must include at least one digit
        return GLOB_ERROR_PARSING_FAILED;
    }
    ptToken->eKind = LEX_TOKEN_KIND_NUMBER;
    ptToken->uValue.nNumber = bIsMinus ? -nValue : nValue;
    return GLOB_SUCCESS;
}


GLOB_ERROR lex_ParseString(HLEX_FILE hFile, PLEX_TOKEN ptToken) {
    hFile->nCurrentColumn++; // skip the '"'
    // the current char may be a digit, a plus or a minus sign
    while ((hFile->nCurrentColumn < hFile->nCurrentLineLength)
            && ('"' != hFile->ptCurrentLine->szLine[hFile->nCurrentColumn])) {
        hFile->nCurrentColumn++;
    }
    if (hFile->nCurrentColumn == hFile->nCurrentLineLength) {
        // didn't find closing '"'
        return GLOB_ERROR_PARSING_FAILED;
    }
    ptToken->uValue.szStr = malloc(hFile->nCurrentColumn - ptToken->nColumn - 1);
    if (NULL == ptToken->uValue.szStr) {
        return GLOB_ERROR_SYS_CALL_ERROR();
    }
    strncpy(ptToken->uValue.szStr, hFile->ptCurrentLine->szLine + ptToken-> nColumn + 1, hFile->nCurrentColumn - ptToken->nColumn - 2);
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

GLOB_ERROR lex_ParseAlpha(HLEX_FILE hFile, PLEX_TOKEN ptToken) {
    BOOL bIsLabelDefinition = FALSE;
    int nIdentifierLength = 0;
    int nOpcode = -1;
    hFile->nCurrentColumn++; // skip the first char
    
    // next chars may be alpha or digits
    while ((hFile->nCurrentColumn < hFile->nCurrentLineLength)
            && (isalpha(hFile->ptCurrentLine->szLine[hFile->nCurrentColumn])
                ||isdigit(hFile->ptCurrentLine->szLine[hFile->nCurrentColumn]))) {
        hFile->nCurrentColumn++;
    }
    // check for the maximum length
    nIdentifierLength = hFile->nCurrentColumn - ptToken->nColumn;
    if (nIdentifierLength  > 31) {
        // todo error handling
        return GLOB_ERROR_TOO_LONG_LABEL;
    }
    bIsLabelDefinition =  ((hFile->nCurrentColumn < hFile->nCurrentLineLength)
            && ':' == hFile->ptCurrentLine->szLine[hFile->nCurrentColumn]);
    
    // check for opcode
    nOpcode = lex_FindInStringsArray(g_aszOpcodes, 16, hFile->ptCurrentLine->szLine + ptToken->nColumn, nIdentifierLength);
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
            && ('0' <= hFile->ptCurrentLine->szLine[ptToken->nColumn+1])
            && ('7' >= hFile->ptCurrentLine->szLine[ptToken->nColumn+1])) { 
        if (bIsLabelDefinition) {
            // can't use the register name as a label
            // todo error handling
            return GLOB_ERROR_FORBIDDEN_IDENTIFIER;
        }
        ptToken->eKind = LEX_TOKEN_KIND_REGISTER;
        ptToken->uValue.nNumber = hFile->ptCurrentLine->szLine[ptToken->nColumn+1] - '0';
        return GLOB_SUCCESS;
    }
    
    // check for other forbidden identirifiers
    if ((0 == strncmp(hFile->ptCurrentLine->szLine + ptToken->nColumn, "data", nIdentifierLength))
        || (0 == strncmp(hFile->ptCurrentLine->szLine + ptToken->nColumn, "string", nIdentifierLength))
        || (0 == strncmp(hFile->ptCurrentLine->szLine + ptToken->nColumn, "extern", nIdentifierLength))
        || (0 == strncmp(hFile->ptCurrentLine->szLine + ptToken->nColumn, "entry", nIdentifierLength)))
    {
        // todo error handling
        return GLOB_ERROR_FORBIDDEN_IDENTIFIER;
    }
    
    ptToken->uValue.szStr = malloc(hFile->nCurrentColumn - ptToken->nColumn + 1);
    if (NULL == ptToken->uValue.szStr) {
        return GLOB_ERROR_SYS_CALL_ERROR();
    }
    strncpy(ptToken->uValue.szStr,hFile->ptCurrentLine->szLine + ptToken->nColumn, hFile->nCurrentColumn - ptToken->nColumn);
    ptToken->eKind = bIsLabelDefinition ? LEX_TOKEN_KIND_LABEL : LEX_TOKEN_KIND_WORD;
    hFile->nCurrentColumn += bIsLabelDefinition; // skip ":" of label definition
    return GLOB_SUCCESS;
}
            
/******************************************************************************
 * LEX_ReadNextToken
 *****************************************************************************/
GLOB_ERROR LEX_ReadNextToken(HLEX_FILE hFile, PLEX_TOKEN * pptToken) {
    PLEX_TOKEN ptToken = NULL;
    BOOL bFirstToken = FALSE;
    BOOL bNoSpaceFromPrevToken = TRUE;
    char cCurrentChar = '\0';
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    
    /* Check Parameters */
    if (NULL == hFile || NULL == pptToken) {
        return GLOB_ERROR_INVALID_PARAMETERS;
    }
    // if at end of line, return
    if (LEX_STATUS_END_OF_LINE == hFile->eStatus) {
        return GLOB_ERROR_END_OF_LINE;
    }
    // if ready to read line, read it
    if (LEX_STATUS_READY_TO_READ_LINE == hFile->eStatus) {
        eRetValue = LINESTR_GetNextLine(hFile->hSourceFile, &hFile->ptCurrentLine);
        if (eRetValue) {
            // including end of file
            return eRetValue;
        }
        hFile->nCurrentColumn = 0;
        hFile->nCurrentLineLength = strlen(hFile->ptCurrentLine->szLine);
        hFile->eStatus = LEX_STATUS_IN_LINE;
        bFirstToken = TRUE;
        bNoSpaceFromPrevToken = FALSE;
    }
    
    while (hFile->nCurrentColumn < hFile->nCurrentLineLength) {
        cCurrentChar = hFile->ptCurrentLine->szLine[hFile->nCurrentColumn];
        // if we found a token, break
        if ((' ' != cCurrentChar) && ('\t' != cCurrentChar)) {
            break;
        }
        // white space.
        bNoSpaceFromPrevToken = FALSE;
        hFile->nCurrentColumn++;
    }
    
    // check if we found a token or it is end of line
    if (hFile->nCurrentColumn == hFile->nCurrentLineLength)
    {
        // end of line
        hFile->eStatus = LEX_STATUS_END_OF_LINE;
        return GLOB_ERROR_END_OF_LINE;
    }
    
    // alloc token
    ptToken = malloc(sizeof(*ptToken));
    if (NULL == ptToken) {
        // todo error handling
        return GLOB_ERROR_SYS_CALL_ERROR();
    }
    
    ptToken->nColumn = hFile->nCurrentColumn;
    
    if (';' == cCurrentChar) {
        // remark.
        ptToken->eKind = LEX_TOKEN_KIND_REMARK;
        // remark has no value and it extends till end of line
        hFile->nCurrentColumn = hFile->nCurrentLineLength;
    } else if (NULL != strchr(",()", cCurrentChar)) {
        // special char
        ptToken->eKind = LEX_TOKEN_KIND_SPECIAL;
        ptToken->uValue.cChar = cCurrentChar;
        hFile->nCurrentColumn++;
    } else if ('.' == cCurrentChar) {
        eRetValue = lex_ParseDirective(hFile, ptToken);
        if (eRetValue) {
            free(ptToken);
            return eRetValue;
        }
    } else if ('#' == cCurrentChar) {
        eRetValue = lex_ParseImmediateNumber(hFile, ptToken);
        if (eRetValue) {
            free(ptToken);
            return eRetValue;
        }
    } else if ('"' == cCurrentChar) {
        eRetValue = lex_ParseString(hFile, ptToken);
        if (eRetValue) {
            free(ptToken);
            return eRetValue;
        }
    } else if (isalpha(cCurrentChar)) {
        eRetValue = lex_ParseAlpha(hFile, ptToken);
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

GLOB_ERROR LEX_GetCurrentPosition(HLEX_FILE hFile, PLINESTR_LINE * pptLine,
                                  int * pnColumn) {
    if (LEX_STATUS_IN_LINE != hFile->eStatus) {
        // todo error handling
        return GLOB_ERROR_INVALID_STATE;
    }
    *pptLine = hFile->ptCurrentLine;
    *pnColumn = hFile->nCurrentColumn + 1;
    return GLOB_SUCCESS;
}

GLOB_ERROR LEX_MoveToNextLine(HLEX_FILE hFile){
    if (LEX_STATUS_END_OF_LINE != hFile->eStatus) {
        // todo error handling
        return GLOB_ERROR_INVALID_STATE;
    }
    LINESTR_FreeLine(hFile->ptCurrentLine);
    hFile->ptCurrentLine = NULL;
    hFile->eStatus = LEX_STATUS_READY_TO_READ_LINE;
    return GLOB_SUCCESS;
}

void LEX_FreeToken(PLEX_TOKEN ptToken){
    if (NULL == ptToken) {
        return;
    }
    if (LEX_TOKEN_KIND_STRING == ptToken->eKind
            || LEX_TOKEN_KIND_LABEL == ptToken->eKind
            || LEX_TOKEN_KIND_WORD == ptToken->eKind) {
        free(ptToken->uValue.szStr);
    }
    free(ptToken);
}

void LEX_Close(HLEX_FILE hFile) {
    if (NULL == hFile) { 
        return;
    }
    LINESTR_Close(hFile->hSourceFile);
    if (NULL != hFile->ptCurrentLine) {
        LINESTR_FreeLine(hFile->ptCurrentLine);
    }
    free(hFile);
}
