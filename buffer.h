/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   buffer.h
 * Author: boazgildor
 *
 * Created on August 14, 2018, 4:58 PM
 */

#ifndef BUFFER_H
#define BUFFER_H


/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include "global.h"

/******************************************************************************
 * TYPEDEFS
 *****************************************************************************/

/* The HBUFFER represents a handle to a memory stream.
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

/******************************************************************************
 * Name:    BUFFER_AppendString
 * Purpose: Write a string to the stream 
 * Parameters:
 *          hStream [IN] - the handle to the stream
 *          pszStr [IN] - the string to write into the stream
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          If the function fails, an error code is returned.
 * Remark:  The function also write the NULL-terminator to the stream
 *****************************************************************************/
GLOB_ERROR BUFFER_AppendString(HBUFFER hStream, const char * pszStr);


GLOB_ERROR BUFFER_AppendPrintf(HBUFFER hStream, const char * pszFormat, ...);

/******************************************************************************
 * Name:    BUFFER_GetStream
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
