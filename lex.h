/******************************************************************************
 * File:    lex.h
 * Author:  Doron Shvartztuch
 * The LEX module is responsible for parsing the source file
 * It performs lexical parsing of the source file to tokens from different
 * types as described below.
 *****************************************************************************/

#ifndef LEX_H
#define LEX_H

/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include "global.h"
#include "linestr.h"

/******************************************************************************
 * TYPEDEFS
 *****************************************************************************/

/* The module parses the text into tokens from various types. the following
 * enum contains the possible kind of tokens. For each token see the remark
 * about the relevant field to use as a value (see LEX_TOKEN_VALUE). */
typedef enum LEX_TOKEN_KIND {   /* Description                  Value field*/
                                /* --------------------------   -----------*/
    LEX_TOKEN_KIND_LABEL,       /* Label definition             szStr      */
    LEX_TOKEN_KIND_WORD,        /* Label usage                  szStr      */
    LEX_TOKEN_KIND_NUMBER,      /* Number (in .data line)       nNumber    */
    LEX_TOKEN_KIND_IMMED_NUMBER,/* Number (immediate address)   nNumber    */
    LEX_TOKEN_KIND_OPCODE,      /* Opcode                       eOpcode    */
    LEX_TOKEN_KIND_REGISTER,    /* Register                     nNumber    */
    LEX_TOKEN_KIND_DIRECTIVE,   /* Directive                    eDirective */
    LEX_TOKEN_KIND_STRING,      /* String                       szStr      */
    LEX_TOKEN_KIND_SPECIAL,     /* Special char                 cChar      */
    LEX_TOKEN_KIND_REMARK,      /* Remark                       no value   */
} LEX_TOKEN_KIND;

/* This union defines the possible values for token.
 * See LEX_TOKEN_KIND documentation above. */
typedef union LEX_TOKEN_VALUE {
    char * szStr;
    int nNumber;
    GLOB_OPCODE eOpcode;
    GLOB_DIRECTIVE eDiretive;
    char cChar;
} LEX_TOKEN_VALUE;

/* The token contains a flags field. The flags add some
 * meta data about the token. */
typedef enum LEX_TOKEN_FLAGS {
    
    /* The flag is set if this is the first token in the current line. */
    /* The flag is set even if the token doesn't start in the first column */
    LEX_TOKEN_FLAGS_FIRST_TOKEN_IN_LINE = 1,
            
    /* The flag is set if there is no white chars (spaces, tabs) from the
     * previous token. */
    LEX_TOKEN_FLAGS_NO_SPACE_FROM_PREV_TOKEN = 2,
} LEX_TOKEN_FLAGS, *PLEX_TOKEN_FLAGS;

/* LEX_TOKEN encapsulates the information about a token. The caller must free
 * the token with LEX_FreeToken. */
typedef struct LEX_TOKEN {
    
    /* The kind of token. It determines the value type.
     * See LEX_TOKEN_KIND declaration for more information. */
    LEX_TOKEN_KIND eKind;
    
    /* The source line of the token. */
    PLINESTR_LINE ptLine;
    
    /* The Column of the token. First column gets 0. */
    int nColumn;
    
    /* Some meta-data about the token. See LEX_TOKEN_FLAGS for more info. */
    LEX_TOKEN_FLAGS eFlags;
    
    /* The value of the token. The value type is determined by the token kind.*/
    LEX_TOKEN_VALUE uValue;
} LEX_TOKEN, *PLEX_TOKEN;

/* The HLEX_FILE represents a handle to a file opened by the LEX_Open
 * function. Always close the handle with the LEX_Close function. */
typedef struct LEX_FILE LEX_FILE, *HLEX_FILE, **PHLEX_FILE;

/******************************************************************************
 * FUNCTIONS
 *****************************************************************************/

/******************************************************************************
 * Name:    LEX_Open
 * Purpose: The function opens a source file for parsing.
 * Parameters:
 *          szFileName [IN] - the path to the file to open (w/o the extension)
 *          pfnErrorsCallback [IN] - callback function for errors/warnings
 *          pvContext [IN] - context for the callback function
 *          phFile [OUT] - the handle to the opened file
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          In this case, a handle to the opened file is returned in phFile and
 *          the caller must close it with LEX_Close.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
GLOB_ERROR LEX_Open(const char * szFileName,
                    GLOB_ErrorOrWarningCallback pfnErrorsCallback,
                    void * pvContext,
                    PHLEX_FILE phFile);

/******************************************************************************
 * Name:    LEX_ReadNextToken
 * Purpose: The function reads the next token from the current line in the
 *          source file.
 * Parameters:
 *          hFile [IN] - handle to the file, previously opened by LEX_Open.
 *          pptToken [OUT] - the retrieved token.
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          In this case, the token must be freed with LEX_FreeToken.
 *          If there are no more tokens in the current line,
 *          GLOB_ERROR_END_OF_LINE is returned. Any further calls will fail with
 *          the same error until a call to LEX_MoveToNextLine.
 *          GLOB_ERROR_END_OF_FILE is returned if finished to process the file.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
GLOB_ERROR LEX_ReadNextToken(HLEX_FILE hFile, PLEX_TOKEN * pptToken);

/******************************************************************************
 * Name:    LEX_GetCurrentPosition
 * Purpose: Provide information about the current position in the file
 *          of the parser
 * Parameters:
 *          hFile [IN] - handle to the file, previously opened by LEX_Open.
 *          pptLine [OUT] - contains the line level information of the current
 *                          posision of the parser. See remarks.
 *          pnColumn [OUT] - the current column.
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          If the function fails, an error code is returned.
 * Remarks:
 *          The caller can use the returned LINESTR_LINE struct until another
 *          call to any function of this module with the same hFile. Then,
 *          the LINESTR_LINE may become unavailable.
 *          The Caller should NOT free the LINESTR_LINE.
 *****************************************************************************/
GLOB_ERROR LEX_GetCurrentPosition(HLEX_FILE hFile, PLINESTR_LINE * pptLine,
                                  int * pnColumn);

/******************************************************************************
 * Name:    LEX_MoveToNextLine
 * Purpose: Move to the next line in the source file
 * Parameters:
 *          hFile [IN] - handle to the file, previously opened by LEX_Open.
 *****************************************************************************/
void LEX_MoveToNextLine(HLEX_FILE hFile);

/******************************************************************************
 * Name:    LEX_FreeTokenLEX_FreeToken
 * Purpose: The function frees a token previously returned
 *          from LEX_ReadNextToken.
 * Parameters:
 *          ptToken [IN] - the token to free.
 *****************************************************************************/
void LEX_FreeToken(PLEX_TOKEN ptToken);

/******************************************************************************
 * Name:    LEX_Close
 * Purpose: The function closes a file previously opened by LEX_Open
 * Parameters:
 *          hFile [IN] - handle to the file to close
 *****************************************************************************/
void LEX_Close(HLEX_FILE hFile);

#endif /* LEX_H */
