/*****************************************************************************
 * File:    lex.c
 * Author:  Doron Shvartztuch
 * The LEX module is responsible for parsing the source file
 * It performs lexical parsing of the source file to tokens from different
 * types as described below.
 * 
 * Implementation:
 * The LEX modules uses LINESTR to read the source file into lines.
 * It goes over the lines and parse the text into tokens from different types.
 *****************************************************************************/

/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "helper.h"
#include "global.h"
#include "linestr.h"
#include "lex.h"

/******************************************************************************
 * CONSTANTS & MACROS
 *****************************************************************************/

/* The maximum length (in characters) of a label */
#define LEX_MAX_LABEL_LENGTH 31

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
    
    /* Callback function to use for errors and warnings */
    GLOB_ERRORCALLBACK pfnErrorsCallback;
    
    /* Context for the callback function */
    void * pvContext;
    
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
static void lex_ReportError(HLEX_FILE hFile, BOOL bIsError, int nColumn,
                            const char * pszErrorFormat, ...);
static GLOB_ERROR lex_ParseRemark(HLEX_FILE hFile, PLEX_TOKEN ptToken);
static GLOB_ERROR lex_ParseSpecialChar(HLEX_FILE hFile, PLEX_TOKEN ptToken);
static GLOB_ERROR lex_ParseDirective(HLEX_FILE hFile, PLEX_TOKEN ptToken);
static GLOB_ERROR lex_ParseImmediateNumber(HLEX_FILE hFile, PLEX_TOKEN ptToken);
static GLOB_ERROR lex_ParseNumber(HLEX_FILE hFile, PLEX_TOKEN ptToken);
static GLOB_ERROR lex_ParseString(HLEX_FILE hFile, PLEX_TOKEN ptToken);
static GLOB_ERROR lex_ParseAlpha(HLEX_FILE hFile, PLEX_TOKEN ptToken);

/******************************************************************************
 * CONSTANTS
 *****************************************************************************/

/* This constant array contains the strings of the opcode in the language.
 * Note: The index of an opcode is equal to its GLOB_OPCODE value. */
static const char * g_aszOpcodes[] = {"mov", "cmp", "add", "sub", "not", "clr",
    "lea", "inc", "dec", "jmp", "bne", "red", "prn", "jsr", "rts", "stop"};

/* This constant array contains the strings of the directives in the language.
 * Note: The index of a directive is equal to its GLOB_DIRECTIVE value. */
static const char * g_aszDirectives[] = {"data", "string", "entry", "extern"};

/* This constant array contains the strings of the registers in the language.
 * Note: The index of a register is equal to its number */
static const char * g_aszRegisters[] = {"r0", "r1", "r2", "r3",
                                        "r4", "r5", "r6", "r7"};

/* array of the parsers. The module tries to parse the token text with each
 * parser in this constant array */
static const LEX_PARSER g_afParsers[] = {
    lex_ParseRemark, lex_ParseSpecialChar,
    lex_ParseDirective, lex_ParseImmediateNumber,
    lex_ParseNumber, lex_ParseString, lex_ParseAlpha};

/******************************************************************************
 * INTERNAL FUNCTIONS
 *****************************************************************************/

/******************************************************************************
 * Name:    lex_ReportError
 * Purpose: Print parsing error message
 * Parameters:
 *          hFile [IN] - handle to the current file
 *          bIsError [IN] - TRUE for error. FALSE for warning
 *          nColumn [IN] - zero-based column index of the error
 *          pszErroFormat[IN] - error message (format as printf syntax)
 *          ... [IN] - parameters to include in the message
  *****************************************************************************/
static void lex_ReportError(HLEX_FILE hFile, BOOL bIsError, int nColumn,
                            const char * pszErrorFormat, ...) {
    va_list vaArgs;
    va_start (vaArgs, pszErrorFormat);
    hFile->pfnErrorsCallback(hFile->pvContext,
                             LINESTR_GetFullFileName(hFile->hSourceFile),
                             hFile->ptCurrentLine->nLineNumber,
                             nColumn+1, 
                             hFile->ptCurrentLine->szLine,
                             bIsError, pszErrorFormat, vaArgs);
    va_end (vaArgs);
}

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
    int nDirectiveIndex = 0;
    
    /* Directives start with the "." character. */
    if ('.' != hFile->ptCurrentLine->szLine[hFile->nCurrentColumn]) {
        return GLOB_ERROR_CONTINUE;
    }
    
    /* Skip the '.' */
    hFile->nCurrentColumn++;
    
    /* Read the directive name */
    while ((hFile->nCurrentColumn < hFile->nCurrentLineLength)
            && isalpha(hFile->ptCurrentLine->szLine[hFile->nCurrentColumn])) {
        hFile->nCurrentColumn++;
    }
    
    /* Calculate the directive length (without the '.') */
    nTokenLength = hFile->nCurrentColumn - ptToken->nColumn - 1;
    
    /* Search the directives array */
    nDirectiveIndex = HELPER_FindInStringsArray(
                g_aszDirectives,
                ARRAY_ELEMENTS(g_aszDirectives),
                hFile->ptCurrentLine->szLine+ptToken->nColumn+1,
                nTokenLength);
    
    if (-1 == nDirectiveIndex) {
        /* Unknown directive error */
        lex_ReportError(hFile, TRUE, ptToken->nColumn, "Unknown directive");
        return GLOB_ERROR_PARSING_FAILED;
    }
    ptToken->uValue.eDiretive = (GLOB_DIRECTIVE)nDirectiveIndex;
    ptToken->eKind = LEX_TOKEN_KIND_DIRECTIVE;
    return GLOB_SUCCESS;
}

/******************************************************************************
 * Name:    lex_ParseImmediateNumber
 * Purpose: Parse immediate numbers (such as "#-780")
 * Parameters:
 *          see LEX_PARSER declaration above
 * Return Value:
 *          see LEX_PARSER declaration above
 *****************************************************************************/
static GLOB_ERROR lex_ParseImmediateNumber(HLEX_FILE hFile, PLEX_TOKEN ptToken){
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
        lex_ReportError(hFile, TRUE, hFile->nCurrentColumn,
                "a number is expected");
        return GLOB_ERROR_PARSING_FAILED;
    }
    if (eRetValue) {
        return eRetValue;
    }
    ptToken->eKind = LEX_TOKEN_KIND_IMMED_NUMBER;
    return GLOB_SUCCESS;
}

/******************************************************************************
 * Name:    lex_ParseNumber
 * Purpose: Parse numbers (such as "-780")
 * Parameters:
 *          see LEX_PARSER declaration above
 * Return Value:
 *          see LEX_PARSER declaration above
 *****************************************************************************/
static GLOB_ERROR lex_ParseNumber(HLEX_FILE hFile, PLEX_TOKEN ptToken) {
    BOOL bFoundDigit = FALSE;
    BOOL bIsMinus = FALSE;
    BOOL bIsPlus = FALSE;
    int nValue = 0;
    
    /* the first char may be a digit, a plus or a minus sign */
    bIsMinus = ('-' == hFile->ptCurrentLine->szLine[hFile->nCurrentColumn]);
    bIsPlus = ('+' == hFile->ptCurrentLine->szLine[hFile->nCurrentColumn]);
    if (!bIsMinus && !bIsPlus
            && !isdigit(hFile->ptCurrentLine->szLine[hFile->nCurrentColumn])) {
        return GLOB_ERROR_CONTINUE;
    }
    
    /* Skip the sign */
    if (bIsMinus || bIsPlus) {
        hFile->nCurrentColumn++;
    }
    
    /* Now we expect the value (digits) */
    while ((hFile->nCurrentColumn < hFile->nCurrentLineLength)
            && isdigit(hFile->ptCurrentLine->szLine[hFile->nCurrentColumn])) {
        bFoundDigit = TRUE;
        nValue = 10*nValue +
                hFile->ptCurrentLine->szLine[hFile->nCurrentColumn] - '0';
        hFile->nCurrentColumn++;
    }
    
    /* Check that we have at least one digit */
    if (!bFoundDigit) {
        lex_ReportError(hFile, TRUE, ptToken->nColumn,
                "A number must incluse at least one digit");
        return GLOB_ERROR_PARSING_FAILED;
    }
    
    /* Set the value */
    ptToken->eKind = LEX_TOKEN_KIND_NUMBER;
    ptToken->uValue.nNumber = bIsMinus ? -nValue : nValue;
    return GLOB_SUCCESS;
}

/******************************************************************************
 * Name:    lex_ParseString
 * Purpose: Parse strings ("...")
 * Parameters:
 *          see LEX_PARSER declaration above
 * Return Value:
 *          see LEX_PARSER declaration above
 *****************************************************************************/
static GLOB_ERROR lex_ParseString(HLEX_FILE hFile, PLEX_TOKEN ptToken) {
    /* Strings begin with the '"' character*/
    if ('"' != hFile->ptCurrentLine->szLine[hFile->nCurrentColumn]) {
        return GLOB_ERROR_CONTINUE;
    }
    
    /* Skip the opening '"' */
    hFile->nCurrentColumn++;
    
    /* Now we are searching for the closing '"' */
    while ((hFile->nCurrentColumn < hFile->nCurrentLineLength)
            && ('"' != hFile->ptCurrentLine->szLine[hFile->nCurrentColumn])) {
        hFile->nCurrentColumn++;
    }
    
    /* If we didn't find the closing '"', report error */
    if (hFile->nCurrentColumn >= hFile->nCurrentLineLength) {
        lex_ReportError(hFile, TRUE, ptToken->nColumn, "No closing \" found");
        return GLOB_ERROR_PARSING_FAILED;
    }
    
    /* Allocate space for the value of the token */
    ptToken->uValue.szStr = malloc(hFile->nCurrentColumn - ptToken->nColumn );
    if (NULL == ptToken->uValue.szStr) {
        return GLOB_ERROR_SYS_CALL_ERROR();
    }
    strncpy(ptToken->uValue.szStr,
        hFile->ptCurrentLine->szLine + ptToken-> nColumn + 1,
        hFile->nCurrentColumn - ptToken->nColumn - 1);
    /* Skip the closing '"' */
    hFile->nCurrentColumn++;
    
    ptToken->eKind = LEX_TOKEN_KIND_STRING;
    return GLOB_SUCCESS;
}

/******************************************************************************
 * Name:    lex_ParseAlpha
 * Purpose: Parse tokens that start with a letter
 * Parameters:
 *          see LEX_PARSER declaration above
 * Return Value:
 *          see LEX_PARSER declaration above
 *****************************************************************************/
static GLOB_ERROR lex_ParseAlpha(HLEX_FILE hFile, PLEX_TOKEN ptToken) {
    BOOL bIsLabelDefinition = FALSE;
    int nIdentifierLength = 0;
    int nOpcode = -1;
    int nDirective = -1;
    int nRegister = -1;
    
    /* The first character should be a letter */
    if (!isalpha(hFile->ptCurrentLine->szLine[hFile->nCurrentColumn])) {
        return GLOB_ERROR_CONTINUE;
    }
    /* Skip the first char */
    hFile->nCurrentColumn++;
    
    /* The remaining characters may be either letters or digits */
    while ((hFile->nCurrentColumn < hFile->nCurrentLineLength)
            && (isalpha(hFile->ptCurrentLine->szLine[hFile->nCurrentColumn])
               ||isdigit(hFile->ptCurrentLine->szLine[hFile->nCurrentColumn]))){
        hFile->nCurrentColumn++;
    }
    
    /* Check for the maximum length */
    nIdentifierLength = hFile->nCurrentColumn - ptToken->nColumn;
    if (nIdentifierLength  > LEX_MAX_LABEL_LENGTH) {
        lex_ReportError(hFile, TRUE, ptToken->nColumn, "Too long label");
        return GLOB_ERROR_PARSING_FAILED;
    }
    
    /* Check if the next char is semi-colon, indicates label definition. */
    bIsLabelDefinition =  ((hFile->nCurrentColumn < hFile->nCurrentLineLength)
            && ':' == hFile->ptCurrentLine->szLine[hFile->nCurrentColumn]);
    
    /* Check if this is an opcode, register or directive*/
    nOpcode = HELPER_FindInStringsArray(g_aszOpcodes,
                ARRAY_ELEMENTS(g_aszOpcodes),
                hFile->ptCurrentLine->szLine + ptToken->nColumn,
                nIdentifierLength);
    nRegister = HELPER_FindInStringsArray(g_aszRegisters,
                ARRAY_ELEMENTS(g_aszRegisters),
                hFile->ptCurrentLine->szLine + ptToken->nColumn,
                nIdentifierLength);
    nDirective = HELPER_FindInStringsArray(g_aszDirectives,
                ARRAY_ELEMENTS(g_aszDirectives),
                hFile->ptCurrentLine->szLine + ptToken->nColumn,
                nIdentifierLength);
    /* Opcode, Directive and Registers are forbidden as labels */
    if ((-1 != nDirective) ||
            (bIsLabelDefinition && ((-1 != nOpcode) || (-1 != nRegister)))) {
        lex_ReportError(hFile, TRUE, ptToken->nColumn,
                "Forbidden label name");
        return GLOB_ERROR_PARSING_FAILED;
    }
    
    /* Check for opcode. */
    if (-1 != nOpcode) {
        ptToken->eKind = LEX_TOKEN_KIND_OPCODE;
        ptToken->uValue.eOpcode = nOpcode;
        return GLOB_SUCCESS;
    }
    
    /* Check for register. */
    if (-1 != nRegister) {
        ptToken->eKind = LEX_TOKEN_KIND_REGISTER;
        ptToken->uValue.nNumber = nRegister;
        return GLOB_SUCCESS;
    }
    
    /* For label (definition/usage) we need to copy the string */
    ptToken->uValue.szStr = malloc(hFile->nCurrentColumn -ptToken->nColumn + 1);
    if (NULL == ptToken->uValue.szStr) {
        return GLOB_ERROR_SYS_CALL_ERROR();
    }
    strncpy(ptToken->uValue.szStr,
            hFile->ptCurrentLine->szLine + ptToken->nColumn,
            hFile->nCurrentColumn - ptToken->nColumn);
    ptToken->eKind = bIsLabelDefinition ?
                     LEX_TOKEN_KIND_LABEL :
                     LEX_TOKEN_KIND_WORD;
    /* Skip ":" of label definition */
    hFile->nCurrentColumn += bIsLabelDefinition;
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
        eRetValue = LINESTR_GetNextLine(hFile->hSourceFile,
                                        &hFile->ptCurrentLine);
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
 * EXTERNAL FUNCTIONS
 * ------------------
 * See function-level documentation in the header file
 *****************************************************************************/

/******************************************************************************
 * LEX_Open
 *****************************************************************************/
GLOB_ERROR LEX_Open(const char * szFileName,
                    GLOB_ERRORCALLBACK pfnErrorsCallback,
                    void * pvContext,
                    PHLEX_FILE phFile){
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
    hFile->pfnErrorsCallback = pfnErrorsCallback;
    hFile->pvContext = pvContext;
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
        return GLOB_ERROR_SYS_CALL_ERROR();
    }
    
    /* Set common properties */
    ptToken->eFlags = eTokenFlags;
    ptToken->nColumn = hFile->nCurrentColumn;
    
    for (int nParserIndex = 0;
            nParserIndex < ARRAY_ELEMENTS(g_afParsers);
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
    
    /* Set the line of the token */
    ptToken->ptLine = hFile->ptCurrentLine;
    LINESTR_LineAddRef(ptToken->ptLine);
    
    /* Set out parameters */
    *pptToken = ptToken;
    return GLOB_SUCCESS;
}

// TODO REMOVE FUNCTION
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
void LEX_MoveToNextLine(HLEX_FILE hFile){
    /* Check parameters */
    if (NULL == hFile) {
        return;
    }
    if (NULL != hFile->ptCurrentLine) {
        LINESTR_FreeLine(hFile->ptCurrentLine);
        hFile->ptCurrentLine = NULL;
    }
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
    
    LINESTR_FreeLine(ptToken->ptLine);
    
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
