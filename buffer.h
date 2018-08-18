/******************************************************************************
 * File:    buffer.h
 * Author:  Doron Shvartztuch
 * The BUFFER module provides memory stream functionality.
 * The basic unit of the stream is char.
 *****************************************************************************/

#ifndef BUFFER_H
#define BUFFER_H

/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include "global.h"

/******************************************************************************
 * TYPEDEFS
 *****************************************************************************/

/* The HBUFFER represents a handle to the stream.
 * Always free the stream with the BUFFER_Free function */
typedef struct BUFFER BUFFER, *HBUFFER, **PHBUFFER;

/******************************************************************************
 * FUNCTIONS
 *****************************************************************************/

/******************************************************************************
 * Name:    BUFFER_Create
 * Purpose: Create a new buffer
 * Parameters:
 *          phStream [OUT] - the handle to the created stream
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
GLOB_ERROR BUFFER_Create(PHBUFFER phStream);

GLOB_ERROR BUFFER_AppendPrintf(HBUFFER hStream, const char * pszFormat, ...);

/******************************************************************************
 * Name:    BUFFER_GetStream
 * Purpose: get a pointer to the memory block itself
 * Parameters:
 *          hStream [IN] - the handle to the stream.
 *          ppnStream [OUT] - pointer to the beginning of the memory block
 *          pnStreamLength [OUT] - size (in bytes) of the memory block
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          If the function fails, an error code is returned.
 * Remark:
 *          The caller should not change the memory block.
 *          The pointer may become unavailable after any call to other function
 *          of the module.
 *****************************************************************************/
GLOB_ERROR BUFFER_GetStream(HBUFFER hStream,
                               char ** ppnStream, int * pnStreamLength);

/******************************************************************************
 * Name:    BUFFER_Free
 * Purpose: Free a stream
 * Parameters:
 *          hStream [IN] - the handle to the stream
 *****************************************************************************/
void BUFFER_Free(HBUFFER hStream);

#endif /* BUFFER_H */
