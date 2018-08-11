/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   asm.c
 * Author: boazgildor
 * 
 * Created on August 7, 2018, 10:37 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "helper.h"
#include "global.h"
#include "lex.h"
#include "linestr.h"
#include "symtable.h"
#include "memstream.h"
#include "asm.h"

#define ASM_MAX_OPERANDS 3
#define ASM_HAS_OPERANDS(opcode)  (g_znAllowedOperands[opcode] > 0)
#define ASM_HAS_SOURCE_OPERAND(opcode) (g_znAllowedOperands[opcode] & 0xF0)
#define ASM_GET_ALLOWED_SOURCE_OPERAND(opcode) ((g_znAllowedOperands[opcode] & 0xF0) >> 4)
#define ASM_HAS_DESTINATION_OPERAND(opcode) (g_znAllowedOperands[opcode] & 0x0F)
#define ASM_GET_ALLOWED_DESTINATION_OPERAND(opcode) ASM_HAS_DESTINATION_OPERAND(opcode)
#define ASM_IS_IMMEDIATE_OPERAND_ALLOWED(operand) ((operand) & 0x1)
#define ASM_IS_DIRECT_OPERAND_ALLOWED(operand) ((operand) & 0x2)
#define ASM_IS_PARAMETER_OPERAND_ALLOWED(operand) ((operand) & 0x4)
#define ASM_IS_REGISTER_OPERAND_ALLOWED(operand) ((operand) & 0x8)


#define ASM_COMBINE_FIRST_WORD(eParam1, eParam2, eOpcode, eSource, eDest) \
    ((eParam1)<<12 | (eParam2)<<10 | (eOpcode)<<6 | (eSource)<<4 | (eDest)<<2 | (ASM_OPERAND_METHOD_IMMEDIATE))

typedef enum ASM_OPERAND_METHOD {
    ASM_OPERAND_METHOD_IMMEDIATE = 0,
    ASM_OPERAND_METHOD_DIRECT = 1,
    ASM_OPERAND_METHOD_PARAMETERS = 2,
    ASM_OPERAND_METHOD_REGISTER = 3,
} ASM_OPERAND_METHOD;

typedef enum ASM_ARE {
    ASM_ARE_ABSOLUTE = 0,
    ASM_ARE_EXTERNAL = 1,
    ASM_ARE_RELOCATABLE = 2,
} ASM_ARE;

typedef struct ASM_LINE {
    HMEMSTREAM hStream;
    PLEX_TOKEN aptOperands[ASM_MAX_OPERANDS];
    int nOperandsLength;
    int nLength;
    BOOL bIsData;
    ASM_OPERAND_METHOD eParam1;
    ASM_OPERAND_METHOD eParam2;
    ASM_OPERAND_METHOD eSourceParam;
    ASM_OPERAND_METHOD eDestParam;
    
    struct ASM_LINE * ptNext;
} ASM_LINE, *PASM_LINE;

struct ASM_FILE {
    /* Handle to the LEX "instance" that parse the file. */
    HLEX_FILE hLex;
    
    PSYMTABLE_TABLE hSymTable;
    
    int nCodeCounter;
    int nDataCounter;
    BOOL bHasErrors;
    
    PASM_LINE ptFirstLine;
    PASM_LINE ptLastLine;
};

int g_znAllowedOperands[] = 
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
 * Name:    lex_ReportError TODO
 * Purpose: Print parsing error message
 * Parameters:
 *          hFile [IN] - handle to the current file
 *          bIsError [IN] - TRUE for error. FALSE for warning
 *          nColumn [IN] - zero-based column index of the error
 *          pszErroFormat[IN] - error message (format as printf syntax)
 *          ... [IN] - parameters to include in the message
  *****************************************************************************/
static void asm_ReportError(HASM_FILE hFile, BOOL bIsError,
                            const char * pszErrorFormat, ...) {    
    /* Print the error message. */
    printf("%s ", bIsError ? "error" : "warning");
    va_list vaArgs;
    va_start (vaArgs, pszErrorFormat);
    vprintf (pszErrorFormat, vaArgs);
    va_end (vaArgs);
    printf("\n");
}
static GLOB_ERROR asm_FirstPhaseCompileString(HASM_FILE hFile, PASM_LINE ptLine) {
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    PLEX_TOKEN ptStringToken = NULL;
    eRetValue = LEX_ReadNextToken(hFile->hLex, &ptStringToken);
    if (GLOB_SUCCESS != eRetValue && GLOB_ERROR_END_OF_LINE != eRetValue) {
        return eRetValue;
    }
    
    if (GLOB_ERROR_END_OF_LINE == eRetValue || ptStringToken->eKind != LEX_TOKEN_KIND_STRING) {
        asm_ReportError(hFile, TRUE, "String is expected");
        if (GLOB_SUCCESS == eRetValue) {
            LEX_FreeToken(ptStringToken);
        }
        return GLOB_ERROR_PARSING_FAILED;
    }
    eRetValue = MEMSTREAM_AppendString(ptLine->hStream, ptStringToken->uValue.szStr);
    ptLine->nLength = strlen(ptStringToken->uValue.szStr)+1;
    ptLine->bIsData = TRUE;
    LEX_FreeToken(ptStringToken);
    return eRetValue;
}

static GLOB_ERROR asm_FirstPhaseCompileData(HASM_FILE hFile, PASM_LINE ptLine) {
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    PLEX_TOKEN ptToken = NULL;
    
    ptLine->bIsData = TRUE;
    while (GLOB_ERROR_END_OF_LINE != eRetValue) {
        eRetValue = LEX_ReadNextToken(hFile->hLex, &ptToken);
        if (GLOB_SUCCESS != eRetValue && GLOB_ERROR_END_OF_LINE != eRetValue) {
            return eRetValue;
        }

        if (GLOB_ERROR_END_OF_LINE == eRetValue || ptToken->eKind != LEX_TOKEN_KIND_NUMBER) {
            asm_ReportError(hFile, TRUE, "Number (data) is expected");
            if (GLOB_SUCCESS == eRetValue) {
                LEX_FreeToken(ptToken);
            }
            return GLOB_ERROR_PARSING_FAILED;
        }
        
        eRetValue = MEMSTREAM_AppendNumber(ptLine->hStream, ptToken->uValue.nNumber);
        ptLine->nLength++;
        LEX_FreeToken(ptToken);
        
        /* Check if have a comma (to continue the data */
        eRetValue = LEX_ReadNextToken(hFile->hLex, &ptToken);
        if (GLOB_SUCCESS != eRetValue && GLOB_ERROR_END_OF_LINE != eRetValue) {
            return eRetValue;
        }
    }
    
    return GLOB_SUCCESS;
}

static GLOB_ERROR asm_FirstPhaseCompileExtern(HASM_FILE hFile, PASM_LINE ptLine) {
    return GLOB_ERROR_UNKNOWN;
}


static GLOB_ERROR asm_FirstPhaseCompileEntry(HASM_FILE hFile, PASM_LINE ptLine) {
    return GLOB_ERROR_UNKNOWN;
}

static GLOB_ERROR asm_FirstPhaseReadOperand(HASM_FILE hFile, ASM_OPERAND_METHOD * peMethod, int nAllowedOperandType, PASM_LINE ptLine);


static GLOB_ERROR asm_FirstPhaseReadParameterOperand(HASM_FILE hFile, PASM_LINE ptLine) {
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    PLEX_TOKEN ptToken = NULL;
    
    // TODO spaces should not be allowed in the parameter
    
    /* Try to read the '(' */
    eRetValue = LEX_ReadNextToken(hFile->hLex, &ptToken);
    if (eRetValue) {
        return eRetValue;
    }
    if (LEX_TOKEN_KIND_SPECIAL != ptToken->eKind || '(' != ptToken->uValue.cChar) {
        asm_ReportError(hFile, TRUE, "a '(' or end of line is expected");
        LEX_FreeToken(ptToken);
        return GLOB_ERROR_PARSING_FAILED;
    }
    LEX_FreeToken(ptToken);
    
    eRetValue = asm_FirstPhaseReadOperand(hFile, &ptLine->eParam1, 0xB, ptLine);
    if (eRetValue) {
        return eRetValue;
    }
    
    /* Read the comma */
    eRetValue = LEX_ReadNextToken(hFile->hLex, &ptToken);
    if (eRetValue) {
        return eRetValue;
    }
    if (LEX_TOKEN_KIND_SPECIAL != ptToken->eKind || ',' != ptToken->uValue.cChar) {
        asm_ReportError(hFile, TRUE, "a ',' is expected");
        LEX_FreeToken(ptToken);
        return GLOB_ERROR_PARSING_FAILED;
    }
    LEX_FreeToken(ptToken);

    
    eRetValue = asm_FirstPhaseReadOperand(hFile, &ptLine->eParam2, 0xB, ptLine);
    if (eRetValue) {
        return eRetValue;
    }
    
    /* Read the ')' */
    eRetValue = LEX_ReadNextToken(hFile->hLex, &ptToken);
    if (eRetValue) {
        return eRetValue;
    }
    if (LEX_TOKEN_KIND_SPECIAL != ptToken->eKind || ')' != ptToken->uValue.cChar) {
        asm_ReportError(hFile, TRUE, "a ')' is expected");
        LEX_FreeToken(ptToken);
        return GLOB_ERROR_PARSING_FAILED;
    }
    LEX_FreeToken(ptToken);

    return GLOB_SUCCESS;
}

static GLOB_ERROR asm_FirstPhaseReadOperand(HASM_FILE hFile, ASM_OPERAND_METHOD * peMethod, int nAllowedOperandType, PASM_LINE ptLine) {
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    PLEX_TOKEN ptToken = NULL;
    ASM_OPERAND_METHOD eMethod;


    /* Read the operand */
    eRetValue = LEX_ReadNextToken(hFile->hLex, &ptToken);
    if (GLOB_SUCCESS != eRetValue && GLOB_ERROR_END_OF_LINE != eRetValue) {
        return eRetValue;
    }
    if (GLOB_ERROR_END_OF_LINE == eRetValue) {
        asm_ReportError(hFile, TRUE, "an operand is expected");
        return GLOB_ERROR_PARSING_FAILED;
    }
    if (ASM_IS_IMMEDIATE_OPERAND_ALLOWED(nAllowedOperandType) && LEX_TOKEN_KIND_IMMED_NUMBER == ptToken->eKind) {
        // immediate
        eMethod = ASM_OPERAND_METHOD_IMMEDIATE;
    } else if (ASM_IS_DIRECT_OPERAND_ALLOWED(nAllowedOperandType) && LEX_TOKEN_KIND_WORD == ptToken->eKind) {
        // direct (including parameter)
        eMethod = ASM_OPERAND_METHOD_DIRECT; // TODO parameter
        if (ASM_IS_PARAMETER_OPERAND_ALLOWED(nAllowedOperandType)) {
            // we are at the destination operand
            // try to read the next token
            eRetValue = asm_FirstPhaseReadParameterOperand(hFile, ptLine);
            if (GLOB_SUCCESS != eRetValue && GLOB_ERROR_END_OF_LINE != eRetValue) {
                return eRetValue;
            }
            if (GLOB_SUCCESS == eRetValue) {
                eMethod = ASM_OPERAND_METHOD_PARAMETERS;
            }
        }
    } else if (ASM_IS_REGISTER_OPERAND_ALLOWED(nAllowedOperandType) && LEX_TOKEN_KIND_REGISTER == ptToken->eKind) {
        eMethod = ASM_OPERAND_METHOD_REGISTER;
    } else {
        asm_ReportError(hFile, TRUE, "Unsupported operand");
        LEX_FreeToken(ptToken);
        return GLOB_ERROR_PARSING_FAILED;
    }
    *peMethod = eMethod;
    
    ptLine->aptOperands[ptLine->nOperandsLength] = ptToken;
    ptLine->nOperandsLength++;
    return GLOB_SUCCESS;
}
static GLOB_ERROR asm_FirstPhaseCompileSourceOperand(HASM_FILE hFile, GLOB_OPCODE eOpcode, PASM_LINE ptLine) {
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    PLEX_TOKEN ptCommaToken = NULL;
    
    /* Check of this opcode has source operand */
    if (!ASM_HAS_SOURCE_OPERAND(eOpcode)) {
        /* No source operand */
        return GLOB_SUCCESS;
    }
    
    eRetValue = asm_FirstPhaseReadOperand(hFile, &ptLine->eSourceParam, ASM_GET_ALLOWED_SOURCE_OPERAND(eOpcode),ptLine);
    if (eRetValue) {
        return eRetValue;
    }
    
    /* Read the comma */
    eRetValue = LEX_ReadNextToken(hFile->hLex, &ptCommaToken);
    if (GLOB_SUCCESS != eRetValue && GLOB_ERROR_END_OF_LINE != eRetValue) {
        return eRetValue;
    }
    if (GLOB_ERROR_END_OF_LINE == eRetValue) {
        asm_ReportError(hFile, TRUE, "a comma is expected");
        return GLOB_ERROR_PARSING_FAILED;
    }
    if (LEX_TOKEN_KIND_SPECIAL != ptCommaToken->eKind || ',' != ptCommaToken->uValue.cChar) {
        asm_ReportError(hFile, TRUE, "a comma is expected");
        LEX_FreeToken(ptCommaToken);
        return GLOB_ERROR_PARSING_FAILED;
    }
    
    
    return GLOB_SUCCESS;
}

static GLOB_ERROR asm_FirstPhaseCompileDestinationOperand(HASM_FILE hFile, GLOB_OPCODE eOpcode, PASM_LINE ptLine) {
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    
    /* Check of this opcode has source operand */
    if (!ASM_HAS_DESTINATION_OPERAND(eOpcode)) {
        /* No source operand */
        return GLOB_SUCCESS;
    }
    eRetValue = asm_FirstPhaseReadOperand(hFile, &ptLine->eDestParam, ASM_GET_ALLOWED_DESTINATION_OPERAND(eOpcode),ptLine);
    if (eRetValue) {
        return eRetValue;
    }
    
    return GLOB_SUCCESS;
}

static GLOB_ERROR asm_FirstPhaseCompileOpcode(HASM_FILE hFile, GLOB_OPCODE eOpcode, PASM_LINE ptLine) {
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
    
    ptLine->nLength = 1;
    ptLine->bIsData = FALSE;
    return GLOB_SUCCESS;
}

static GLOB_ERROR asm_HandleLabelDefinition(HASM_FILE hFile, PLEX_TOKEN * pptToken) {
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    SYMTABLE_SYMTYPE eLabelType = SYMTABLE_SYMTYPE_DATA;
    int nLabelAddress = 0;

    PLEX_TOKEN ptLabelToken = NULL;

    if (LEX_TOKEN_KIND_LABEL != (*pptToken)->eKind) {
        return GLOB_SUCCESS;
    }
    ptLabelToken = *pptToken;
    *pptToken = NULL;
    eRetValue = LEX_ReadNextToken(hFile->hLex, pptToken);
    if (GLOB_ERROR_END_OF_LINE == eRetValue) {
        /* Line can't contain only a label definition */
        asm_ReportError(hFile, TRUE, "an opcode or directive is expected");
        LEX_FreeToken(ptLabelToken);
        return GLOB_ERROR_PARSING_FAILED;
    }
    
    if (LEX_TOKEN_KIND_DIRECTIVE != (*pptToken)->eKind && LEX_TOKEN_KIND_OPCODE != (*pptToken)->eKind) {
        asm_ReportError(hFile, TRUE, "an opcode or directive is expected");
        LEX_FreeToken(ptLabelToken);
        return GLOB_ERROR_PARSING_FAILED;        
    }
    
    if (LEX_TOKEN_KIND_OPCODE == (*pptToken)->eKind) {
        eLabelType = SYMTABLE_SYMTYPE_CODE;
        nLabelAddress = hFile->nCodeCounter;
    } else {
        if (GLOB_DIRECTIVE_EXTERN == (*pptToken)->uValue.eDiretive
                || GLOB_DIRECTIVE_ENTRY == (*pptToken)->uValue.eDiretive) {
            /* Label definition in .extern or .entry. report a warning and ignore */
            asm_ReportError(hFile, FALSE, "Label is defined in .extern or .entry statement");
            LEX_FreeToken(ptLabelToken);
            return GLOB_SUCCESS;
        }
        eLabelType = SYMTABLE_SYMTYPE_DATA;
        nLabelAddress = hFile->nDataCounter;
    }
    
    /* Insert the label */
    eRetValue = SYMTABLE_Insert(hFile->hSymTable, ptLabelToken->uValue.szStr,
                     eLabelType, nLabelAddress, FALSE);
    if (GLOB_ERROR_ALREADY_EXIST == eRetValue) {
        asm_ReportError(hFile, TRUE, "Duplicate label definition");
        LEX_FreeToken(ptLabelToken);
        return GLOB_SUCCESS; // return with SUCCESS to continue parsing the line
    }
    if (eRetValue) {
        LEX_FreeToken(ptLabelToken);
        return eRetValue;
    }
    LEX_FreeToken(ptLabelToken);
    return GLOB_SUCCESS;
}
static GLOB_ERROR asm_FirstPhaseCompileLineContent(HASM_FILE hFile, PLEX_TOKEN ptToken, PASM_LINE ptLine) {
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
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
        eRetValue = asm_FirstPhaseCompileOpcode(hFile, ptToken->uValue.eOpcode, ptLine);
    } else {
        asm_ReportError(hFile, TRUE, "an opcode or directive is expected");
        eRetValue = GLOB_ERROR_PARSING_FAILED;
    }
    LEX_FreeToken(ptToken);
    return eRetValue;
}

static GLOB_ERROR asm_FirstPhaseCompileNonEmptyLine(HASM_FILE hFile, PLEX_TOKEN ptToken) {
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
    for (int nIndex = 0; nIndex < ARRAY_ELEMENTS(ptLine->aptOperands); nIndex++) {
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
        LEX_FreeToken(ptToken);
        free(ptLine);
        return eRetValue;
    }

    /* Add the line to the list */
    if (NULL == hFile->ptFirstLine) {
        hFile->ptFirstLine = ptLine;
    } else {
        hFile->ptLastLine->ptNext = ptLine;
    }
    hFile->ptLastLine = ptLine;

    /* Update the data/code counter */
    if (ptLine->bIsData) {
        hFile->nDataCounter += ptLine->nLength;
    } else {
        hFile->nCodeCounter += ptLine->nLength;
    }
    
    return GLOB_SUCCESS;
}

static GLOB_ERROR asm_FirstPhaseCompileLine(HASM_FILE hFile) {
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    PLEX_TOKEN ptToken = NULL;
    
    /* Read the first token of the line.*/
    eRetValue = LEX_ReadNextToken(hFile->hLex, &ptToken);
    switch (eRetValue) {
        case GLOB_ERROR_PARSING_FAILED:
            hFile->bHasErrors = TRUE;
        case GLOB_SUCCESS:
        case GLOB_ERROR_END_OF_LINE:
            break;
        case GLOB_ERROR_END_OF_FILE:
            return GLOB_ERROR_END_OF_FILE;
        default:
            return eRetValue;
    }
    if (GLOB_ERROR_END_OF_LINE == eRetValue || GLOB_ERROR_PARSING_FAILED == eRetValue) {
        eRetValue = GLOB_SUCCESS;
    } else {
        eRetValue = asm_FirstPhaseCompileNonEmptyLine(hFile, ptToken);
        if (GLOB_ERROR_PARSING_FAILED == eRetValue) { 
            eRetValue = GLOB_SUCCESS;
        }
    }
    LEX_MoveToNextLine(hFile->hLex);
    return eRetValue;
}
    
static GLOB_ERROR asm_FirstPhase(HASM_FILE hFile) {
    GLOB_ERROR eRetValue = GLOB_SUCCESS;
    while (GLOB_SUCCESS == eRetValue) {
        eRetValue = asm_FirstPhaseCompileLine(hFile);
    }
    return GLOB_ERROR_END_OF_FILE == eRetValue ? GLOB_SUCCESS : eRetValue;
}

GLOB_ERROR ASM_Compile(const char * szFileName, PHASM_FILE phFile) {
    HASM_FILE hFile = NULL;
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;

    /* Allocate the handle */
    hFile = malloc(sizeof(*hFile));
    if (NULL == hFile) {
        return GLOB_ERROR_SYS_CALL_ERROR();
    }
    
    /* Init fields */
    hFile->hLex = NULL;
    hFile->hSymTable = NULL;
    hFile->nCodeCounter = CODE_STARTUP_ADDRESS;
    hFile->nDataCounter = 0;
    hFile->bHasErrors = FALSE;
    hFile->ptFirstLine = NULL;
    hFile->ptLastLine = NULL;
    
    /* Open the file for parsing. */
    eRetValue = LEX_Open(szFileName, &hFile->hLex);
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
}

void ASM_Close(HASM_FILE hFile) {
    PASM_LINE ptLineToFree = NULL;
    LEX_Close(hFile->hLex);
    SYMTABLE_Free(hFile->hSymTable);
    
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
        
    }
    free(hFile);
}
