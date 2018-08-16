/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   output.c
 * Author: boazgildor
 * 
 * Created on August 11, 2018, 6:28 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include "helper.h"
#include "global.h"
#include "asm.h"
#include "memstream.h"
#include "output.h"

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
    char szWord[15];
    int nMask = 0;

    szWord[14] = '\0';
    /* Get the binary to write */
    eRetValue = ASM_WriteBinary(hFile, &hStream, &nCode, &nData);
    if (eRetValue) {
        return eRetValue;
    }
    
    szBinaryFileName = HELPER_ConcatStrings(szFileName, GLOB_FILE_EXTENSION_BINARY);
    if (NULL == szBinaryFileName) {
        eRetValue = GLOB_ERROR_SYS_CALL_ERROR(); 
        MEMSTREAM_Free(hStream);
        return eRetValue;
    }
    eRetValue =  MEMSTREAM_GetStream(hStream, &pnStream, &nStreamLength);
    if (eRetValue) {
        MEMSTREAM_Free(hStream);
        free(szBinaryFileName);
        return eRetValue;        
    }
    phBinaryFile = fopen(szBinaryFileName, "w");
    if (NULL == phBinaryFile) {
        eRetValue = GLOB_ERROR_SYS_CALL_ERROR();
        MEMSTREAM_Free(hStream);
        free(szBinaryFileName);
        return eRetValue;
    }
    
    fprintf(phBinaryFile, "%d %d\n", nCode, nData);
    for (int nIndex = 0; nIndex < nStreamLength; nIndex++) {
        
        nMask = 0x2000;
        for (int nBit=0; nBit<14; nBit++) {
            szWord[nBit] = pnStream[nIndex] & nMask ? '/' : '.';
            nMask = nMask >> 1;
        }
        fprintf(phBinaryFile, "%04d\t%s\n", nAddress, szWord);
        nAddress++;
    }
    MEMSTREAM_Free(hStream);
    free(szBinaryFileName);
    fclose(phBinaryFile);
    return GLOB_SUCCESS;
 
    
}

static GLOB_ERROR output_WriteToFile(const char * szFileName, const char * szFileExt,
        char * pszBuffer, int nBufferLength) {
    char * szFullFileName = NULL;
    FILE * phFile = NULL;
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;

    szFullFileName = HELPER_ConcatStrings(szFileName, szFileExt);
    if (NULL == szFullFileName) {
        eRetValue = GLOB_ERROR_SYS_CALL_ERROR(); 
        return eRetValue;
    }
    
    phFile = fopen(szFullFileName, "w");
    if (NULL == phFile) {
        eRetValue = GLOB_ERROR_SYS_CALL_ERROR();
        free(szFullFileName);
        return eRetValue;
    }
    
    if (nBufferLength != fwrite(pszBuffer, 1, nBufferLength, phFile)) {
        eRetValue = GLOB_ERROR_SYS_CALL_ERROR();
        fclose(phFile);
        free(szFullFileName);
        return eRetValue;        
    }
    fclose(phFile);
    free(szFullFileName);
    return GLOB_SUCCESS; 
}

static GLOB_ERROR output_WriteExternals(const char * szFileName, HASM_FILE hFile) {
    char * pszBuffer = NULL;
    int  nBufferLength = 0;
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    eRetValue = ASM_GetExternals(hFile, &pszBuffer, &nBufferLength);
    if (eRetValue) {
        return eRetValue;
    }
    
    if (nBufferLength > 0) {
        return output_WriteToFile(szFileName, GLOB_FILE_EXTENSION_EXTERN,
                pszBuffer, nBufferLength);
    }
    return GLOB_SUCCESS;
}

static GLOB_ERROR output_WriteEntries(const char * szFileName, HASM_FILE hFile) {
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

GLOB_ERROR OUTPUT_WriteFiles(const char * szFileName, HASM_FILE hFile) {
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    
    if (NULL == hFile) { 
        return GLOB_ERROR_INVALID_PARAMETERS;
    }
    
    eRetValue = output_WriteBinary(szFileName, hFile);
    if (eRetValue) {
        return eRetValue;
    }

    eRetValue = output_WriteExternals(szFileName, hFile);
    if (eRetValue) {
        return eRetValue;
    }
    
    eRetValue = output_WriteEntries(szFileName, hFile);
    if (eRetValue) {
        return eRetValue;
    }

    return GLOB_SUCCESS;
}
