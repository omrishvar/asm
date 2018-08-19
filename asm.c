/******************************************************************************
 * File:    asm.c
 * Author:  Doron Shvartztuch
 * The ASM module checks the grammar of the statements and compile the code
 * 
 * Implementation:
 * The ASM modules uses LEX to read the source file and parse it into tokens.
 * It goes over the tokens, checks the grammar and create the binary code.
 * In the first phase, we get the tokens and build binary code(including data)
 * except the operands. Labels are inserted into the symbols table.
 * In the second phase, we already have all symbols in the table, so we can
 * build the binary code of the operands.
 *****************************************************************************/

/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "helper.h"
#include "global.h"
#include "lex.h"
#include "linestr.h"
#include "symtable.h"
#include "buffer.h"
#include "memstream.h"
#include "asm.h"

/******************************************************************************
 * CONSTANTS & MACROS
 *****************************************************************************/

/* although there are maximum 2 operands for each opcode, some on the
 * single operand opcode can use the parameters operand method. In this method
 * we actually have 3 operands (the label and the 2 parameters). */
#define ASM_MAX_OPERANDS 3

/* The next macros uses the g_szAllowedOperands to retrieve information
 * about opcodes in the language. See documentation next
 * to g_szAllowedOperands definition. */

/* True if this opcode expect source operand (= 2-operand opcode) */
#define ASM_HAS_SOURCE_OPERAND(opcode) (g_znAllowedOperands[opcode] & 0xF0)

/* Get the allowed methods for the source operand of this opcode */
#define ASM_GET_ALLOWED_SOURCE_OPERAND(opcode) ((g_znAllowedOperands[opcode] \
                                                 & 0xF0) >> 4)

/* True if this opcode expect destination operand (= 1 or 2-operand opcode) */
#define ASM_HAS_DESTINATION_OPERAND(opcode) (g_znAllowedOperands[opcode] & 0x0F)

/* Get the allowed methods for the destination operand of this opcode */
#define ASM_GET_ALLOWED_DESTINATION_OPERAND(opcode) \
                                        ASM_HAS_DESTINATION_OPERAND(opcode)

/* Check whether the specified method is allowed. the parameter should be
 * the result of either  ASM_GET_ALLOWED_SOURCE_OPERAND or 
 * ASM_GET_ALLOWED_DESTINATION_OPERAND.*/
#define ASM_IS_IMMEDIATE_OPERAND_ALLOWED(operand)   ((operand) & 0x1)
#define ASM_IS_DIRECT_OPERAND_ALLOWED(operand)      ((operand) & 0x2)
#define ASM_IS_PARAMETER_OPERAND_ALLOWED(operand)   ((operand) & 0x4)
#define ASM_IS_REGISTER_OPERAND_ALLOWED(operand)    ((operand) & 0x8)

/* The allowed operands method for each of the parameters in the
 * "jump with parameters" operand method. See documentation next to
 * g_znAllowedOperands for more information. */
#define ASM_ALLOWED_OPERANDS_AS_PARAM               0xB

/* Create the first binary word of the statement (opcode).
 * the ARE are always absolute. We shift left the parameters to their location*/
#define ASM_COMBINE_FIRST_WORD(eParam1, eParam2, eOpcode, eSource, eDest) \
    ((eParam1)<<12 | (eParam2)<<10 | (eOpcode)<<6 | (eSource)<<4          \
    | (eDest)<<2 | (ASM_ARE_ABSOLUTE))

#define ASM_COMBINE_IMMEDIATE_WORD(value) \
    (((value) << 2) | ASM_ARE_ABSOLUTE) 

#define ASM_COMBINE_DIRECT_WORD(value) \
    (((value) << 2) | ASM_ARE_RELOCATABLE) 

#define ASM_COMBINE_EXTERNAL_WORD ASM_ARE_EXTERNAL

#define ASM_COMBINE_REGISTER_WORD(nSource, nDest) \
    (((nSource) << 8) | ((nDest) << 2) | ASM_ARE_ABSOLUTE) 

/******************************************************************************
 * TYPEDEFS
 *****************************************************************************/

/* enum of the operand methods exist in the language */
typedef enum ASM_OPERAND_METHOD {
    ASM_OPERAND_METHOD_IMMEDIATE = 0,
    ASM_OPERAND_METHOD_DIRECT = 1,
    ASM_OPERAND_METHOD_PARAMETERS = 2,
    ASM_OPERAND_METHOD_REGISTER = 3,
} ASM_OPERAND_METHOD;

/* enum of values of the ARE bits in the first word */
typedef enum ASM_ARE {
    ASM_ARE_ABSOLUTE = 0,
    ASM_ARE_EXTERNAL = 1,
    ASM_ARE_RELOCATABLE = 2,
} ASM_ARE;

/* Linked list. Each element represent a line (statement) in the source file */
typedef struct ASM_LINE {
    
    /* Stream that contain the binary code of this line*/
    HMEMSTREAM hStream;
    
    /* Array of the tokens of the operands */
    PLEX_TOKEN aptOperands[ASM_MAX_OPERANDS];
    
    /* Number of used elements in aptOperands*/
    int nOperandsLength;
    
    /* The instruction/data counter. the address of the first word*/
    int nCounter;
    
    /* The length (in words) of this statement */
    int nLength;
    
    /* True if the binary should be part of the data section.
     * False for code section */
    BOOL bIsData;
    
    /* The operand methods of the two parameters (in PARAMETERS operand only) */
    ASM_OPERAND_METHOD eParam1;
    ASM_OPERAND_METHOD eParam2;
    
    /* The operand method of the source and destination param*/
    ASM_OPERAND_METHOD eSourceParam;
    ASM_OPERAND_METHOD eDestParam;
    
    /* pointer to the next line */
    struct ASM_LINE * ptNext;
} ASM_LINE, *PASM_LINE;

struct ASM_FILE {
    /* Handle to the LEX "instance" that parse the file. */
    HLEX_FILE hLex;

    /* Callback function and a context for errors/warnings reporting */
    GLOB_ERRORCALLBACK pfnErrorsCallback;
    void * pvErrorsCallbackContext;

    /* Handle to the symbols table */
    HSYMTABLE_TABLE hSymTable;
    
    /* Buffers that contain the content of the externals and entries files */
    HBUFFER hExternalsStream;
    HBUFFER hEntriesStream;

    /* Flags to set when the relevant buffer has content */
    BOOL bHaveExternals;
    BOOL bHaveEntries;
   
    /* Current location in the code and data counters (IC/DC) */
    int nCodeCounter; /* initialized to CODE_STARTUP_ADDRESS */
    int nDataCounter; /* initialized to 0 */
    
    /* Flag to set in case we have compilation errors */
    BOOL bHasErrors;
    
    /* Pointers to the first element and the last element in the
     * one-way linked list of the lines */
    PASM_LINE ptFirstLine;
    PASM_LINE ptLastLine;
};

/******************************************************************************
 * INTERNAL FUNCTIONS (prototypes)
 * -------------------------------
 * See function-level documentation next to the implementation below
 *****************************************************************************/
static void asm_ReportError(HASM_FILE hFile,
                            BOOL bIsError,
                            PLEX_TOKEN ptToken,
                            const char * pszErrorFormat,
                            ...);
static GLOB_ERROR asm_FirstPhaseCompileString(HASM_FILE hFile,PASM_LINE ptLine);
static GLOB_ERROR asm_FirstPhaseCompileData(HASM_FILE hFile, PASM_LINE ptLine);
static GLOB_ERROR asm_FirstPhaseCompileExtern(HASM_FILE hFile,PASM_LINE ptLine);
static GLOB_ERROR asm_FirstPhaseCompileEntry(HASM_FILE hFile, PASM_LINE ptLine);
static GLOB_ERROR asm_FirstPhaseReadParameterOperand(HASM_FILE hFile,
                                                     PASM_LINE ptLine,
                                                     BOOL * bParametersRead);
static GLOB_ERROR asm_FirstPhaseReadOperand(HASM_FILE hFile,
                                            ASM_OPERAND_METHOD * peMethod,
                                            int nAllowedOperandType,
                                            PASM_LINE ptLine,
                                            BOOL bAllowSpacesBeforeOperand);
static GLOB_ERROR asm_FirstPhaseCompileSourceOperand(HASM_FILE hFile,
                                                     GLOB_OPCODE eOpcode,
                                                     PASM_LINE ptLine);
static GLOB_ERROR asm_FirstPhaseCompileDestinationOperand(HASM_FILE hFile,
                                                          GLOB_OPCODE eOpcode,
                                                          PASM_LINE ptLine);
static GLOB_ERROR asm_FirstPhaseCompileOpcode(HASM_FILE hFile,
                                              GLOB_OPCODE eOpcode,
                                              PASM_LINE ptLine);
static GLOB_ERROR asm_FirstPhaseCompileLineContent(HASM_FILE hFile,
                                                   PLEX_TOKEN ptToken,
                                                   PASM_LINE ptLine);
static GLOB_ERROR asm_FirstPhaseCompileNonEmptyLine(HASM_FILE hFile,
                                                    PLEX_TOKEN ptToken);
static GLOB_ERROR asm_FirstPhaseCompileLine(HASM_FILE hFile);
static GLOB_ERROR asm_FirstPhase(HASM_FILE hFile);
static GLOB_ERROR asm_SecondPhaseCompileLine(HASM_FILE hFile, PASM_LINE ptLine);
static GLOB_ERROR asm_SecondPhase(HASM_FILE hFile);
static GLOB_ERROR asm_SymTableForEachCallback(const char * pszName,
                                              int nAddress, 
                                              BOOL bIsMarkedForExport,
                                              void * pContext);
static GLOB_ERROR asm_PrepareEntries(HASM_FILE hFile);

/******************************************************************************
 * CONSTANTS
 *****************************************************************************/

/* A table that described which operanding method is allowed for each opcode.
 * For each opcode, there is one value in the array. The index is equal to the
 * code of the opcode. For example, the allowed operand methods for the 'not'
 * operatod is described in index 4.
 * For each opcode we have a 8-bit number. The 4 most significant bits describe
 * the source operand and the 4 less significant bits describe the destination
 * operand. If all 4 bits are 0, the opcode doesn't expect this operand.
 * Otherwise, the 4 bits describe the allowed methods for this operand. 
 * The less significant bit is 1 if immediate operand is allowed, etc.
 * The comment next to the table can help the reader to check the meaning of
 * the values in the array. For example, for the 'not' opcode' the value
 * is 0x0A. That means that there is no source operand and only register and
 * direct operand methods are allowed for the destination operand.
 * The methods are described in the comment below as:
 * R - Register
 * P - Parameter
 * D - Direct
 * I - Immediate */
const int g_znAllowedOperands[] = 
        /* |--Opcode--|----SOURCE OPERAND-----|--DESTINATION OPERAND--| */
        /* |          |  R  |  P  |  D  |  I  |  R  |  P  |  D  |  I  | */
        /* |----------|-----|-----|-----|-----|-----|-----|-----|-----| */
{ 0xBA, /* |   mov    |  1  |  0  |  1  |  1  |  1  |  0  |  1  |  0  | */
  0xBB, /* |   cmp    |  1  |  0  |  1  |  1  |  1  |  0  |  1  |  1  | */
  0xBA, /* |   add    |  1  |  0  |  1  |  1  |  1  |  0  |  1  |  0  | */
  0xBA, /* |   sub    |  1  |  0  |  1  |  1  |  1  |  0  |  1  |  0  | */
  0x0A, /* |   not    |  0  |  0  |  0  |  0  |  1  |  0  |  1  |  0  | */
  0x0A, /* |   clr    |  0  |  0  |  0  |  0  |  1  |  0  |  1  |  0  | */
  0x2A, /* |   lea    |  0  |  0  |  1  |  0  |  1  |  0  |  1  |  0  | */
  0x0A, /* |   inc    |  0  |  0  |  0  |  0  |  1  |  0  |  1  |  0  | */
  0x0A, /* |   dec    |  0  |  0  |  0  |  0  |  1  |  0  |  1  |  0  | */
  0x0E, /* |   jmp    |  0  |  0  |  0  |  0  |  1  |  1  |  1  |  0  | */
  0x0E, /* |   bne    |  0  |  0  |  0  |  0  |  1  |  1  |  1  |  0  | */
  0x0A, /* |   red    |  0  |  0  |  0  |  0  |  1  |  0  |  1  |  0  | */
  0x0B, /* |   prn    |  0  |  0  |  0  |  0  |  1  |  0  |  1  |  1  | */
  0x0E, /* |   jsr    |  0  |  0  |  0  |  0  |  1  |  1  |  1  |  0  | */
  0x00, /* |   rts    |  0  |  0  |  0  |  0  |  0  |  0  |  0  |  0  | */
  0x00  /* |   stop   |  0  |  0  |  0  |  0  |  0  |  0  |  0  |  0  | */
};      /* |----------|-----|-----|-----|-----|-----|-----|-----|-----| */

/******************************************************************************
 * INTERNAL FUNCTIONS
 *****************************************************************************/

/******************************************************************************
 * Name:    asm_ReportError
 * Purpose: Report parsing error message
 * Parameters:
 *          hFile [IN] - handle to the current file
 *          bIsError [IN] - TRUE for error. FALSE for warning
 *          ptToken [IN OPTIONAL] - the token we faild to parse
 *          pszErroFormat[IN] - error message (format as printf syntax)
 *          ... [IN] - parameters to include in the message
 *****************************************************************************/
static void asm_ReportError(HASM_FILE hFile, BOOL bIsError, PLEX_TOKEN ptToken,
                            const char * pszErrorFormat, ...) {
    va_list vaArgs;
    
    if (bIsError) {
        hFile->bHasErrors = TRUE;
    }
    va_start (vaArgs, pszErrorFormat);
    /* Call the callback function with the relevant arguments*/
    hFile->pfnErrorsCallback(hFile->pvErrorsCallbackContext,
        NULL == ptToken ? "" : LINESTR_GetFullFileName(ptToken->ptLine->hFile),
        NULL == ptToken ? 0 : ptToken->ptLine->nLineNumber,
        NULL == ptToken ? 0 : ptToken->nColumn+1,
        NULL == ptToken ? NULL : ptToken->ptLine->szLine,
        bIsError, pszErrorFormat, vaArgs);
    va_end (vaArgs);
}

/******************************************************************************
 * Name:    asm_FirstPhaseCompileString
 * Purpose: parse the content of the .string statement
 * Parameters:
 *          hFile [IN] - handle to the current file
 *          ptLine [IN] - the structure of the line to fill
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          GLOB_ERROR_PARSING_FAILED if the syntax is incorrect.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
static GLOB_ERROR asm_FirstPhaseCompileString(HASM_FILE hFile,PASM_LINE ptLine){
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    PLEX_TOKEN ptStringToken = NULL;
    
    /* Read the string token */
    eRetValue = LEX_ReadNextToken(hFile->hLex, &ptStringToken);
    if (eRetValue) {
        return eRetValue;
    }
    
    /* Check the token type */
    if (ptStringToken->eKind != LEX_TOKEN_KIND_STRING) {
        asm_ReportError(hFile, TRUE, ptStringToken, "String is expected");
        LEX_FreeToken(ptStringToken);
        return GLOB_ERROR_PARSING_FAILED;
    }
    
    /* The binary of .string statement is just the string (including '\0') */
    eRetValue = MEMSTREAM_AppendString(ptLine->hStream,
                                       ptStringToken->uValue.szStr);
    ptLine->nLength = strlen(ptStringToken->uValue.szStr)+1;
    ptLine->bIsData = TRUE;
    LEX_FreeToken(ptStringToken);
    return eRetValue;
}

/******************************************************************************
 * Name:    asm_FirstPhaseCompileData
 * Purpose: parse the content of the .data statement
 * Parameters:
 *          hFile [IN] - handle to the current file
 *          ptLine [IN] - the structure of the line to fill
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          GLOB_ERROR_PARSING_FAILED if the syntax is incorrect.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
static GLOB_ERROR asm_FirstPhaseCompileData(HASM_FILE hFile, PASM_LINE ptLine) {
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    PLEX_TOKEN ptToken = NULL;
    
    ptLine->bIsData = TRUE;
    for (;;) {
        
        /* Read the next number*/
        eRetValue = LEX_ReadNextToken(hFile->hLex, &ptToken);
        if (eRetValue) {
            return eRetValue;
        }

        /* Check the token type */
        if (ptToken->eKind != LEX_TOKEN_KIND_NUMBER) {
            asm_ReportError(hFile, TRUE, ptToken, "Number (data) is expected");
            LEX_FreeToken(ptToken);
            return GLOB_ERROR_PARSING_FAILED;
        }
        
        /* Write it to the binary */
        eRetValue = MEMSTREAM_AppendNumber(ptLine->hStream,
                                           ptToken->uValue.nNumber);
        ptLine->nLength++;
        LEX_FreeToken(ptToken);
        
        /* Check if have a comma (to continue the data) */
        eRetValue = LEX_ReadNextToken(hFile->hLex, &ptToken);
        if (eRetValue) {
            return eRetValue;
        }
        if (LEX_TOKEN_KIND_END_OF_LINE == ptToken->eKind) {
            break;
        }
        if (LEX_TOKEN_KIND_SPECIAL != ptToken->eKind
                || ',' != ptToken->uValue.cChar) {
            asm_ReportError(hFile, TRUE, ptToken,
                    "comma or end of line expected");
            LEX_FreeToken(ptToken);
            return GLOB_ERROR_PARSING_FAILED;
        }
    }
    LEX_FreeToken(ptToken);    
    return GLOB_SUCCESS;
}

/******************************************************************************
 * Name:    asm_FirstPhaseCompileExtern
 * Purpose: parse the content of the .extern statement
 * Parameters:
 *          hFile [IN] - handle to the current file
 *          ptLine [IN] - the structure of the line to fill
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          GLOB_ERROR_PARSING_FAILED if the syntax is incorrect.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
static GLOB_ERROR asm_FirstPhaseCompileExtern(HASM_FILE hFile,PASM_LINE ptLine){
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    PLEX_TOKEN ptToken = NULL;

    for (;;) {
        /* Read the label to define as extern */
        eRetValue = LEX_ReadNextToken(hFile->hLex, &ptToken);
        if (eRetValue) {
            return eRetValue;
        }

        /* Check the type */
        if (ptToken->eKind != LEX_TOKEN_KIND_WORD) {
            asm_ReportError(hFile, TRUE, ptToken, "identifier is expected");
            LEX_FreeToken(ptToken);
            return GLOB_ERROR_PARSING_FAILED;
        }
        
        /* Add to the symbols table*/
        eRetValue = SYMTABLE_Insert(hFile->hSymTable, ptToken->uValue.szStr,
                SYMTABLE_SYMTYPE_CODE, 0, TRUE);
        if (GLOB_ERROR_ALREADY_EXIST == eRetValue) {
            asm_ReportError(hFile, TRUE, ptToken, "label already exist");
            LEX_FreeToken(ptToken);
   
            return GLOB_ERROR_PARSING_FAILED;
        }
        if (GLOB_ERROR_EXPORT_AND_EXTERN == eRetValue) {
            asm_ReportError(hFile, TRUE, ptToken,
                    "label already defined as entry");
            LEX_FreeToken(ptToken);
   
            return GLOB_ERROR_PARSING_FAILED;
        }
        if (eRetValue) {
            LEX_FreeToken(ptToken);
            return eRetValue;
        }
        LEX_FreeToken(ptToken);

        /* Check if we have comma (more labels)*/
        eRetValue = LEX_ReadNextToken(hFile->hLex, &ptToken);
        if (eRetValue) {
            return eRetValue;
        }
        if (LEX_TOKEN_KIND_END_OF_LINE == ptToken->eKind) {
            break;
        }
        if (LEX_TOKEN_KIND_SPECIAL != ptToken->eKind
                || ',' != ptToken->uValue.cChar) {
            asm_ReportError(hFile,TRUE,ptToken,"comma or end of line expected");
            LEX_FreeToken(ptToken);
            return GLOB_ERROR_PARSING_FAILED;
        }
    }
    LEX_FreeToken(ptToken);
    hFile->bHaveExternals = TRUE;
    return GLOB_SUCCESS;
}

/******************************************************************************
 * Name:    asm_FirstPhaseCompileEntry
 * Purpose: parse the content of the .entry statement
 * Parameters:
 *          hFile [IN] - handle to the current file
 *          ptLine [IN] - the structure of the line to fill
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          GLOB_ERROR_PARSING_FAILED if the syntax is incorrect.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
static GLOB_ERROR asm_FirstPhaseCompileEntry(HASM_FILE hFile, PASM_LINE ptLine){
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    PLEX_TOKEN ptToken = NULL;

    for (;;) {
        /* Read the label to define as extern */
        eRetValue = LEX_ReadNextToken(hFile->hLex, &ptToken);
        if (eRetValue) {
            return eRetValue;
        }

        /* Check the type */
        if (ptToken->eKind != LEX_TOKEN_KIND_WORD) {
            asm_ReportError(hFile, TRUE, ptToken, "identifier is expected");
            LEX_FreeToken(ptToken);
            return GLOB_ERROR_PARSING_FAILED;
        }
        
        /* Mark this label for export in the symbols table*/
        eRetValue = SYMTABLE_MarkForExport(hFile->hSymTable,
                                           ptToken->uValue.szStr);
        if (GLOB_ERROR_EXPORT_AND_EXTERN == eRetValue) {
            /* Same label can't be defined both extern and entry */
            asm_ReportError(hFile, TRUE, ptToken,
                    "label already defined as extern");
            LEX_FreeToken(ptToken);
            return GLOB_ERROR_PARSING_FAILED;
        }
        if (GLOB_ERROR_ALREADY_EXIST == eRetValue) {
            /* Same label can't be defined both extern and entry */
            asm_ReportError(hFile, TRUE, ptToken,
                    "label already defined as entry");
            LEX_FreeToken(ptToken);
            return GLOB_ERROR_PARSING_FAILED;
        }
        LEX_FreeToken(ptToken);

        /* Check if we have comma (more labels)*/
        eRetValue = LEX_ReadNextToken(hFile->hLex, &ptToken);
        if (eRetValue) {
            return eRetValue;
        }
        if (LEX_TOKEN_KIND_END_OF_LINE == ptToken->eKind) {
            break;
        }
        if (LEX_TOKEN_KIND_SPECIAL != ptToken->eKind
                || ',' != ptToken->uValue.cChar) {
            asm_ReportError(hFile, TRUE, ptToken,
                            "comma or end of line expected");
            LEX_FreeToken(ptToken);
            return GLOB_ERROR_PARSING_FAILED;
        }
    }
    hFile->bHaveEntries = TRUE;
    LEX_FreeToken(ptToken);
    return GLOB_SUCCESS;
}

/******************************************************************************
 * Name:    asm_FirstPhaseReadParameterOperand
 * Purpose: try to parse the content of "operand with parameters"
 * Parameters:
 *          hFile [IN] - handle to the current file
 *          ptLine [IN] - the structure of the line to fill
 *          bParametersRead [OUT] - whether we read the parameters
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          GLOB_ERROR_PARSING_FAILED if the syntax is incorrect.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
static GLOB_ERROR asm_FirstPhaseReadParameterOperand(HASM_FILE hFile,
                                                     PASM_LINE ptLine, 
                                                     BOOL * bParametersRead) {
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    PLEX_TOKEN ptToken = NULL;
    
    /* Try to read the '(' */
    eRetValue = LEX_ReadNextToken(hFile->hLex, &ptToken);
    if (eRetValue) {
        return eRetValue;
    }
    if (LEX_TOKEN_KIND_END_OF_LINE == ptToken->eKind) {
        /* There is no parameters. we will stay with simple label */
        *bParametersRead = FALSE;
        LEX_FreeToken(ptToken);
        return GLOB_SUCCESS;
    }
    
    /* Check this is the leading '(' and there are no spaces*/
    if (LEX_TOKEN_KIND_SPECIAL != ptToken->eKind
            || '(' != ptToken->uValue.cChar
            || (!(ptToken->eFlags & LEX_TOKEN_FLAGS_NO_SPACE_FROM_PREV_TOKEN))){
        asm_ReportError(hFile, TRUE, ptToken, 
                        "an end of line ot  '(' (withtout space) expected");
        LEX_FreeToken(ptToken);
        return GLOB_ERROR_PARSING_FAILED;
    }
    LEX_FreeToken(ptToken);
    
    /* Read the first parameter.
     * For this parameter we use the Param1 field */
    eRetValue = asm_FirstPhaseReadOperand(hFile, &ptLine->eParam1,
                                          ASM_ALLOWED_OPERANDS_AS_PARAM,
                                          ptLine, FALSE);
    if (eRetValue) {
        return eRetValue;
    }
    
    /* Read the comma (also here, space are not allowed) */
    eRetValue = LEX_ReadNextToken(hFile->hLex, &ptToken);
    if (eRetValue) {
        return eRetValue;
    }
    if (LEX_TOKEN_KIND_SPECIAL != ptToken->eKind || ',' != ptToken->uValue.cChar
            || !(ptToken->eFlags & LEX_TOKEN_FLAGS_NO_SPACE_FROM_PREV_TOKEN)) {
        asm_ReportError(hFile, TRUE, ptToken,
                        "a ',' (without spaces) is expected");
        LEX_FreeToken(ptToken);
        return GLOB_ERROR_PARSING_FAILED;
    }
    LEX_FreeToken(ptToken);

    /* Read the second parameter.
     * For this parameter we use the Param2 field */
    eRetValue = asm_FirstPhaseReadOperand(hFile, &ptLine->eParam2,
                                          ASM_ALLOWED_OPERANDS_AS_PARAM,
                                          ptLine, FALSE);
    if (eRetValue) {
        return eRetValue;
    }
    
    /* Read the ')'. don't allow spaces */
    eRetValue = LEX_ReadNextToken(hFile->hLex, &ptToken);
    if (eRetValue) {
        return eRetValue;
    }
    if (LEX_TOKEN_KIND_SPECIAL != ptToken->eKind
            || ')' != ptToken->uValue.cChar) {
        asm_ReportError(hFile, TRUE, ptToken, "a ')' is expected");
        LEX_FreeToken(ptToken);
        return GLOB_ERROR_PARSING_FAILED;
    }
    LEX_FreeToken(ptToken);

    return GLOB_SUCCESS;
}
/******************************************************************************
 * Name:    asm_FirstPhaseReadOperand
 * Purpose: read an operand
 * Parameters:
 *          hFile [IN] - handle to the current file
 *          pdMethod [OUT] - the actual operanding method of the operand
 *          nAllowedOperandType [IN] - 4-bit number that describe the allowed
 *                                     mtehods. see documentation above.
 *          ptLine [IN] - the structure of the line to fill
 *          bAllowSpacesBeforeOperand [IN] - TRUE if we allow white chars
 *                                           between the previous token to the
 *                                           operand.
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          GLOB_ERROR_PARSING_FAILED if the syntax is incorrect.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
static GLOB_ERROR asm_FirstPhaseReadOperand(HASM_FILE hFile,
                                            ASM_OPERAND_METHOD * peMethod,
                                            int nAllowedOperandType,
                                            PASM_LINE ptLine,
                                            BOOL bAllowSpacesBeforeOperand) {
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    PLEX_TOKEN ptToken = NULL;
    ASM_OPERAND_METHOD eMethod;
    BOOL bParametersRead = FALSE;

    /* Read the operand */
    eRetValue = LEX_ReadNextToken(hFile->hLex, &ptToken);
    if (eRetValue) {
        return eRetValue;
    }
    if (LEX_TOKEN_KIND_END_OF_LINE == ptToken->eKind) {
        /* Operand is mandatory. */
        asm_ReportError(hFile, TRUE, ptToken, "an operand is expected");
        LEX_FreeToken(ptToken);
        return GLOB_ERROR_PARSING_FAILED;
    }
    /* Check for white spaces limitation */
    if (!bAllowSpacesBeforeOperand
            && !(ptToken->eFlags & LEX_TOKEN_FLAGS_NO_SPACE_FROM_PREV_TOKEN)) {
        asm_ReportError(hFile, TRUE, ptToken, "spaces are not allowed here");
        LEX_FreeToken(ptToken);
        return GLOB_ERROR_PARSING_FAILED;
    }
    
    /* Now we get the operand method (by the token kind) and check whether it
     * is allowed in the current context. */
    if (ASM_IS_IMMEDIATE_OPERAND_ALLOWED(nAllowedOperandType)
            && LEX_TOKEN_KIND_IMMED_NUMBER == ptToken->eKind) {
        eMethod = ASM_OPERAND_METHOD_IMMEDIATE;
    } else if (ASM_IS_DIRECT_OPERAND_ALLOWED(nAllowedOperandType)
            && LEX_TOKEN_KIND_WORD == ptToken->eKind) {
        eMethod = ASM_OPERAND_METHOD_DIRECT; /* Including parameters method */    
    } else if (ASM_IS_REGISTER_OPERAND_ALLOWED(nAllowedOperandType)
            && LEX_TOKEN_KIND_REGISTER == ptToken->eKind) {
        eMethod = ASM_OPERAND_METHOD_REGISTER;
    } else {
        asm_ReportError(hFile, TRUE, ptToken, "Unsupported operand");
        LEX_FreeToken(ptToken);
        return GLOB_ERROR_PARSING_FAILED;
    }
    
    /* Keep the operand in the array for later encoding */
    ptLine->aptOperands[ptLine->nOperandsLength] = ptToken;
    ptLine->nOperandsLength++;
    
    /* maybe there are parameters for this operand.*/
    if (ASM_OPERAND_METHOD_DIRECT == eMethod
            && ASM_IS_PARAMETER_OPERAND_ALLOWED(nAllowedOperandType)) {
        /* We can assume that we are parsing the destination operand.
         * Only 1-operand opcodes allow this kind of operanding method. */
        eRetValue = asm_FirstPhaseReadParameterOperand(hFile, ptLine,
                                                       &bParametersRead);
        if (eRetValue) {
            return eRetValue;
        }
        /* If we found parameters, change the method */
        if (bParametersRead) {
            eMethod = ASM_OPERAND_METHOD_PARAMETERS;
        }
    }
    
    *peMethod = eMethod;
    return GLOB_SUCCESS;
}

/******************************************************************************
 * Name:    asm_FirstPhaseCompileSourceOperand
 * Purpose: Read the source operand (if this opcode should have)
 * Parameters:
 *          hFile [IN] - handle to the current file
 *          eOpcode [IN] - the opcode of the current statement
 *          ptLine [IN] - the structure of the line to fill
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          GLOB_ERROR_PARSING_FAILED if the syntax is incorrect.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
static GLOB_ERROR asm_FirstPhaseCompileSourceOperand(HASM_FILE hFile,
                                                     GLOB_OPCODE eOpcode,
                                                     PASM_LINE ptLine) {
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    PLEX_TOKEN ptCommaToken = NULL;
    
    /* Check of this opcode has source operand */
    if (!ASM_HAS_SOURCE_OPERAND(eOpcode)) {
        /* No source operand */
        return GLOB_SUCCESS;
    }
    
    /* Read the operand*/
    eRetValue = asm_FirstPhaseReadOperand(hFile, &ptLine->eSourceParam,
                    ASM_GET_ALLOWED_SOURCE_OPERAND(eOpcode),ptLine, TRUE);
    if (eRetValue) {
        return eRetValue;
    }
    
    /* Read the comma (we always expect destination operand) */
    eRetValue = LEX_ReadNextToken(hFile->hLex, &ptCommaToken);
    if (eRetValue) {
        return eRetValue;
    }
    if (LEX_TOKEN_KIND_SPECIAL != ptCommaToken->eKind
            || ',' != ptCommaToken->uValue.cChar) {
        asm_ReportError(hFile, TRUE, ptCommaToken, "a comma is expected");
        LEX_FreeToken(ptCommaToken);
        return GLOB_ERROR_PARSING_FAILED;
    }
    
    LEX_FreeToken(ptCommaToken);
    return GLOB_SUCCESS;
}

/******************************************************************************
 * Name:    asm_FirstPhaseCompileDestinationOperand
 * Purpose: Read the destination operand (if this opcode should have)
 * Parameters:
 *          hFile [IN] - handle to the current file
 *          eOpcode [IN] - the opcode of the current statement
 *          ptLine [IN] - the structure of the line to fill
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          GLOB_ERROR_PARSING_FAILED if the syntax is incorrect.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
static GLOB_ERROR asm_FirstPhaseCompileDestinationOperand(HASM_FILE hFile,
                                                          GLOB_OPCODE eOpcode,
                                                          PASM_LINE ptLine) {
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    
    /* Check of this opcode has source operand */
    if (!ASM_HAS_DESTINATION_OPERAND(eOpcode)) {
        /* No source operand */
        return GLOB_SUCCESS;
    }
    
    /* Read the operand */
    eRetValue = asm_FirstPhaseReadOperand(hFile, &ptLine->eDestParam,
                    ASM_GET_ALLOWED_DESTINATION_OPERAND(eOpcode),ptLine, TRUE);
    if (eRetValue) {
        return eRetValue;
    }
    
    return GLOB_SUCCESS;
}

/******************************************************************************
 * Name:    asm_FirstPhaseCompileOpcode
 * Purpose: compile statement that starts with an opcode 
 * Parameters:
 *          hFile [IN] - handle to the current file
 *          eOpcode [IN] - the opcode of the current statement
 *          ptLine [IN] - the structure of the line to fill
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          GLOB_ERROR_PARSING_FAILED if the syntax is incorrect.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
static GLOB_ERROR asm_FirstPhaseCompileOpcode(HASM_FILE hFile,
                                              GLOB_OPCODE eOpcode,
                                              PASM_LINE ptLine) {
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;

    /* Handle Source operand */
    eRetValue = asm_FirstPhaseCompileSourceOperand(hFile, eOpcode, ptLine);
    if (eRetValue) {
        return eRetValue;
    }
    
    /* Handle Destination operand */
    eRetValue = asm_FirstPhaseCompileDestinationOperand(hFile, eOpcode, ptLine);
    if (eRetValue) {
        return eRetValue;
    }
    
    /* Create the first word */
    eRetValue = MEMSTREAM_AppendNumber(ptLine->hStream,
        ASM_COMBINE_FIRST_WORD(ptLine->eParam1, ptLine->eParam2,
                                eOpcode,
                                ptLine->eSourceParam, ptLine->eDestParam));
    if (eRetValue) {
        return eRetValue;
    }
    
    /* Calculate the length:
     * Basically the length is the leading word + 1 word for each operand.
     * In case there are two register operands they will share the same word.
     * We know from the syntax that if there are 2 registers operands, they are
     * at the last 2 cells of the array */
    ptLine->nLength = 1 + ptLine->nOperandsLength;
    if (ptLine->nOperandsLength >= 2
            && LEX_TOKEN_KIND_REGISTER ==
                        ptLine->aptOperands[ptLine->nOperandsLength-1]->eKind
            && LEX_TOKEN_KIND_REGISTER ==
                        ptLine->aptOperands[ptLine->nOperandsLength-2]->eKind) {
        ptLine->nLength--;
    }
    ptLine->bIsData = FALSE;
    return GLOB_SUCCESS;
}

/******************************************************************************
 * Name:    asm_HandleLabelDefinition
 * Purpose: insert new labels to the symbols table
 * Parameters:
 *          hFile [IN] - handle to the current file
 *          pptToken [IN OUT] - the current token
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          If *pptToken was label definition, it will be freed and the next
 *          token will be read.
 *          GLOB_ERROR_PARSING_FAILED if the syntax is incorrect.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
static GLOB_ERROR asm_HandleLabelDefinition(HASM_FILE hFile,
                                            PLEX_TOKEN * pptToken) {
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    SYMTABLE_SYMTYPE eLabelType = SYMTABLE_SYMTYPE_DATA;
    int nLabelAddress = 0;

    PLEX_TOKEN ptLabelToken = NULL;

    if (LEX_TOKEN_KIND_LABEL != (*pptToken)->eKind) {
        /* No label definition in this statement */
        return GLOB_SUCCESS;
    }
    
    /* Keep the label token */
    ptLabelToken = *pptToken;
    *pptToken = NULL;
    
    /* Read the next token */
    eRetValue = LEX_ReadNextToken(hFile->hLex, pptToken);
    if (eRetValue) {
        return eRetValue;
    }
    
    /* We must have opcode/directive after label definition */
    if (LEX_TOKEN_KIND_DIRECTIVE != (*pptToken)->eKind
            && LEX_TOKEN_KIND_OPCODE != (*pptToken)->eKind) {
        asm_ReportError(hFile, TRUE, *pptToken,
                "an opcode or directive is expected");
        LEX_FreeToken(ptLabelToken);
        return GLOB_ERROR_PARSING_FAILED;        
    }
    
    if (LEX_TOKEN_KIND_OPCODE == (*pptToken)->eKind) {
        /* Labels before opcodes should point to the code section*/
        eLabelType = SYMTABLE_SYMTYPE_CODE;
        nLabelAddress = hFile->nCodeCounter;
    } else {
        if (GLOB_DIRECTIVE_EXTERN == (*pptToken)->uValue.eDiretive
                || GLOB_DIRECTIVE_ENTRY == (*pptToken)->uValue.eDiretive) {
            /* Label definition in .extern or .entry.
             * report a warning and ignore */
            asm_ReportError(hFile, FALSE, ptLabelToken,
                    "Label is defined in .extern or .entry statement");
            LEX_FreeToken(ptLabelToken);
            return GLOB_SUCCESS;
        }
        /* Labels before .string/.data point to the data section*/
        eLabelType = SYMTABLE_SYMTYPE_DATA;
        nLabelAddress = hFile->nDataCounter;
    }
    
    /* Insert the label */
    eRetValue = SYMTABLE_Insert(hFile->hSymTable, ptLabelToken->uValue.szStr,
                     eLabelType, nLabelAddress, FALSE);
    if (GLOB_ERROR_ALREADY_EXIST == eRetValue) {
        asm_ReportError(hFile, TRUE, ptLabelToken,"Duplicate label definition");
        LEX_FreeToken(ptLabelToken);
        /* return with SUCCESS to continue parsing the line */
        return GLOB_SUCCESS;
    }
    if (eRetValue) {
        LEX_FreeToken(ptLabelToken);
        return eRetValue;
    }
    LEX_FreeToken(ptLabelToken);
    return GLOB_SUCCESS;
}

/******************************************************************************
 * Name:    asm_FirstPhaseCompileLineContent
 * Purpose: parse and compile a non-empty line (directive/opcode)
 * Parameters:
 *          hFile [IN] - handle to the current file
 *          ptToken [IN] - the token of the opcode/directive
 *          ptLine [IN] - the structure of the line to fill
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          GLOB_ERROR_PARSING_FAILED if the syntax is incorrect.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
static GLOB_ERROR asm_FirstPhaseCompileLineContent(HASM_FILE hFile,
                                                   PLEX_TOKEN ptToken,
                                                   PASM_LINE ptLine) {
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    
    /* Check for different directives and call the relevant function */
    if (LEX_TOKEN_KIND_DIRECTIVE == ptToken->eKind) {
        switch (ptToken->uValue.eDiretive) {
            case GLOB_DIRECTIVE_STRING:
                eRetValue = asm_FirstPhaseCompileString(hFile, ptLine);
                break;
            case GLOB_DIRECTIVE_DATA:
                eRetValue = asm_FirstPhaseCompileData(hFile, ptLine);
                break;
            case GLOB_DIRECTIVE_ENTRY:
                eRetValue = asm_FirstPhaseCompileEntry(hFile, ptLine);
                break;
            case GLOB_DIRECTIVE_EXTERN:
                eRetValue = asm_FirstPhaseCompileExtern(hFile, ptLine);
                break;                                
        }       
    } else if (LEX_TOKEN_KIND_OPCODE == ptToken->eKind) {
        eRetValue = asm_FirstPhaseCompileOpcode(hFile,
                ptToken->uValue.eOpcode, ptLine);
    } else {
        asm_ReportError(hFile, TRUE, ptToken,
                "an opcode or directive is expected");
        eRetValue = GLOB_ERROR_PARSING_FAILED;
    }
    LEX_FreeToken(ptToken);
    return eRetValue;
}

/******************************************************************************
 * Name:    asm_FirstPhaseCompileNonEmptyLine
 * Purpose: parse and compile a non-empty line
 * Parameters:
 *          hFile [IN] - handle to the current file
 *          ptToken [IN] - the first token in this line
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          GLOB_ERROR_PARSING_FAILED if the syntax is incorrect.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
static GLOB_ERROR asm_FirstPhaseCompileNonEmptyLine(HASM_FILE hFile,
                                                    PLEX_TOKEN ptToken) {
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    PASM_LINE ptLine = NULL;
    
    /* Ignore remark lines */
    if (LEX_TOKEN_KIND_REMARK == ptToken->eKind) {
        LEX_FreeToken(ptToken);
        return GLOB_SUCCESS;
    }
    
    /* Allocate the line structure */
    ptLine = malloc(sizeof(*ptLine));
    if (NULL == ptLine) {
        eRetValue = GLOB_ERROR_SYS_CALL_ERROR();
        LEX_FreeToken(ptToken);
        return eRetValue;
    }
    /* Init fields */
    ptLine->hStream = NULL;
    for (int nIndex = 0; nIndex < ARRAY_ELEMENTS(ptLine->aptOperands);nIndex++){
        ptLine->aptOperands[nIndex] = NULL;
    }
    ptLine->nOperandsLength = 0;
    ptLine->nLength = 0;
    ptLine->bIsData = FALSE;;
    ptLine->ptNext = NULL;
    ptLine->eParam1 = ASM_OPERAND_METHOD_IMMEDIATE;
    ptLine->eParam2 = ASM_OPERAND_METHOD_IMMEDIATE;
    ptLine->eSourceParam = ASM_OPERAND_METHOD_IMMEDIATE;
    ptLine->eDestParam = ASM_OPERAND_METHOD_IMMEDIATE;
    
    eRetValue = MEMSTREAM_Create(&ptLine->hStream);
    if (eRetValue) {
        LEX_FreeToken(ptToken);
        free(ptLine);
        return eRetValue;
    }
    
    /* Check if a label is defined at the beginning of this line */
    eRetValue = asm_HandleLabelDefinition(hFile, &ptToken);
    if (eRetValue) {
        MEMSTREAM_Free(ptLine->hStream);
        LEX_FreeToken(ptToken);
        free(ptLine);
        return eRetValue;
    }
    
    /* Now ptToken should be the opcode or directive */
    eRetValue = asm_FirstPhaseCompileLineContent(hFile, ptToken, ptLine);
    if (eRetValue) {
        MEMSTREAM_Free(ptLine->hStream);
        free(ptLine);
        return eRetValue;
    }

    /* Add the line at the tail of the list */
    if (NULL == hFile->ptFirstLine) {
        hFile->ptFirstLine = ptLine;
    } else {
        hFile->ptLastLine->ptNext = ptLine;
    }
    hFile->ptLastLine = ptLine;

    /* Update the data/code counter */
    if (ptLine->bIsData) {
        ptLine->nCounter = hFile->nDataCounter;
        hFile->nDataCounter += ptLine->nLength;
    } else {
        ptLine->nCounter = hFile->nCodeCounter;
        hFile->nCodeCounter += ptLine->nLength;
    }
    
    /* expect end of line */
    eRetValue = LEX_ReadNextToken(hFile->hLex, &ptToken);
    if (eRetValue) {
        return eRetValue;
    }
    if (LEX_TOKEN_KIND_END_OF_LINE != ptToken->eKind) {
        asm_ReportError(hFile, TRUE, ptToken, "end of line expected");     
    }
    LEX_FreeToken(ptToken);

    return GLOB_SUCCESS;
}

/******************************************************************************
 * Name:    asm_FirstPhaseCompileLine
 * Purpose: parse and compile the next line
 * Parameters:
 *          hFile [IN] - handle to the current file
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          GLOB_ERROR_PARSING_FAILED if the syntax is incorrect.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
static GLOB_ERROR asm_FirstPhaseCompileLine(HASM_FILE hFile) {
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    PLEX_TOKEN ptToken = NULL;
    
    /* Read the first token of the line.*/
    eRetValue = LEX_ReadNextToken(hFile->hLex, &ptToken);
    switch (eRetValue) {
        case GLOB_ERROR_PARSING_FAILED:
            hFile->bHasErrors = TRUE;
        case GLOB_SUCCESS:
            break;
        case GLOB_ERROR_END_OF_FILE:
            return GLOB_ERROR_END_OF_FILE;
        default:
            return eRetValue;
    }
    if (GLOB_ERROR_PARSING_FAILED == eRetValue) {
        eRetValue = GLOB_SUCCESS;
    } else if (LEX_TOKEN_KIND_END_OF_LINE == ptToken->eKind) {
        /* line without tokens */
        LEX_FreeToken(ptToken);
    } else {
        /* compile the line */
        eRetValue = asm_FirstPhaseCompileNonEmptyLine(hFile, ptToken);
        if (GLOB_ERROR_PARSING_FAILED == eRetValue) {
            /* change the return value to continue compiling the file */
            eRetValue = GLOB_SUCCESS;
        }
    }
    
    /* Now we can move to the next line in the file. */
    LEX_MoveToNextLine(hFile->hLex);
    return eRetValue;
}

/******************************************************************************
 * Name:    asm_FirstPhase
 * Purpose: parse and compile the file
 * Parameters:
 *          hFile [IN] - handle to the current file
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
static GLOB_ERROR asm_FirstPhase(HASM_FILE hFile) {
    GLOB_ERROR eRetValue = GLOB_SUCCESS;
    
    /* Compile until we get END_OF_LINE (or error) */
    while (!eRetValue) {
        eRetValue = asm_FirstPhaseCompileLine(hFile);
    }
    return GLOB_ERROR_END_OF_FILE == eRetValue ? GLOB_SUCCESS : eRetValue;
}

/******************************************************************************
 * Name:    asm_SecondPhaseCompileLine
 * Purpose: complete the binary code of the line
 * Parameters:
 *          hFile [IN] - handle to the current file
 *          ptLine [IN] - the structure of the line to compile
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          GLOB_ERROR_PARSING_FAILED if the syntax is incorrect.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
static GLOB_ERROR asm_SecondPhaseCompileLine(HASM_FILE hFile, PASM_LINE ptLine){
    int nOperand = 0;
    int nLabelAddress = 0;
    BOOL bIsExtern = FALSE;
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    
    /* Compile each operand to its binary code*/
    for (int nIndex = 0; nIndex < ptLine->nOperandsLength; nIndex++) {
        switch (ptLine->aptOperands[nIndex]->eKind) {
            case LEX_TOKEN_KIND_IMMED_NUMBER:
                /* For immediate number, write the value*/
                nOperand = ASM_COMBINE_IMMEDIATE_WORD(
                                ptLine->aptOperands[nIndex]->uValue.nNumber);
                break;
            case LEX_TOKEN_KIND_WORD:
                /* For labels, check the value from the symbols table*/
                eRetValue = SYMTABLE_GetSymbolInfo(hFile->hSymTable,
                        ptLine->aptOperands[nIndex]->uValue.szStr,
                        &nLabelAddress, &bIsExtern);
                if (GLOB_ERROR_NOT_FOUND == eRetValue) {
                    asm_ReportError(hFile, TRUE, NULL,
                            "Missing label %s",
                            ptLine->aptOperands[nIndex]->uValue.szStr);
                    return GLOB_SUCCESS;
                }
                if (bIsExtern) {
                    /* For externals label, we do't know their value.
                     * We put zeros in the address, but set the appropriate ARE.
                     * In addition, we have to add this location to the 
                     * externals file. */
                    nOperand = ASM_COMBINE_EXTERNAL_WORD;
                    /* Add to the externals file */
                    eRetValue = BUFFER_AppendPrintf(hFile->hExternalsStream,
                            "%s\t%d\n",
                            ptLine->aptOperands[nIndex]->uValue.szStr,
                            ptLine->nCounter + 1 + nIndex);
                    if (eRetValue) {
                        return eRetValue;
                    }
                }
                else {
                    nOperand = ASM_COMBINE_DIRECT_WORD(nLabelAddress);
                }
                break;
            case LEX_TOKEN_KIND_REGISTER:
                /* If the next operand is also a register, combine them */
                if (nIndex+1 < ptLine->nOperandsLength
                        && LEX_TOKEN_KIND_REGISTER
                           == ptLine->aptOperands[nIndex+1]->eKind) {
                    nOperand = ASM_COMBINE_REGISTER_WORD(
                                ptLine->aptOperands[nIndex]->uValue.nNumber,
                                ptLine->aptOperands[nIndex+1]->uValue.nNumber);
                    /* Skip one extra operand */
                    nIndex++;
                } else if (nIndex+1 < ptLine->nOperandsLength) {
                    /* Source operand */
                    nOperand = ASM_COMBINE_REGISTER_WORD(
                                ptLine->aptOperands[nIndex]->uValue.nNumber, 0);
                } else {
                    /* Destination operand*/
                    nOperand = ASM_COMBINE_REGISTER_WORD(
                                0, ptLine->aptOperands[nIndex]->uValue.nNumber);
                }
                break;
            default:
                return GLOB_ERROR_UNKNOWN;
        }
        
        /* Write the operand to the binary */
        eRetValue = MEMSTREAM_AppendNumber(ptLine->hStream, nOperand);
        if (eRetValue) {
            return eRetValue;
        }
    }
    return GLOB_SUCCESS;
}
 
/******************************************************************************
 * Name:    asm_SecondPhase
 * Purpose: do the second phase of the compilation
 * Parameters:
 *          hFile [IN] - handle to the current file
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
static GLOB_ERROR asm_SecondPhase(HASM_FILE hFile) {
    GLOB_ERROR eRetValue = GLOB_SUCCESS;
    
    /* Run line by line */
    for (PASM_LINE ptLine = hFile->ptFirstLine;
         NULL != ptLine;
         ptLine = ptLine->ptNext) {
        eRetValue = asm_SecondPhaseCompileLine(hFile, ptLine);
        if (eRetValue) {
            return eRetValue;
        }
    }
    return GLOB_SUCCESS;
}

/******************************************************************************
 * Name:    asm_SymTableForEachCallback
 * Purpose: callback function to enumerate the symbols table
 * Parameters:
 *          pszName [IN] - symbol name
 *          nAddress [IN] - symbol address
 *          bIsMarkedForExport [IN] - should we include it in the entry file
 *          pContext [IN] - should be the HASH_FILE to the current file
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
static GLOB_ERROR asm_SymTableForEachCallback(const char * pszName,
                                              int nAddress, 
                                              BOOL bIsMarkedForExport,
                                              void * pContext) {
    HASM_FILE hFile = (HASM_FILE)pContext;
    
    /* Ignore symboles not for export */
    if (!bIsMarkedForExport) {
        return GLOB_SUCCESS;
    }
    
    /* add the symbol to the buffer of the entries file */
    return BUFFER_AppendPrintf(hFile->hEntriesStream,
            "%s\t%d\n", pszName, nAddress);
}

/******************************************************************************
 * Name:    asm_PrepareEntries
 * Purpose: prepare the content of the entries file
 * Parameters:
 *          hFile [IN] - the file we compile
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
static GLOB_ERROR asm_PrepareEntries(HASM_FILE hFile) {
    /* Go over all symbols. write the one that was marked for export.*/
    return SYMTABLE_ForEach(hFile->hSymTable,
            asm_SymTableForEachCallback, hFile);
}

/******************************************************************************
 * EXTERNAL FUNCTIONS
 * ------------------
 * See function-level documentation in the header file
 *****************************************************************************/

/******************************************************************************
 * Name:    ASM_Compile
 *****************************************************************************/
GLOB_ERROR ASM_Compile(const char * szFileName,
                       GLOB_ERRORCALLBACK pfnErrorsCallback,
                        void * pvContext,
                       PHASM_FILE phFile) {
    HASM_FILE hFile = NULL;
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;

    /* Allocate the handle */
    hFile = malloc(sizeof(*hFile));
    if (NULL == hFile) {
        return GLOB_ERROR_SYS_CALL_ERROR();
    }
    
    /* Init fields */
    hFile->pfnErrorsCallback = pfnErrorsCallback;
    hFile->pvErrorsCallbackContext = pvContext;
    hFile->hLex = NULL;
    hFile->hSymTable = NULL;
    hFile->hExternalsStream = NULL;
    hFile->hEntriesStream = NULL;
    hFile->nCodeCounter = CODE_STARTUP_ADDRESS;
    hFile->nDataCounter = 0;
    hFile->bHasErrors = FALSE;
    hFile->bHaveExternals = FALSE;
    hFile->bHaveEntries = FALSE;
    hFile->ptFirstLine = NULL;
    hFile->ptLastLine = NULL;
    
    /* Open the file for parsing. */
    eRetValue = LEX_Open(szFileName, pfnErrorsCallback, pvContext,&hFile->hLex);
    if (eRetValue) {
        ASM_Close(hFile);
        return eRetValue;
    }
    
    /* Create the symbols table */
    eRetValue = SYMTABLE_Create(&hFile->hSymTable);
    if (eRetValue) {
        ASM_Close(hFile);
        return eRetValue;
    }
    
    /* Create the Externals Stream */
    eRetValue = BUFFER_Create(&hFile->hExternalsStream);
    if (eRetValue) {
        ASM_Close(hFile);
        return eRetValue;
    }    

    /* Create the Entries Stream */
    eRetValue = BUFFER_Create(&hFile->hEntriesStream);
    if (eRetValue) {
        ASM_Close(hFile);
        return eRetValue;
    }

    /* Start the first phase */
    eRetValue = asm_FirstPhase(hFile);
    if (eRetValue) {
        ASM_Close(hFile);
        return eRetValue;
    }
    
    /* Check if we have errors */
    if (hFile->bHasErrors) {
        /* Stop the compilation*/
        ASM_Close(hFile);
        return GLOB_ERROR_PARSING_FAILED;
    }
    
    /* Finalize the symbols table */
    eRetValue = SYMTABLE_Finalize(hFile->hSymTable, hFile->nCodeCounter);
    if (GLOB_ERROR_NOT_FOUND == eRetValue) {
        asm_ReportError(hFile, TRUE, NULL, "one or more label were defined "
                "in .entry statement, but can't find them in the code");
        ASM_Close(hFile);
        return GLOB_ERROR_PARSING_FAILED;
    }
    if (eRetValue) {
        ASM_Close(hFile);
        return eRetValue;
    }
    
    /* Start second phase */
    eRetValue = asm_SecondPhase(hFile);
    if (eRetValue) {
        ASM_Close(hFile);
        return eRetValue;
    }
    
    if (hFile->bHasErrors) {
        return GLOB_ERROR_PARSING_FAILED;
    }
    
    /* Prepare Entries buffer */
    eRetValue = asm_PrepareEntries(hFile);
    if (eRetValue) {
        ASM_Close(hFile);
        return eRetValue;
    }
    *phFile = hFile;
    return GLOB_SUCCESS;
}

GLOB_ERROR ASM_WriteBinary(HASM_FILE hFile, PHMEMSTREAM phStream, int * nCode, int * nData) {
    HMEMSTREAM hStream = NULL;
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    if (NULL == hFile || NULL == phStream || NULL == nCode || NULL == nData) {
        return GLOB_ERROR_INVALID_PARAMETERS;
    }
    if (hFile->bHasErrors) {
        return GLOB_ERROR_INVALID_STATE;
    }
    
    eRetValue = MEMSTREAM_Create(&hStream);
    if (eRetValue) {
        return eRetValue;
    }
    
    /* Append Code */
    for (PASM_LINE ptLine = hFile->ptFirstLine;
            ptLine != NULL;
            ptLine = ptLine->ptNext) {
        if (!ptLine->bIsData) {
            eRetValue = MEMSTREAM_Concat(hStream, ptLine->hStream);
            if (eRetValue) {
                MEMSTREAM_Free(hStream);
                return eRetValue;
            }
        }
    }
    
    /* Append Data */
    for (PASM_LINE ptLine = hFile->ptFirstLine;
            ptLine != NULL;
            ptLine = ptLine->ptNext) {
        if (ptLine->bIsData) {
            eRetValue = MEMSTREAM_Concat(hStream, ptLine->hStream);
            if (eRetValue) {
                MEMSTREAM_Free(hStream);
                return eRetValue;
            }
        }
    }
    
    /* Set out parameters */
    *phStream = hStream;
    *nCode = hFile->nCodeCounter - CODE_STARTUP_ADDRESS;
    *nData = hFile->nDataCounter;
    
    return GLOB_SUCCESS;
}

/******************************************************************************
 * Name:    ASM_GetEntries
 *****************************************************************************/
GLOB_ERROR ASM_GetEntries(HASM_FILE hFile, char ** ppszEntries, int * nLength) {
    if (NULL == hFile || NULL == ppszEntries || NULL == nLength) {
        return GLOB_ERROR_INVALID_PARAMETERS;
    }
    if (!hFile->bHaveEntries) {
        *nLength = 0;
        return GLOB_SUCCESS;
    }
    return BUFFER_GetStream(hFile->hEntriesStream, ppszEntries, nLength);
}

/******************************************************************************
 * Name:    ASM_GetExternals
 *****************************************************************************/
GLOB_ERROR ASM_GetExternals(HASM_FILE hFile,
                            char ** ppszExternals,
                            int * nLength) {
    if (NULL == hFile || NULL == ppszExternals || NULL == nLength) {
        return GLOB_ERROR_INVALID_PARAMETERS;
    }
    if (!hFile->bHaveExternals) {
        *nLength = 0;
        return GLOB_SUCCESS;
    }
    return BUFFER_GetStream(hFile->hExternalsStream, ppszExternals, nLength);
}

/******************************************************************************
 * Name:    ASM_Close
 *****************************************************************************/
void ASM_Close(HASM_FILE hFile) {
    PASM_LINE ptLineToFree = NULL;
    if (NULL != hFile->hLex) {
        LEX_Close(hFile->hLex);
    }
    if (NULL != hFile->hSymTable) {
        SYMTABLE_Free(hFile->hSymTable);
    }
    if (NULL != hFile->hEntriesStream) {
        BUFFER_Free(hFile->hEntriesStream);
    }
    if (NULL != hFile->hExternalsStream) {
        BUFFER_Free(hFile->hExternalsStream);
    }
    
    /* free all lines */
    while (NULL != hFile->ptFirstLine) {
        ptLineToFree = hFile->ptFirstLine;
        hFile->ptFirstLine = ptLineToFree->ptNext;
        MEMSTREAM_Free(ptLineToFree->hStream);
        for (int nIndex = 0; nIndex < ARRAY_ELEMENTS(ptLineToFree->aptOperands); nIndex++) {
            if (NULL != ptLineToFree->aptOperands[nIndex]) {
                LEX_FreeToken(ptLineToFree->aptOperands[nIndex]);
            }
        }
        free(ptLineToFree);
    }
    /* free the handle itself */
    free(hFile);
}
