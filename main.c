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
typedef struct MAIN_ERRORS_COUNTER{
    int nErrors;
    int nWarnings;
} MAIN_ERRORS_COUNTER, *PMAIN_ERRORS_COUNTER;


static void ErrorOrWarningCallback(void * pvContext, const char * pszFileName, int nLine,
        int nColumn, const char * pszSourceLine,
        BOOL bIsError, const char * pszErrorFormat, va_list vaArgs) {
    int nIndex = 0;
    PMAIN_ERRORS_COUNTER ptCounters = (PMAIN_ERRORS_COUNTER)pvContext;
    
    /* Print the error message. */
    printf("%s:%d:%d %s: ", pszFileName, nLine,
            nColumn, bIsError ? "error" : "warning");
    vprintf (pszErrorFormat, vaArgs);
    
    if (NULL != pszSourceLine) {
        /* Print the source line. */
        printf("\n%s\n", pszSourceLine);

        /* Print an arrow below the error. */
        for (nIndex = 0; nIndex < nColumn-1; nIndex++){
            printf(" ");
        }
        printf("^\n");
    }
    if (bIsError) {
        ptCounters->nErrors++;
    } else {
        ptCounters->nWarnings++;
    }
}

int main(int argc, char * argv[]) {
    HASM_FILE hAsm = NULL;
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    MAIN_ERRORS_COUNTER tCounters;
    
    if (argc < MIN_NUMBER_OF_ARGUMENTS) {
        printf("USAGE: %s <file1> <file2> ...\n", argv[0]);
        return 1;
    }
    for (int nIndex = 1; nIndex < argc; nIndex++) {
        printf("Compiling %s...\n", argv[nIndex]);
        tCounters.nErrors = 0;
        tCounters.nWarnings = 0;
        
        eRetValue = ASM_Compile(argv[nIndex], ErrorOrWarningCallback, &tCounters, &hAsm);
        if (GLOB_ERROR_PARSING_FAILED == eRetValue) {
            printf("FAILED - %d error(s), %d warning(s)\n", tCounters.nErrors, tCounters.nWarnings);
            continue;
        }
        if (eRetValue) {
            return eRetValue;
        }
        eRetValue = OUTPUT_WriteFiles(argv[nIndex], hAsm);
        if (eRetValue) {
            ASM_Close(hAsm);
            return eRetValue;
        }
        ASM_Close(hAsm);
        printf("SUCCESS - 0 error(s), %d warning(s)\n", tCounters.nWarnings);
        
    }
}

