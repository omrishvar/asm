/******************************************************************************
 * File:    buffer.c
 * Author:  Doron Shvartztuch
 * The MEMSTREAM module provides memory stream functionality.
 * 
 * Implementation:
 * The memory stream is based on dynamic allocated array of integers.
 * We use realloc to expand the array when there is not enough space 
 *****************************************************************************/

/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "helper.h"
#include "buffer.h"

/******************************************************************************
 * CONSTANTS & MACROS
 *****************************************************************************/

/* The default size of a stream */
#define BUFFER_DEFAULT_SIZE 4
#define BUFFER_EXPAND_FACTOR 2

/******************************************************************************
 * TYPEDEFS
 *****************************************************************************/

struct BUFFER {
    char * pnStream;
    int nAllocated;
    int nUsed;
};

static GLOB_ERROR memstream_EnsureSpace(HBUFFER hStream, int nSpace) {
    int nNeedToAllocate = 0;
    char * pnNewStream = NULL;
    
    /* Calc the size we need */
    nNeedToAllocate = nSpace - hStream->nAllocated + hStream->nUsed;
    if (nNeedToAllocate <= 0) {
        /* No need to allocate */
        return GLOB_SUCCESS;
    }
    
    nNeedToAllocate = MAX(nNeedToAllocate,
                        hStream->nAllocated * (BUFFER_EXPAND_FACTOR-1));
    pnNewStream = realloc(hStream->pnStream, hStream->nAllocated + nNeedToAllocate);
    if (NULL == pnNewStream) {
        return GLOB_ERROR_SYS_CALL_ERROR();
    }
    hStream->nAllocated += nNeedToAllocate;
    hStream->pnStream = pnNewStream;
    return GLOB_SUCCESS;
}
/******************************************************************************
 * EXTERNAL FUNCTIONS
 * ------------------
 * See function-level documentation in the header file
 *****************************************************************************/

/******************************************************************************
 * Name:    BUFFER_Create
 *****************************************************************************/
GLOB_ERROR BUFFER_Create(PHBUFFER phStream) {
    HBUFFER hStream = NULL;
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    
    /* Check parameters. */
    if (NULL == phStream) {
        return GLOB_ERROR_INVALID_PARAMETERS;
    }
    
    /* Allocate the handle */
    hStream = malloc(sizeof(*hStream));
    if (NULL == hStream) {
        return GLOB_ERROR_SYS_CALL_ERROR();
    }
    hStream->nAllocated = BUFFER_DEFAULT_SIZE;
    hStream->nUsed = 0;
    
    /* Allocate the default stream */
    hStream->pnStream = malloc(hStream->nAllocated * sizeof(int));
    if (NULL == hStream) {
        eRetValue = GLOB_ERROR_SYS_CALL_ERROR();
        free(hStream);
        return eRetValue;
    }
    
    /* Set out parameter */
    *phStream = hStream;
    return GLOB_SUCCESS;
}

/******************************************************************************
 * Name:    BUFFER_AppendString
 *****************************************************************************/
GLOB_ERROR BUFFER_AppendString(HBUFFER hStream, const char * pszStr) {
    int nLength = 0;
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    if (NULL == hStream || NULL == pszStr) {
        return GLOB_ERROR_INVALID_PARAMETERS;
    }
    
    nLength = strlen(pszStr);
    eRetValue = memstream_EnsureSpace(hStream, nLength);
    if (eRetValue) {
        return eRetValue;
    }
    
    memcpy(hStream->pnStream+hStream->nUsed, pszStr, nLength);
    hStream->nUsed += nLength;
    return GLOB_SUCCESS;
}

/******************************************************************************
 * Name:    BUFFER_AppendPrintf
 *****************************************************************************/
GLOB_ERROR BUFFER_AppendPrintf(HBUFFER hStream, const char * pszFormat, ...) {        
    char szFormatted[80];
    va_list vaArgs;
    va_start (vaArgs, pszFormat);
    vsnprintf (szFormatted, sizeof(szFormatted), pszFormat, vaArgs);
    va_end (vaArgs);
    
    return BUFFER_AppendString(hStream, szFormatted);
}
    
/******************************************************************************
 * Name:    BUFFER_GetStream
 *****************************************************************************/
GLOB_ERROR BUFFER_GetStream(HBUFFER hStream,
                               char ** ppnStream, int * pnStreamLength) {
    if (NULL == hStream) {
        return GLOB_ERROR_INVALID_PARAMETERS;
    }
    *ppnStream = hStream->pnStream;
    *pnStreamLength = hStream->nUsed;
    return GLOB_SUCCESS;
}

/******************************************************************************
 * Name:    BUFFER_Free 
 *****************************************************************************/
void BUFFER_Free(HBUFFER hStream) {
    if (NULL != hStream) {
        free(hStream->pnStream);
        free(hStream);
    }
}
