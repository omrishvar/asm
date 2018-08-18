/******************************************************************************
 * File:    output.h
 * Author:  Doron Shvartztuch
 * The MOUTPUT module provides functionality to write
 * the output files (object, externals, entries).
 *****************************************************************************/

#ifndef OUTPUT_H
#define OUTPUT_H

/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include "global.h"
#include "asm.h"

/******************************************************************************
 * FUNCTIONS
 *****************************************************************************/

/******************************************************************************
 * Name:    OUTPUT_WriteFiles
 * Purpose: write the output files of successfully compiled file
 * Parameters:
 *          szFileName [IN] - the file name (w/o extension)
 *          hFile [IN] - handle to the compiled file 
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
GLOB_ERROR OUTPUT_WriteFiles(const char * szFileName, HASM_FILE hFile);

#endif /* OUTPUT_H */
