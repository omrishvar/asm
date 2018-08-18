/******************************************************************************
 * File:    memstream.c
 * Author:  Doron Shvartztuch
 * The MEMSTREAM module provides memory stream functionality.
 * The basic unit of the stream is int.
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
#include "memstream.h"

/******************************************************************************
 * CONSTANTS & MACROS
 *****************************************************************************/

/* The default size(in int) of a stream */
#define MEMSTREAM_DEFAULT_SIZE 4
#define MEMSTREAM_EXPAND_FACTOR 2

/******************************************************************************
 * TYPEDEFS
 *****************************************************************************/

/* MEMSTREAM is the struct behind the the HMEMSTREAM.
 * It keeps some information about the stream */
struct MEMSTREAM {
    int * pnStream; /* Pointer to the dynamic allocated stream */
    int nAllocated; /* Allocated words (int) */
    int nUsed; /* Used words (int) */
};


/******************************************************************************
 * INTERNAL FUNCTIONS (prototypes)
 * -------------------------------
 * See function-level documentation next to the implementation below
 *****************************************************************************/

static GLOB_ERROR memstream_EnsureSpace(HMEMSTREAM hStream, int nSpace);

/******************************************************************************
 * INTERNAL FUNCTIONS
 *****************************************************************************/

/******************************************************************************
 * Name:    memstream_EnsureSpace
 * Purpose: Ensure there is enough free space in the stream
 * Parameters:
 *          hStream [IN] - the stream
 *          nSpace [IN] - size of free space (in words) we need in the stream
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
static GLOB_ERROR memstream_EnsureSpace(HMEMSTREAM hStream, int nSpace) {
    int nNeedToAllocate = 0;
    int * pnNewStream = NULL;
    
    /* Calc the size we need */
    nNeedToAllocate = nSpace - hStream->nAllocated + hStream->nUsed;
    if (nNeedToAllocate <= 0) {
        /* No need to allocate */
        return GLOB_SUCCESS;
    }
    nNeedToAllocate = MAX(nNeedToAllocate,
                        hStream->nAllocated * (MEMSTREAM_EXPAND_FACTOR-1));
    
    /* Reallocate */
    pnNewStream = realloc(hStream->pnStream,
                    (hStream->nAllocated + nNeedToAllocate) * sizeof(int));
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
 * Name:    MEMSTREAM_Create
 *****************************************************************************/
GLOB_ERROR MEMSTREAM_Create(PHMEMSTREAM phStream) {
    HMEMSTREAM hStream = NULL;
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
    hStream->nAllocated = MEMSTREAM_DEFAULT_SIZE;
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
 * Name:    MEMSTREAM_AppendString
 *****************************************************************************/
GLOB_ERROR MEMSTREAM_AppendString(HMEMSTREAM hStream, const char * pszStr) {
    int nIndex = 0;
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    
    /* Check parameters */
    if (NULL == hStream || NULL == pszStr) {
        return GLOB_ERROR_INVALID_PARAMETERS;
    }
    
    /* Note the '<='. we include the null terminator */
    for (nIndex = 0; nIndex <= strlen(pszStr) ; nIndex++) {
        int nCurrentCharAsNumber = 0;
        nCurrentCharAsNumber = pszStr[nIndex];
        /* We write each char from the string as a "number" so it takes
         * the size of a word */
        eRetValue = MEMSTREAM_AppendNumber(hStream, nCurrentCharAsNumber);
        if (eRetValue) {
            return eRetValue;
        }
    }
    return GLOB_SUCCESS;
}

/******************************************************************************
 * Name:    MEMSTREAM_AppendNumber
 *****************************************************************************/
GLOB_ERROR MEMSTREAM_AppendNumber(HMEMSTREAM hStream, int nNumber) {
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    
    /* Check parameters */
    if (NULL == hStream) {
        return GLOB_ERROR_INVALID_PARAMETERS;
    }
    
    /* We need space for one word*/
    eRetValue = memstream_EnsureSpace(hStream, 1);
    if (eRetValue) {
        return eRetValue;
    }
    
    /* Write to the stream */
    hStream->pnStream[hStream->nUsed] = nNumber;
    hStream->nUsed++;
    return GLOB_SUCCESS;
}

/******************************************************************************
 * Name:    MEMSTREAM_Concat
 *****************************************************************************/
GLOB_ERROR MEMSTREAM_Concat(HMEMSTREAM hStream1, HMEMSTREAM hStream2) {
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    if (NULL == hStream1 || NULL == hStream2) {
        return GLOB_ERROR_INVALID_PARAMETERS;
    }
    eRetValue = memstream_EnsureSpace(hStream1, hStream2->nUsed);
    if (eRetValue) {
        return eRetValue;
    }
    memcpy(hStream1->pnStream + hStream1->nUsed,
           hStream2->pnStream,
           hStream2->nUsed * sizeof(int));
    hStream1->nUsed += hStream2->nUsed;
    return GLOB_SUCCESS;
}

/******************************************************************************
 * Name:    MEMSTREAM_GetStream
 *****************************************************************************/
GLOB_ERROR MEMSTREAM_GetStream(HMEMSTREAM hStream,
                               int ** ppnStream, int * pnStreamLength) {
    if (NULL == hStream) {
        return GLOB_ERROR_INVALID_PARAMETERS;
    }
    *ppnStream = hStream->pnStream;
    *pnStreamLength = hStream->nUsed;
    return GLOB_SUCCESS;
}

/******************************************************************************
 * Name:    MEMSTREAM_Free 
 *****************************************************************************/
void MEMSTREAM_Free(HMEMSTREAM hStream) {
    if (NULL != hStream) {
        free(hStream->pnStream);
        free(hStream);
    }
}
