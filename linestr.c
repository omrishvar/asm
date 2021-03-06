/*****************************************************************************
 * File:    linestr.c
 * Author:  Doron Shvartztuch
 * The LINESTR module is responsible for reading the source files
 * and split the text to lines.
 * See linestr.h for more documentation.
 * 
 * Implementation:
 * File I/O operations are performed with standard C library functions.
 * The LINESTR_HANDLE contains the information for reading the next lines from
 * the source file and counting the rows.
 *****************************************************************************/

/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "helper.h"
#include "linestr.h"

/******************************************************************************
 * TYPEDEFS
 *****************************************************************************/

/* LINESTR_FILE is the struct behind the the HLINESTR_FILE.
 * It keeps the FILE* to the file itself and the number of the
 * next row to be read  */
struct LINESTR_FILE {
    
    /* Full source file name */
    char * pszFullFileName;
    
    /* The source file */
    FILE * phSourceFile;
    
    /* Number of the next row that will be read. First row gets 1 */
    int nLineNumber; 
};

/******************************************************************************
 * EXTERNAL FUNCTIONS
 * ------------------
 * See function-level documentation in the header file
 *****************************************************************************/

/******************************************************************************
 * LINESTR_Open
 *****************************************************************************/
GLOB_ERROR LINESTR_Open(const char * szFileName,  PHLINESTR_FILE phFile) {
    HLINESTR_FILE hFile = NULL;
    GLOB_ERROR eRetVal = GLOB_ERROR_UNKNOWN;
    
    /* Check parameters */
    if (NULL == szFileName || NULL == phFile) {
        return GLOB_ERROR_INVALID_PARAMETERS;
    }
    
    /* Allocate the handle */
    hFile = malloc(sizeof(*hFile));
    if (NULL == hFile) {
        return GLOB_ERROR_SYS_CALL_ERROR();
    }
    
    /* Get the full file name to open */
    hFile->pszFullFileName = HELPER_ConcatStrings(szFileName,
        GLOB_FILE_EXTENSION_SOURCE);
    if (NULL == hFile->pszFullFileName) {
        eRetVal = GLOB_ERROR_SYS_CALL_ERROR();
        free(hFile);
        return eRetVal;
    }

    /* Open the file */
    hFile->phSourceFile = fopen(hFile->pszFullFileName, "r");
    if (hFile->phSourceFile == NULL) {
        eRetVal = GLOB_ERROR_SYS_CALL_ERROR();
        free(hFile->pszFullFileName);
        free(hFile);
        return eRetVal;
    }
    
    /* Init the line counter */
    hFile->nLineNumber = 1;
    
    /* Set out parameter upon success */
    *phFile = hFile;
    return GLOB_SUCCESS;
}

/******************************************************************************
 * LINESTR_GetFullFileName
 *****************************************************************************/
const char * LINESTR_GetFullFileName(HLINESTR_FILE hFile) {
    if (NULL == hFile) {
        return NULL;
    }
    return hFile->pszFullFileName;
}
/******************************************************************************
 * LINESTR_GetNextLine
 *****************************************************************************/
GLOB_ERROR LINESTR_GetNextLine(HLINESTR_FILE hFile, PLINESTR_LINE * pptLine) {
    PLINESTR_LINE ptLine = NULL;
    GLOB_ERROR eRetVal = GLOB_ERROR_UNKNOWN;
    
    /* Check parameters */
    if (NULL == hFile || NULL == pptLine) {
        return GLOB_ERROR_INVALID_PARAMETERS;
    }

    /* allocate a new LINESTR_LINE structure */
    ptLine = malloc(sizeof(*ptLine));
    if (NULL == ptLine) {
        return GLOB_ERROR_SYS_CALL_ERROR();
    }
    
    /* Newly created. have just 1 reference. */
    ptLine->nReferences = 1;
    
    /* Set the row number */
    ptLine->nLineNumber = hFile->nLineNumber;
    
    /* set the reference to the file. */
    ptLine->hFile = hFile;
    
    /* Read the line */
    if (NULL == fgets(ptLine->szLine, sizeof(ptLine->szLine),
        hFile->phSourceFile)) {
        eRetVal = GLOB_ERROR_SYS_CALL_ERROR();
        free(ptLine);
        /* fgets returns NULL in case of either error or EOF, so check it */
        return feof(hFile->phSourceFile) ? GLOB_ERROR_END_OF_FILE : eRetVal;
    }

    TERMINATE_STRING(ptLine->szLine);
    
    /* Trunk the '\n', if exists from the string. */
    if (ptLine->szLine[strlen(ptLine->szLine)-1] == '\n') {
        ptLine->szLine[strlen(ptLine->szLine)-1] = '\0';
    }
    
    /* Increment the rows counter */
    hFile->nLineNumber++;

    /* Set out parameters */
    *pptLine = ptLine;
    return GLOB_SUCCESS;
}

/******************************************************************************
 * Name:    LINESTR_LineAddRef
 *****************************************************************************/
void LINESTR_LineAddRef(PLINESTR_LINE ptLine) {
    if (NULL != ptLine) {
        ptLine->nReferences++;
    }
}

/******************************************************************************
 * LINESTR_FreeLine
 *****************************************************************************/
void LINESTR_FreeLine(PLINESTR_LINE ptLine) {
    if (NULL != ptLine) {
        ptLine->nReferences--;
        if (0 == ptLine->nReferences) {
            free(ptLine);
        }
    }
}

/******************************************************************************
 * LINESTR_Close
 *****************************************************************************/
void LINESTR_Close(HLINESTR_FILE hFile) {
    if (NULL != hFile) {
        fclose(hFile->phSourceFile);
        free(hFile->pszFullFileName);
        free(hFile);
    }
}
