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

/* LEX_FILE is the struct behind the the HLEX_FILE.
 * It keeps the HLINESTR_FILE that read the file as well as other information
 * about the current parsing status  */
struct LEX_FILE {
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
 * Name:    LEX_PARSER
 * Purpose: a LEX_PARSER function tries to parse the token
 *          at the current position
 * Parameters:
 *          hFile [IN] - the file that we are parsing
 *          ptToken [IN OUT] - the token to fill with the token data
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          If the next token isn't from the type this function handles,
 *          the function should return GLOB_ERROR_CONTINUE.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
typedef GLOB_ERROR (*LEX_PARSER)(HLEX_FILE hFile, PLEX_TOKEN ptToken);

/******************************************************************************
 * INTERNAL FUNCTIONS (prototypes)
 * -------------------------------
 * See function-level documentation next to the implementation below
 *****************************************************************************/
static GLOB_ERROR lex_ParseRemark(HLEX_FILE hFile, PLEX_TOKEN ptToken);
static GLOB_ERROR lex_ParseSpecialChar(HLEX_FILE hFile, PLEX_TOKEN ptToken);
static GLOB_ERROR lex_ParseDirective(HLEX_FILE hFile, PLEX_TOKEN ptToken);
static GLOB_ERROR lex_ParseImmediateNumber(HLEX_FILE hFile, PLEX_TOKEN ptToken);
static GLOB_ERROR lex_ParseNumber(HLEX_FILE hFile, PLEX_TOKEN ptToken);
static GLOB_ERROR lex_ParseAlpha(HLEX_FILE hFile, PLEX_TOKEN ptToken);

/******************************************************************************
 * CONSTANTS
 *****************************************************************************/

/* This constant array contains the strings of the opcode in the language.
 * Note: The index of an opcode is equal to its GLOB_OPCODE value. */
static const char * g_aszOpcodes[] = {"mov", "cmp", "add", "sub", "not", "clr",
    "lea", "inc", "dec", "jmp", "bne", "red", "prn", "jsr", "rts", "stop"};

/* array of the parsers. The module tries to parse the token text with each
 * parser in this constant array */
static const LEX_PARSER g_afParsers[] = {
    lex_ParseRemark, lex_ParseSpecialChar,
    lex_ParseDirective, lex_ParseImmediateNumber,
    lex_ParseNumber, lex_ParseAlpha};

/******************************************************************************
 * INTERNAL FUNCTIONS
 *****************************************************************************/

/******************************************************************************
 * Name:    lex_ParseRemark
 * Purpose: Parse remarks in the code. Remark starts with the ';' character
 *          and continues until the end of the line
 * Parameters:
 *          see LEX_PARSER declaration above
 * Return Value:
 *          see LEX_PARSER declaration above
 *****************************************************************************/
static GLOB_ERROR lex_ParseRemark(HLEX_FILE hFile, PLEX_TOKEN ptToken) {
    /* Check if the current char is the remark sign*/
    if (';' != hFile->ptCurrentLine->szLine[hFile->nCurrentColumn]) {
        return GLOB_ERROR_CONTINUE;
    }
    ptToken->eKind = LEX_TOKEN_KIND_REMARK;
    /* Move to the end of line */
    hFile->nCurrentColumn = hFile->nCurrentLineLength;
    
    return GLOB_SUCCESS;
}

/******************************************************************************
 * Name:    lex_ParseSpecialChar
 * Purpose: Parse one-char tokens [ "(", ")", "," ]
 * Parameters:
 *          see LEX_PARSER declaration above
 * Return Value:
 *          see LEX_PARSER declaration above
 *****************************************************************************/
static GLOB_ERROR lex_ParseSpecialChar(HLEX_FILE hFile, PLEX_TOKEN ptToken) {
    char cCurrentChar = '\0';
    cCurrentChar = hFile->ptCurrentLine->szLine[hFile->nCurrentColumn];
    
    /* Check if the current char is one of the one-char tokens */
    if (NULL == strchr(",()", cCurrentChar)) {
        return GLOB_ERROR_CONTINUE;
    }
    ptToken->eKind = LEX_TOKEN_KIND_SPECIAL;
    ptToken->uValue.cChar = cCurrentChar;
    hFile->nCurrentColumn++;
    return GLOB_SUCCESS;
}

/******************************************************************************
 * Name:    lex_ParseDirective
 * Purpose: Parse directive tokens (such as ".data")
 * Parameters:
 *          see LEX_PARSER declaration above
 * Return Value:
 *          see LEX_PARSER declaration above
 *****************************************************************************/
static GLOB_ERROR lex_ParseDirective(HLEX_FILE hFile, PLEX_TOKEN ptToken) {
    int nTokenLength = 0;
    
    /* Directives start with the "." character. */
    if ('.' != hFile->ptCurrentLine->szLine[hFile->nCurrentColumn]) {
        return GLOB_ERROR_CONTINUE;
    }
    
    hFile->nCurrentColumn++;
    while ((hFile->nCurrentColumn < hFile->nCurrentLineLength)
            && isalpha(hFile->ptCurrentLine->szLine[hFile->nCurrentColumn])) {
        hFile->nCurrentColumn++;
    }
    nTokenLength = hFile->nCurrentColumn - ptToken->nColumn;
    // bug error length
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
        return GLOB_ERROR_PARSING_FAILED;
    }
    ptToken->eKind = LEX_TOKEN_KIND_DIRECTIVE;
    return GLOB_SUCCESS;
}

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

static GLOB_ERROR lex_ParseImmediateNumber(HLEX_FILE hFile, PLEX_TOKEN ptToken) {
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    
    /* Check for the '#' sign */
    if ('#' != hFile->ptCurrentLine->szLine[hFile->nCurrentColumn]) {
        return GLOB_ERROR_CONTINUE;
    }
    /* Skip it. */
    hFile->nCurrentColumn++;
    
    /* Now we expect a number*/
    eRetValue = lex_ParseNumber(hFile, ptToken);
    if (GLOB_ERROR_CONTINUE == eRetValue) {
        // expecting number
        return GLOB_ERROR_PARSING_FAILED;
    }
    if (eRetValue) {
        return eRetValue;
    }
    ptToken->eKind = LEX_TOKEN_KIND_IMMED_NUMBER;
    return GLOB_SUCCESS;
}

static GLOB_ERROR lex_ParseNumber(HLEX_FILE hFile, PLEX_TOKEN ptToken) {
    BOOL bFoundDigit = FALSE;
    BOOL bIsMinus = FALSE;
    BOOL bIsPlus = FALSE;
    int nValue = 0;
    
    hFile->nCurrentColumn++; // skip the '#'
    // the current char may be a digit, a plus or a minus sign
    bIsMinus = ('-' == hFile->ptCurrentLine->szLine[hFile->nCurrentColumn]);
    bIsMinus = ('+' == hFile->ptCurrentLine->szLine[hFile->nCurrentColumn]);
    if (!bIsMinus && !bIsPlus && !isdigit(hFile->ptCurrentLine->szLine[hFile->nCurrentColumn])) {
        return GLOB_ERROR_CONTINUE;
    }
    if (bIsMinus || bIsPlus) {
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
    if ('"' != hFile->ptCurrentLine->szLine[hFile->nCurrentColumn]) {
        return GLOB_ERROR_CONTINUE;
    }
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

static GLOB_ERROR lex_ParseAlpha(HLEX_FILE hFile, PLEX_TOKEN ptToken) {
    BOOL bIsLabelDefinition = FALSE;
    int nIdentifierLength = 0;
    int nOpcode = -1;
    
    if (!isalpha(hFile->ptCurrentLine->szLine[hFile->nCurrentColumn])) {
        return GLOB_ERROR_CONTINUE;
    }
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
 * Name:    lex_MoveToNextToken
 * Purpose: The function moves the parser position to the next token to parse
 * Parameters:
 *          hFile [IN] - handle to the file we are parsing
 *          peFlags [OUT] - flags describing the next token to parse
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          If there are no more tokens in the current line,
 *          GLOB_ERROR_END_OF_LINE is returned.
 *          GLOB_ERROR_END_OF_FILE is returned if finished to process the file.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
static GLOB_ERROR lex_MoveToNextToken(HLEX_FILE hFile,
                                      PLEX_TOKEN_FLAGS peFlags) {
    BOOL bFirstToken = FALSE;
    BOOL bNoSpaceFromPrevToken = TRUE;
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    
    /* If no line is in parsing, read the next line. */
    if (NULL == hFile->ptCurrentLine) {
        eRetValue = LINESTR_GetNextLine(hFile->hSourceFile, &hFile->ptCurrentLine);
        if (eRetValue) {
            /* Including EOF */
            return eRetValue;
        }
        hFile->nCurrentColumn = 0;
        hFile->nCurrentLineLength = strlen(hFile->ptCurrentLine->szLine);

        bFirstToken = TRUE;
        bNoSpaceFromPrevToken = FALSE;
    }
    
    /* Skip white chars (spaces and tabs) */
    while (hFile->nCurrentColumn < hFile->nCurrentLineLength) {
        if ((' ' != hFile->ptCurrentLine->szLine[hFile->nCurrentColumn])
            && ('\t' != hFile->ptCurrentLine->szLine[hFile->nCurrentColumn])) {
            break;
        }
        bNoSpaceFromPrevToken = FALSE;
        hFile->nCurrentColumn++;
    }
    
    /* Set the flags for the next token */
    *peFlags = (bFirstToken ? LEX_TOKEN_FLAGS_FIRST_TOKEN_IN_LINE : 0)
              | (bNoSpaceFromPrevToken ?
                  LEX_TOKEN_FLAGS_NO_SPACE_FROM_PREV_TOKEN : 0);
    
    return (hFile->nCurrentColumn >= hFile->nCurrentLineLength ?
        GLOB_ERROR_END_OF_LINE : GLOB_SUCCESS);
}

/******************************************************************************
 * LEX_ReadNextToken
 *****************************************************************************/
GLOB_ERROR LEX_ReadNextToken(HLEX_FILE hFile, PLEX_TOKEN * pptToken) {
    PLEX_TOKEN ptToken = NULL;
    LEX_TOKEN_FLAGS eTokenFlags = 0;
    
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    
    /* Check Parameters */
    if (NULL == hFile || NULL == pptToken) {
        return GLOB_ERROR_INVALID_PARAMETERS;
    }

    /* Move to next token */
    eRetValue = lex_MoveToNextToken(hFile, &eTokenFlags);
    if (eRetValue) {
        return eRetValue;
    }
    
    /* Allocate a token */
    ptToken = malloc(sizeof(*ptToken));
    if (NULL == ptToken) {
        // todo error handling
        return GLOB_ERROR_SYS_CALL_ERROR();
    }
    
    /* Set common properties */
    ptToken->eFlags = eTokenFlags;
    ptToken->nColumn = hFile->nCurrentColumn;
    
    for (int nParserIndex = 0;
            nParserIndex < ARRARY_ELEMENTS(g_afParsers);
            nParserIndex++) {
        eRetValue = g_afParsers[nParserIndex](hFile, ptToken);
        if (GLOB_ERROR_CONTINUE != eRetValue) {
            break;
        }
    }
    if (GLOB_ERROR_CONTINUE == eRetValue) {
        /* No parser found */
        eRetValue = GLOB_ERROR_PARSING_FAILED;
    }
    
    if (eRetValue) {
        /* Failed to parse the current token. */
        free(ptToken);
        return eRetValue;
    }
    
    /* Set out parameters */
    *pptToken = ptToken;
    return GLOB_SUCCESS;
}

/******************************************************************************
 * LEX_GetCurrentPosition
 *****************************************************************************/
GLOB_ERROR LEX_GetCurrentPosition(HLEX_FILE hFile, PLINESTR_LINE * pptLine,
                                  int * pnColumn) {
    /* Check parameters */
    if (NULL == hFile || NULL == pptLine || NULL == pnColumn) {
        return GLOB_ERROR_INVALID_PARAMETERS;
    }
    
    /* Fill out parameters */
    *pptLine = hFile->ptCurrentLine;
    *pnColumn = hFile->nCurrentColumn + 1; /* From zero-based to one-based */
    return GLOB_SUCCESS;
}

/******************************************************************************
 * LEX_MoveToNextLine
 *****************************************************************************/
GLOB_ERROR LEX_MoveToNextLine(HLEX_FILE hFile){
    /* Check parameters */
    if (NULL == hFile) {
        return GLOB_ERROR_INVALID_PARAMETERS;
    }
    if (NULL != hFile->ptCurrentLine) {
        LINESTR_FreeLine(hFile->ptCurrentLine);
        hFile->ptCurrentLine = NULL;
    }
    return GLOB_SUCCESS;
}

/******************************************************************************
 * LEX_FreeToken
 *****************************************************************************/
void LEX_FreeToken(PLEX_TOKEN ptToken){
    if (NULL == ptToken) {
        return;
    }
    
    /* In case tokens with a string value, free it*/
    if (LEX_TOKEN_KIND_STRING == ptToken->eKind
            || LEX_TOKEN_KIND_LABEL == ptToken->eKind
            || LEX_TOKEN_KIND_WORD == ptToken->eKind) {
        free(ptToken->uValue.szStr);
    }
    
    /* Free the token itself */
    free(ptToken);
}

/******************************************************************************
 * LEX_Close
 *****************************************************************************/
void LEX_Close(HLEX_FILE hFile) {
    if (NULL == hFile) { 
        return;
    }
    /* Free current line */
    LEX_MoveToNextLine(hFile);

    /* Close the source file*/    
    LINESTR_Close(hFile->hSourceFile);
    
    /* Free the handle */
    free(hFile);
}
