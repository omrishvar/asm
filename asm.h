
#ifndef ASM_H
#define ASM_H

#include "memstream.h"

/* The HASM_FILE represents a handle to a file compiled by the ASM module.
 * Always close the handle with the ASM_Close function. */
typedef struct ASM_FILE ASM_FILE, *HASM_FILE, **PHASM_FILE;

/******************************************************************************
 * Name:    ASM_Compile
 * Purpose: The function opens a source file and compile it
 * Parameters:
 *          szFileName [IN] - the path to the file to compile(w/o the extension)
 *          phFile [OUT] - the handle to the compiled file
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          In this case, a handle is returned, so the caller can use
 *          other functions of the module to produce the output files.
 *          You must close the handle with ASM_Close
 *          If the function fails, an error code is returned.
 * 
 *****************************************************************************/
GLOB_ERROR ASM_Compile(const char * szFileName, PHASM_FILE phFile);

GLOB_ERROR ASM_WriteBinary(HASM_FILE hFile, PHMEMSTREAM phStream, int * nCode, int * nData);
GLOB_ERROR ASM_GetExternals(HASM_FILE hFile, char ** ppszExternals, int * nLength);
GLOB_ERROR ASM_GetEntries(HASM_FILE hFile, char ** ppszExternals, int * nLength);

void ASM_Close(HASM_FILE hFile);

#endif /* ASM_H */
