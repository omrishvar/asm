/******************************************************************************
 * File:    linestr.h
 * Author:  Doron Shvartztuch
 * The LINESTR module is responsible for reading the source files
 * and split the text to lines.
 *****************************************************************************/

#ifndef LINESTR_H
#define LINESTR_H

/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include "global.h"

/******************************************************************************
 * CONSTANTS & MACROS
 *****************************************************************************/

/* a line should have maximum 80 chars plus 1 for '\0' */
#define LINESTR_MAX_LINE_LENGTH 81

/******************************************************************************
 * TYPEDEFS
 *****************************************************************************/

/* The HLINESTR_FILE represents a handle to a file opened by the LINESTR_Open
 * function. Always close the handle with the LINESTR_Close function. */
typedef struct LINESTR_FILE LINESTR_FILE, *HLINESTR_FILE, **PHLINESTR_FILE;

/* The LINESTR_LINE struct includes a data of a row read from the source file.
 * After finishing using the LINESTR_LINE, use LINESTR_FreeLine. */
typedef struct LINESTR_LINE {
    /* The original line from the source file.
     * It doesn't contain the '\n' character. */
    char szLine[LINESTR_MAX_LINE_LENGTH];
    
    /* Row number in the source file.
     * First row gets 1. */
    int nLineNumber;
} LINESTR_LINE, *PLINESTR_LINE, **PPLINESTR_LINE;

/******************************************************************************
 * FUNCTIONS
 *****************************************************************************/

/******************************************************************************
 * Name:    LINESTR_Open
 * Purpose: The function opens a source file. The caller gets a handle to the
 *          opened file.
 * Parameters:
 *          szFileName [IN] - the path to the file to open
 *          phFile [OUT] - the handle to the opened file
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          In this case, a handle to the opened file is returned in phFile and
 *          the caller must close it with LINESTR_Close.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
GLOB_ERROR LINESTR_Open(const char * szFilenName, PHLINESTR_FILE phFile);

/******************************************************************************
 * Name:    LINESTR_GetNextLine
 * Purpose: The function reads the next line from the file
 * Parameters:
 *          hFile [IN] - handle to the file, previously opened by LINESTR_Open.
 *          pptLine [OUT] - the line retrieved by the function.
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          In this case, a pointer to a LINESTR_LINE struct is returned to the
 *          caller in pptLine. the caller must free it with LINESTR_FreeLine.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
GLOB_ERROR LINESTR_GetNextLine(HLINESTR_FILE hFile, PPLINESTR_LINE pptLine);

/******************************************************************************
 * Name:    LINESTR_FreeLine
 * Purpose: The function frees a LINESTR_LINE struct previously returned
 *          by LINESTR_GetNextLine
 * Parameters:
 *          ptLine [IN] - pointer to the LINESTR_LINE struct to free
 *****************************************************************************/
void LINESTR_FreeLine(PLINESTR_LINE ptLine);

/******************************************************************************
 * Name:    LINESTR_Close
 * Purpose: The function closes a file previously opened by LINESTR_Open
 * Parameters:
 *          hFile [IN] - handle to the file to close
 *****************************************************************************/
void LINESTR_Close(HLINESTR_FILE hFile);

#endif /* LINESTR_H */
