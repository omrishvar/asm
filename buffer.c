/******************************************************************************
 * File:    buffer.c
 * Author:  Doron Shvartztuch
 * The BUFFER module provides memory stream functionality.
 * The basic unit of the stream is char.
 * 
 * Implementation:
 * The memory stream is based on dynamic allocated array of chars.
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
#define BUFFER_DEFAULT_SIZE 32
#define BUFFER_EXPAND_FACTOR 2

/* maximum size (in chars) of strings we can write to the buffer */
#define MAX_STRING_SIZE 80

/******************************************************************************
 * TYPEDEFS
 *****************************************************************************/

/* BUFFER  is the struct behind the the HBUFFER.
 * It keeps some information about the stream */
struct BUFFER {
    /* Pointer to the dynamic allocated stream */
    char * pnStream;
    
    /* Allocated size (bytes) */
    int nAllocated;
    
    /* Used (bytes) */
    int nUsed;
};

/******************************************************************************
 * INTERNAL FUNCTIONS (prototypes)
 * -------------------------------
 * See function-level documentation next to the implementation below
 *****************************************************************************/
static GLOB_ERROR buffer_EnsureSpace(HBUFFER hStream, int nSpace);

/******************************************************************************
 * INTERNAL FUNCTIONS
 *****************************************************************************/

/******************************************************************************
 * Name:    buffer_EnsureSpace
 * Purpose: Ensure there is enough free space in the stream
 * Parameters:
 *          hStream [IN] - the stream
 *          nSpace [IN] - size of free space (in bytes) we need in the stream
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
static GLOB_ERROR buffer_EnsureSpace(HBUFFER hStream, int nSpace) {
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
    
    /* Reallocate */
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
 * Name:    BUFFER_AppendPrintf
 *****************************************************************************/
GLOB_ERROR BUFFER_AppendPrintf(HBUFFER hStream, const char * pszFormat, ...) {        
    char szFormatted[MAX_STRING_SIZE];
    int nLength = 0;
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;

    
    if (NULL == hStream || NULL == pszFormat) {
        return GLOB_ERROR_INVALID_PARAMETERS;
    }
    
    /* format the parameters into the format string */
    va_list vaArgs;
    va_start (vaArgs, pszFormat);
    nLength = vsnprintf (szFormatted, sizeof(szFormatted), pszFormat, vaArgs);
    va_end (vaArgs);
    if (nLength < 0) {
        return GLOB_ERROR_SYS_CALL_ERROR();
    }
    
    /* Ensure enough space */
    eRetValue = buffer_EnsureSpace(hStream, nLength);
    if (eRetValue) {
        return eRetValue;
    }
    
    /* Copy the string (without the '\0') */
    memcpy(hStream->pnStream+hStream->nUsed, szFormatted, nLength);
    hStream->nUsed += nLength;
    return GLOB_SUCCESS;
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
