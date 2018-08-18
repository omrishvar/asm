/******************************************************************************
 * File:    global.h
 * Author:  Doron Shvartztuch
 * The GLOBAL module contains some declarations that are relevant to different
 * modules of the project.
 *****************************************************************************/

#ifndef GLOBAL_H
#define GLOBAL_H

/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include <errno.h>
#include "helper.h"

/******************************************************************************
 * CONSTANTS & MACROS
 *****************************************************************************/

/* The address of the first instruction in the object file */
#define CODE_STARTUP_ADDRESS        100

/* File extensions of input/output files */
#define GLOB_FILE_EXTENSION_SOURCE  ".as"
#define GLOB_FILE_EXTENSION_BINARY  ".ob"
#define GLOB_FILE_EXTENSION_ENTRY   ".ent"
#define GLOB_FILE_EXTENSION_EXTERN  ".ext"

/* return GLOB_ERROR_SYS_CALL_FAILED error,
 * with the errno in the lower byte */
#define GLOB_ERROR_SYS_CALL_ERROR()   (GLOB_ERROR_SYS_CALL_FAILED | errno)

/******************************************************************************
 * TYPEDEFS
 *****************************************************************************/

/* Enum of the opcodes in our language.
 * The value is equal to the value of this opcode in the binary code. */
typedef enum GLOB_OPCODE {
    GLOB_OPCODE_MOV = 0,
    GLOB_OPCODE_CMP = 1,
    GLOB_OPCODE_ADD = 2,
    GLOB_OPCODE_SUB = 3,
    GLOB_OPCODE_NOT = 4,
    GLOB_OPCODE_CLR = 5,
    GLOB_OPCODE_LEA = 6,
    GLOB_OPCODE_INC = 7,
    GLOB_OPCODE_DEC = 8,
    GLOB_OPCODE_JMP = 9,
    GLOB_OPCODE_BNE = 10,
    GLOB_OPCODE_RED = 11,
    GLOB_OPCODE_PRN = 12,
    GLOB_OPCODE_JSR = 13,
    GLOB_OPCODE_RTS = 14,
    GLOB_OPCODE_STOP = 15,
} GLOB_OPCODE;

/* Enum of the directives in our language */
typedef enum GLOB_DIRECTIVE {
    GLOB_DIRECTIVE_DATA,
    GLOB_DIRECTIVE_STRING,
    GLOB_DIRECTIVE_ENTRY,
    GLOB_DIRECTIVE_EXTERN,
} GLOB_DIRECTIVE;

/* GLOB_ERROR is the error value we use in this project. GLOB_SUCCESS is the
 * only non-error item, and its value is zero. */
typedef enum GLOB_ERROR {
    GLOB_SUCCESS = 0,
            
    /* a call to a library function failed. the less significant byte may
     * contain the errno for more information. */
    GLOB_ERROR_SYS_CALL_FAILED = 0x100,
            
    /* The function encountered end of line */
    GLOB_ERROR_END_OF_LINE,
            
    /* The function encountered end of file */
    GLOB_ERROR_END_OF_FILE,
            
    /* The function called in invalid state of the module */
    GLOB_ERROR_INVALID_STATE,
            
    /* We found some errors in the parsing/compilation process  */
    GLOB_ERROR_PARSING_FAILED,
            
    /* One or more of the mandatory parameters are invalid */
    GLOB_ERROR_INVALID_PARAMETERS,
        
    /* The function can't handle the this call. */
    GLOB_ERROR_CONTINUE,
            
    /* Item already exist */
    GLOB_ERROR_ALREADY_EXIST,
            
    /* Item not found */
    GLOB_ERROR_NOT_FOUND,
            
    /* Trying to declare the same symbol as both export and extern */
    GLOB_ERROR_EXPORT_AND_EXTERN,
            
    /* Unknown error. usually used as initial value for GLOB_ERROR variables */
    GLOB_ERROR_UNKNOWN,          
} GLOB_ERROR;

/******************************************************************************
 * Name:    GLOB_ERRORCALLBACK
 * Purpose: callback function for errors/warnings occur during the compilation
 * Parameters:
 *          pvContext [IN] - context as passed when starting the compilation
 *          pszFileName [IN] - the name of the file
 *          nLine [IN OPTIONAL] - one-based line index of the error/warning
 *          nColumn [IN OPTIONAL] - one-based column index of the error/warning
 *          pszSourceLine [IN OPTIONAL] - the full source line
 *                                        contains the error/warning
 *          bIsError [IN] - TRUE for error. FALSE for warning
 *          pszErroFormat[IN] - error message (format as printf syntax)
 *          vaArgs [IN] - parameters to include in the message
  *****************************************************************************/
typedef void (*GLOB_ERRORCALLBACK)(void * pvContext,
                                   const char * pszFileName,
                                   int nLine,
                                   int nColumn,
                                   const char * pszSourceLine,
                                   BOOL bIsError,
                                   const char * pszErrorFormat,
                                   va_list vaArgs);

#endif /* GLOBAL_H */
