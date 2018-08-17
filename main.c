/*****************************************************************************
 * File:    main.c
 * Author:  Doron Shvartztuch
 * The main module contains the entry point of the assembler.
 * It is responsible for parsing the command line and print the output messages
 * to the stdout.
 * 
 * Implementation:
 * After parsing the command line, we call the ASM module to compile the file
 * and use the OUTPUT module to produce the output files.
 * Errors and Warning are received via callback function from the compilation
 * process and we write them to stdout.
 *****************************************************************************/

/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include <stdio.h>
#include <stdarg.h>
#include "global.h"
#include "asm.h"
#include "output.h"

/******************************************************************************
 * CONSTANTS & MACROS
 *****************************************************************************/

/* The command line should include at least 2 arguments (the program name and
 * a file to compile. */
#define MIN_NUMBER_OF_ARGUMENTS 2

/******************************************************************************
 * TYPEDEFS
 *****************************************************************************/

/* The MAIN_ERRORS_COUNTER structure is used as the context of the errors 
 * callback function context. We use it to count the errors and warnings during
 * the compilation process */
typedef struct MAIN_ERRORS_COUNTER{
    int nErrors;
    int nWarnings;
} MAIN_ERRORS_COUNTER, *PMAIN_ERRORS_COUNTER;

/******************************************************************************
 * INTERNAL FUNCTIONS (prototypes)
 * -------------------------------
 * See function-level documentation next to the implementation below
 *****************************************************************************/
static void main_ErrorOrWarningCallback(void * pvContext,
                                        const char * pszFileName,
                                        int nLine,
                                        int nColumn,
                                        const char * pszSourceLine,
                                        BOOL bIsError,
                                        const char * pszErrorFormat,
                                        va_list vaArgs);


/******************************************************************************
 * INTERNAL FUNCTIONS
 *****************************************************************************/

/******************************************************************************
 * Name:    main_ErrorOrWarningCallback
 * Purpose: Print error/warning message to the standard output and update
 *          the relevant counter
 * Parameters:
 *          See GLOB_ErrorOrWarningCallback declaration.
  *****************************************************************************/
static void main_ErrorOrWarningCallback(void * pvContext,
                                        const char * pszFileName,
                                        int nLine,
                                        int nColumn,
                                        const char * pszSourceLine,
                                        BOOL bIsError,
                                        const char * pszErrorFormat,
                                        va_list vaArgs) {
    int nIndex = 0;
    
    /* The context is a pointer to a MAIN_ERRORS_COUNTER structure which has
     * the errors and warnings counters. */
    PMAIN_ERRORS_COUNTER ptCounters = (PMAIN_ERRORS_COUNTER)pvContext;
    
    /* Increment the relevant counter */
    if (bIsError) {
        ptCounters->nErrors++;
    } else {
        ptCounters->nWarnings++;
    }
    
    /* Print the location of the error/message. Line & Column are optional.*/
    if (nLine > 0 && nColumn > 0) {
        printf("%s:%d:%d ", pszFileName, nLine, nColumn);
    } else {
        printf("%s ", pszFileName);

    }
    /* Print the message type. */
    printf(bIsError ? "error: " : "warning: ");
    
    /* Print the message itself */
    vprintf (pszErrorFormat, vaArgs);
    
    if (NULL == pszSourceLine || nColumn <= 0) {
        /* Source line isn't provided. */
        printf("\n");
        return;
    }
    /* Print the source line. */
    printf("\n%s\n", pszSourceLine);

    /* Print an arrow below the error. */
    for (nIndex = 0; nIndex < nColumn-1; nIndex++){
        printf(" ");
    }
    printf("^\n");    
}


/******************************************************************************
 * EXTERNAL FUNCTIONS
 *****************************************************************************/

/******************************************************************************
 * Name:    main
 * Purpose: compiling the source file (as passed in the command line parameters)
 *          and produce the output files.
 * Command Line:
 *          asm <file1> <file2> ...
 *          The command line should include at least one file to compile
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS (0) is returned.
 *          GLOB_ERROR_PARSING_FAILED - the compilation failed of one
 *                                      or more files failed
 *          If the program fails, an error code is returned.
  *****************************************************************************/
int main(int nArgc, const char * ppszArgv[]) {
    HASM_FILE hAsm = NULL;
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    MAIN_ERRORS_COUNTER tCounters = {0};
    BOOL bSuccess = TRUE;
    
    /* Check for minimum number of arguments */
    if (nArgc < MIN_NUMBER_OF_ARGUMENTS) {
        printf("USAGE: %s <file1> <file2> ...\n", ppszArgv[0]);
        return GLOB_ERROR_INVALID_PARAMETERS;
    }
    
    /* Start to compile the files */
    for (int nIndex = 1; nIndex < nArgc; nIndex++) {
        printf("Compiling %s...\n", ppszArgv[nIndex]);
        
        /* Reset the counters */
        tCounters.nErrors = 0;
        tCounters.nWarnings = 0;
        
        /* Compile the file */
        eRetValue = ASM_Compile(ppszArgv[nIndex], main_ErrorOrWarningCallback,
                                &tCounters, &hAsm);
        if (GLOB_ERROR_PARSING_FAILED == eRetValue) {
            /* We have one or more compilation errors*/
            bSuccess = FALSE;
            printf("FAILED - %d error(s), %d warning(s)\n",
                    tCounters.nErrors, tCounters.nWarnings);
            continue;
        }
        if (eRetValue) {
            /* Fatal error during the compilation process */
            return eRetValue;
        }
        
        /* write the output files of the compilation */
        eRetValue = OUTPUT_WriteFiles(ppszArgv[nIndex], hAsm);
        if (eRetValue) {
            ASM_Close(hAsm);
            return eRetValue;
        }
        
        /* Close resources of this file */
        ASM_Close(hAsm);
        printf("SUCCESS - 0 error(s), %d warning(s)\n", tCounters.nWarnings);
    }
    
    return bSuccess? GLOB_SUCCESS : GLOB_ERROR_PARSING_FAILED;
}

