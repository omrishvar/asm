/******************************************************************************
 * File:    memstream.h
 * Author:  Doron Shvartztuch
 * The MEMSTREAM module provides memory stream functionality.
 *****************************************************************************/

#ifndef MEMSTREAM_H
#define MEMSTREAM_H

/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include "global.h"

/******************************************************************************
 * TYPEDEFS
 *****************************************************************************/

/* The HMEMSTREAM represents a handle to a memory stream.
 * Always free the stream with the MEMSTREAM_Free function */
typedef struct MEMSTREAM MEMSTREAM, *HMEMSTREAM, **PHMEMSTREAM;

/******************************************************************************
 * FUNCTIONS
 *****************************************************************************/

/******************************************************************************
 * Name:    MEMSTREAM_Create
 * Purpose: Create a new memory stream
 * Parameters:
 *          phStream [OUT] - the handle to the created stream
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
GLOB_ERROR MEMSTREAM_Create(PHMEMSTREAM phStream);

/******************************************************************************
 * Name:    MEMSTREAM_AppendString
 * Purpose: Write a string to the stream 
 * Parameters:
 *          hStream [IN] - the handle to the stream
 *          pszStr [IN] - the string to write into the stream
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          If the function fails, an error code is returned.
 * Remark:  The function also write the NULL-terminator to the stream
 *****************************************************************************/
GLOB_ERROR MEMSTREAM_AppendString(HMEMSTREAM hStream, const char * pszStr);

/******************************************************************************
 * Name:    MEMSTREAM_AppendNumber
 * Purpose: Write a number to the stream 
 * Parameters:
 *          hStream [IN] - the handle to the stream
 *          nNumber [IN] - the number to write into the stream
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          If the function fails, an error code is returned.
 * Remark:  The function assumes the number is in the supported range.
 *          Because we use 14-bit words and with the 2's complement,
 *          the supported range is [-8192...+8191]
 *****************************************************************************/
GLOB_ERROR MEMSTREAM_AppendNumber(HMEMSTREAM hStream, int nNumber);

/******************************************************************************
 * Name:    MEMSTREAM_Concat
 * Purpose: Concat the content of the second stream to the first one
 * Parameters:
 *          hStream1 [IN] - the handle to the first stream.
 *          hStream2 [IN] - the handle to the second stream.
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
GLOB_ERROR MEMSTREAM_Concat(HMEMSTREAM hStream1, HMEMSTREAM hStream2);

/******************************************************************************
 * Name:    MEMSTREAM_GetStream
 * Purpose: get a pointer to the memory block itself
 * Parameters:
 *          hStream [IN] - the handle to the stream.
 *          ppnStream [OUT] - pointer to the beginning of the memory block
 *          pnStreamLength [OUT] - size (in 'int') of the memory block
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          If the function fails, an error code is returned.
 * Remark:
 *          The caller should not change the memory block.
 *          The pointer may become unavailable after any call to other function
 *          of the module.
 *****************************************************************************/
GLOB_ERROR MEMSTREAM_GetStream(HMEMSTREAM hStream,
                               int ** ppnStream, int * pnStreamLength);

/******************************************************************************
 * Name:    MEMSTREAM_Free
 * Purpose: Free a stream
 * Parameters:
 *          hStream [IN] - the handle to the stream
 *****************************************************************************/
void MEMSTREAM_Free(HMEMSTREAM hStream);

#endif /* MEMSTREAM_H */
