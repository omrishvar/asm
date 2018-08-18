/******************************************************************************
 * File:    output.c
 * Author:  Doron Shvartztuch
 * The MOUTPUT module provides functionality to write
 * the output files (object, externals, entries).
 *
 * Implementation:
 * The module gets the textual content of the extern+entries files and write
 * it as is to the files. For the object file - it also gets the content, but
 * have to format it as described in the project requirements.
 *****************************************************************************/

/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "helper.h"
#include "global.h"
#include "asm.h"
#include "memstream.h"
#include "output.h"

/******************************************************************************
 * CONSTANTS & MACROS
 *****************************************************************************/

/* number of bits in a word in the assembly language */
#define BIT_IN_WORD 14

/* encoding of 1 & 0 in the object file. */
#define ENCODE_1 '/'
#define ENCODE_0 '.'

/******************************************************************************
 * INTERNAL FUNCTIONS (prototypes)
 * -------------------------------
 * See function-level documentation next to the implementation below
 *****************************************************************************/
static GLOB_ERROR output_WriteBinary(const char * szFileName, HASM_FILE hFile);
static GLOB_ERROR output_WriteToFile(const char * szFileName,
                                     const char * szFileExt,
                                     char * pszBuffer,
                                     int nBufferLength);
static GLOB_ERROR output_WriteExternals(const char * szFileName,
                                        HASM_FILE hFile);
static GLOB_ERROR output_WriteEntries(const char * szFileName, HASM_FILE hFile);

/******************************************************************************
 * INTERNAL FUNCTIONS
 *****************************************************************************/

/******************************************************************************
 * Name:    output_WriteBinary
 * Purpose: Writes the object file
 * Parameters:
 *          szFileName [IN] - the file name (w/o the extension)
 *          hFile [IN] - handle to the compiled file
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
static GLOB_ERROR output_WriteBinary(const char * szFileName, HASM_FILE hFile) {
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    HMEMSTREAM hStream = NULL;
    int * pnStream = NULL;
    int nStreamLength = 0;
    char * szBinaryFileName = NULL;
    FILE * phBinaryFile = NULL;
    int nCode = 0;
    int nData = 0;
    int nAddress = CODE_STARTUP_ADDRESS;
    char szWord[BIT_IN_WORD+1];
    int nMask = 0;

    TERMINATE_STRING(szWord);
    
    /* Get the binary to write */
    eRetValue = ASM_WriteBinary(hFile, &hStream, &nCode, &nData);
    if (eRetValue) {
        return eRetValue;
    }
    
    /* Get the full name of the object file*/
    szBinaryFileName = HELPER_ConcatStrings(szFileName,
                                            GLOB_FILE_EXTENSION_BINARY);
    if (NULL == szBinaryFileName) {
        eRetValue = GLOB_ERROR_SYS_CALL_ERROR(); 
        MEMSTREAM_Free(hStream);
        return eRetValue;
    }
    
    /* Get the pointer to the buffer of the stream */
    eRetValue =  MEMSTREAM_GetStream(hStream, &pnStream, &nStreamLength);
    if (eRetValue) {
        MEMSTREAM_Free(hStream);
        free(szBinaryFileName);
        return eRetValue;        
    }
    
    /* Open the file in write mode */
    phBinaryFile = fopen(szBinaryFileName, "w");
    if (NULL == phBinaryFile) {
        eRetValue = GLOB_ERROR_SYS_CALL_ERROR();
        MEMSTREAM_Free(hStream);
        free(szBinaryFileName);
        return eRetValue;
    }
    
    /* Write the header line (with the length of the code section
     * and the length of the data section. */
    fprintf(phBinaryFile, "%d %d\n", nCode, nData);
    
    for (int nIndex = 0; nIndex < nStreamLength; nIndex++) {
        /* We start with nMask to get the most significant bit */
        nMask = 0x2000; /* 10 0000 0000 0000 */
        for (int nBit=0; nBit<BIT_IN_WORD; nBit++) {
            szWord[nBit] = pnStream[nIndex] & nMask ? ENCODE_1 : ENCODE_0;
            /* Shift right to get the next bit */
            nMask = nMask >> 1;
        }
        
        /* Write the word to the object file */
        fprintf(phBinaryFile, "%04d\t%s\n", nAddress, szWord);
        nAddress++;
    }
    MEMSTREAM_Free(hStream);
    free(szBinaryFileName);
    fclose(phBinaryFile);
    return GLOB_SUCCESS;
}

/******************************************************************************
 * Name:    output_WriteToFile
 * Purpose: Writes a buffer to the file
 * Parameters:
 *          szFileName [IN] - the file name (w/o the extension)
 *          szFileExt [IN] - file extention
 *          pszBuffer [IN] - buffer to write to the file
 *          nBufferLength [IN] - buffer length
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
static GLOB_ERROR output_WriteToFile(const char * szFileName,
                                     const char * szFileExt,
                                     char * pszBuffer,
                                     int nBufferLength) {
    char * szFullFileName = NULL;
    FILE * phFile = NULL;
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;

    /* Get the full file name */
    szFullFileName = HELPER_ConcatStrings(szFileName, szFileExt);
    if (NULL == szFullFileName) {
        eRetValue = GLOB_ERROR_SYS_CALL_ERROR(); 
        return eRetValue;
    }
    
    /* Open the file for write */
    phFile = fopen(szFullFileName, "w");
    if (NULL == phFile) {
        eRetValue = GLOB_ERROR_SYS_CALL_ERROR();
        free(szFullFileName);
        return eRetValue;
    }
    
    /* Write the buffer */
    if (nBufferLength != fwrite(pszBuffer, 1, nBufferLength, phFile)) {
        eRetValue = GLOB_ERROR_SYS_CALL_ERROR();
        fclose(phFile);
        free(szFullFileName);
        return eRetValue;        
    }
    
    /* Close the file */
    fclose(phFile);
    free(szFullFileName);
    return GLOB_SUCCESS; 
}

/******************************************************************************
 * Name:    output_WriteExternals
 * Purpose: Writes the externals file
 * Parameters:
 *          szFileName [IN] - the file name (w/o the extension)
 *          hFile [IN] - handle to the compiled file
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          If the function fails, an error code is returned.
 * Remark:  We don't create empty files.
 *****************************************************************************/
static GLOB_ERROR output_WriteExternals(const char * szFileName,
                                        HASM_FILE hFile) {
    char * pszBuffer = NULL;
    int  nBufferLength = 0;
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    
    /* Get the content of the file */
    eRetValue = ASM_GetExternals(hFile, &pszBuffer, &nBufferLength);
    if (eRetValue) {
        return eRetValue;
    }
    
    /* If there is content, write it to the file */
    if (nBufferLength > 0) {
        return output_WriteToFile(szFileName, GLOB_FILE_EXTENSION_EXTERN,
                pszBuffer, nBufferLength);
    }
    return GLOB_SUCCESS;
}

/******************************************************************************
 * Name:    output_WriteExternals
 * Purpose: Writes the externals file
 * Parameters:
 *          szFileName [IN] - the file name (w/o the extension)
 *          hFile [IN] - handle to the compiled file
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          If the function fails, an error code is returned.
 * Remark:  We don't create empty files.
 *****************************************************************************/
static GLOB_ERROR output_WriteEntries(const char * szFileName, HASM_FILE hFile){
    char * pszBuffer = NULL;
    int  nBufferLength = 0;
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    eRetValue = ASM_GetEntries(hFile, &pszBuffer, &nBufferLength);
    if (eRetValue) {
        return eRetValue;
    }
    
    if (nBufferLength > 0) {
        return output_WriteToFile(szFileName, GLOB_FILE_EXTENSION_ENTRY,
                pszBuffer, nBufferLength);
    }
    return GLOB_SUCCESS;
}

/******************************************************************************
 * EXTERNAL FUNCTIONS
 * ------------------
 * See function-level documentation in the header file
 *****************************************************************************/

/******************************************************************************
 * Name:    OUTPUT_WriteFiles
 *****************************************************************************/
GLOB_ERROR OUTPUT_WriteFiles(const char * szFileName, HASM_FILE hFile) {
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    
    /* Check parameters */
    if (NULL == hFile) { 
        return GLOB_ERROR_INVALID_PARAMETERS;
    }
    
    /* Write the object file */
    eRetValue = output_WriteBinary(szFileName, hFile);
    if (eRetValue) {
        return eRetValue;
    }

    /* Write the externals file */
    eRetValue = output_WriteExternals(szFileName, hFile);
    if (eRetValue) {
        return eRetValue;
    }
    
    /* Write the entries file */
    eRetValue = output_WriteEntries(szFileName, hFile);
    if (eRetValue) {
        return eRetValue;
    }

    return GLOB_SUCCESS;
}
