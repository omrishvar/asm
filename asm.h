/******************************************************************************
 * File:    asm.h
 * Author:  Doron Shvartztuch
 * The ASM module checks the grammar of the statements and compile the code
 *****************************************************************************/
#ifndef ASM_H
#define ASM_H

/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include "memstream.h"
#include "lex.h"

/* The HASM_FILE represents a handle to a file compiled by the ASM module.
 * Always close the handle with the ASM_Close function. */
typedef struct ASM_FILE ASM_FILE, *HASM_FILE, **PHASM_FILE;

/******************************************************************************
 * Name:    ASM_Compile
 * Purpose: The function opens a source file and compile it
 * Parameters:
 *          szFileName [IN] - the path to the file to compile(w/o the extension)
 *          pfnErrorsCallback [IN] - callback function to use in case
 *                                   of errors/warnings
 *          pvContext [IN] -  context for the callback function
 *          phFile [OUT] - the handle to the compiled file
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          In this case, a handle is returned, so the caller can use
 *          other functions of the module to produce the output files.
 *          You must close the handle with ASM_Close
 *          GLOB_ERROR_PARSING_FAILED - in case we found one or more errors
 *                                      in the source code.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
GLOB_ERROR ASM_Compile(const char * szFileName,
                       GLOB_ERRORCALLBACK pfnErrorsCallback,
                        void * pvContext,
                       PHASM_FILE phFile);

/******************************************************************************
 * Name:    ASM_WriteBinary
 * Purpose: writes the binary of the object file to a memstream
 * Parameters:
 *          hFile [IN] - handle to the compiled file
 *          phStream [OUT] - memory stream with the binary of the object file
 *          nCode [OUT] - size (in words) of the code section
 *          phFile [OUT] - size (in words) of the data section
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          In this case, the caller must free the stream returned in phStream
 *          If the function fails, an error code is returned.
 *****************************************************************************/
GLOB_ERROR ASM_WriteBinary(HASM_FILE hFile,
                           PHMEMSTREAM phStream,
                           int * nCode,
                           int * nData);

/******************************************************************************
 * Name:    ASM_GetExternals
 * Purpose: get a buffer with the content of the externals file
 * Parameters:
 *          hFile [IN] - handle to the compiled file
 *          ppszExternals [OUT] - pointer to the memory buffer with the content
 *                                of the externals file
 *          nLength [OUT] - size (in bytes) of the buffer
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          In this case, the caller can read the buffer until the call
 *          to ASM_Close.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
GLOB_ERROR ASM_GetExternals(HASM_FILE hFile,
                            char ** ppszExternals,
                            int * nLength);

/******************************************************************************
 * Name:    ASM_GetEntries
 * Purpose: get a buffer with the content of the entries file
 * Parameters:
 *          hFile [IN] - handle to the compiled file
 *          ppszExternals [OUT] - pointer to the memory buffer with the content
 *                                of the entries file
 *          nLength [OUT] - size (in bytes) of the buffer
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          In this case, the caller can read the buffer until the call
 *          to ASM_Close.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
GLOB_ERROR ASM_GetEntries(HASM_FILE hFile,
                          char ** ppszExternals,
                          int * nLength);

/******************************************************************************
 * Name:    ASM_Close
 * Purpose: closed the handle of the compiled file
 * Parameters:
 *          hFile [IN] - handle to the compiled file
 *****************************************************************************/
void ASM_Close(HASM_FILE hFile);

#endif /* ASM_H */
