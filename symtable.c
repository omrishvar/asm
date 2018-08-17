/******************************************************************************
 * File:    symtable.h
 * Author:  Doron Shvartztuch
 * The SYMTABLE module provides functions to create and manipulate
 * the symbols table.
 * 
 * Implementation:
 * The table is implemented as an array allocated on dynamic memory. In case
 * there is not enough space, we use the realloc method to expand.
 *****************************************************************************/

/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtable.h"

/******************************************************************************
 * CONSTANTS & MACROS
 *****************************************************************************/

/* The default size (in records) of the table */
#define SYMTABLE_DEFAULT_TABLE_SIZE 1

/* The expand factor to use when the table is full */
#define SYMTABLE_ALLOCATION_FACTOR 2

/******************************************************************************
 * TYPEDEFS
 *****************************************************************************/

/* The struct that represents a record in the table */
typedef struct SYMTABLE_RECORD {
    
    /* The symbol name. Saved on dynamic memory allocated separated. */
    char * pszName;
    
    /* Symbol Type */
    SYMTABLE_SYMTYPE eType;
    
    /* Address of the symbol. 0 for extern symbols */
    int nAddress;
    
    /* Whether this symbol is extern */
    BOOL bIsExtern;
    
    /* Whether this symbol is for export */
    BOOL bMarkedForExport;
} SYMTABLE_RECORD, *PSYMTABLE_RECORD;

/* SYMTABLE_TABLE is the struct behind the the HSYMTABLE_TABLE.
 * It keeps some information about the symbols table
 * as well as the table itself.  */
struct SYMTABLE_TABLE {
    
    /* Whether the table is finalized.
     * Changes cannot be made for finalized tables. */
    BOOL bIsFinalized;
    
    /* Number of allocated records in the table (array) */
    int nAllocatedRecords;
    
    /* Number of used records in the table (array) */
    int nUsedRecords;
    
    /* Pointer to the table (array) */
    PSYMTABLE_RECORD patTable;
};

/******************************************************************************
 * INTERNAL FUNCTIONS (prototypes)
 * -------------------------------
 * See function-level documentation next to the implementation below
 *****************************************************************************/
static int symtable_FindSymbol(HSYMTABLE_TABLE table, const char *name);
static GLOB_ERROR symtable_InsertRecord(HSYMTABLE_TABLE hTable,
                                        const char *pszName,
                                        SYMTABLE_SYMTYPE eType,
                                        int nAddress,
                                        BOOL isExtern,
                                        BOOL markedForExport);


/******************************************************************************
 * INTERNAL FUNCTIONS
 *****************************************************************************/

/******************************************************************************
 * Name:    symtable_FindSymbol
 * Purpose: find a symbol in the table
 * Parameters:
 *          hTable [IN] - the handle to the symbols table
 *          pszName [IN] - the symbol name
 * Return Value:
 *          The index of the symbol in the array. -1 if not found.
 *****************************************************************************/
static int symtable_FindSymbol(HSYMTABLE_TABLE hTable, const char *pszName) {
    for (int nIndex = 0; nIndex < hTable->nUsedRecords; nIndex++) {
        /* Symbols name are case-sensitive */
        if (0 == strcmp(pszName, hTable->patTable[nIndex].pszName)) {
            return nIndex;
        }
    }
    return -1;
}

/******************************************************************************
 * Name:    symtable_InsertRecord
 * Purpose: Insert a new record the table
 * Parameters:
 *          hTable [IN] - the handle to the symbols table
 *          pszName [IN] - the symbol name to insert
 *          eType [IN] - the type of the symbol. meaningless for extern symbols
 *          nAddress [IN] - the address of the symbol. 0 for extern symbols
 *          bIsExtern [IN] - whether the symbol is declared as extern.
 *          bMarkedForExport [IN] - whether the symbol is for export
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
static GLOB_ERROR symtable_InsertRecord(HSYMTABLE_TABLE hTable,
                                        const char *pszName,
                                        SYMTABLE_SYMTYPE eType,
                                        int nAddress,
                                        BOOL bIsExtern,
                                        BOOL bMarkedForExport) {
    PSYMTABLE_RECORD patNewTable = NULL;
    int newAllocatedRecords = 0;

    /* Check if the table is full */
    if (hTable->nUsedRecords == hTable->nAllocatedRecords) {
        /* Table is full, exapnd it*/
        
        /* Calculate the new size (in elements) of the table */
        newAllocatedRecords =
                SYMTABLE_ALLOCATION_FACTOR * hTable->nAllocatedRecords;
        
        /* try to reallocate the memory */
        patNewTable = realloc(hTable->patTable,
                              newAllocatedRecords * sizeof(*patNewTable));
        if (NULL == patNewTable) {
            return GLOB_ERROR_SYS_CALL_ERROR();
        }
        
        /* Update the main structure with the new table */
        hTable->patTable = patNewTable;
        hTable->nAllocatedRecords = newAllocatedRecords;
    }
    
    /* Now we are sure there is an empty space in the table. */

    /* Allocate memory for the symbol name. take one extra char for '\0'*/
    hTable->patTable[hTable->nUsedRecords].pszName = malloc(strlen(pszName)+1);
    if (NULL == hTable->patTable[hTable->nUsedRecords].pszName) {
        return GLOB_ERROR_SYS_CALL_ERROR();
    }
    
    /* Set the fields */
    strcpy(hTable->patTable[hTable->nUsedRecords].pszName, pszName);
    hTable->patTable[hTable->nUsedRecords].eType = eType;
    hTable->patTable[hTable->nUsedRecords].nAddress = nAddress;
    hTable->patTable[hTable->nUsedRecords].bIsExtern = bIsExtern;
    hTable->patTable[hTable->nUsedRecords].bMarkedForExport = bMarkedForExport;
    
    /* Update number of used records */
    hTable->nUsedRecords++;
    return GLOB_SUCCESS;
}

/******************************************************************************
 * EXTERNAL FUNCTIONS
 * ------------------
 * See function-level documentation in the header file
 *****************************************************************************/

/******************************************************************************
 * Name:    SYMTABLE_Create
 *****************************************************************************/
GLOB_ERROR SYMTABLE_Create(HSYMTABLE_TABLE *phTable) {
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    HSYMTABLE_TABLE  hTable = NULL;

    /* Check parameters */
    if (NULL == phTable) {
        return GLOB_ERROR_INVALID_PARAMETERS;
    }
    /* Allocate the handle structure */
    hTable = (HSYMTABLE_TABLE)malloc(sizeof(*hTable));
    if (NULL == hTable) {
        return GLOB_ERROR_SYS_CALL_ERROR();
    }
    
    /* Allocate the table in the default size */
    hTable->patTable = (PSYMTABLE_RECORD)malloc(SYMTABLE_DEFAULT_TABLE_SIZE * sizeof(SYMTABLE_RECORD));
    if(NULL == hTable->patTable) {
        eRetValue = GLOB_ERROR_SYS_CALL_ERROR();
        free(hTable);
        return eRetValue;
    }
    
    /* Init fields and set out parameters */
    hTable->nAllocatedRecords = SYMTABLE_DEFAULT_TABLE_SIZE;
    hTable->nUsedRecords = 0;
    hTable->bIsFinalized = FALSE;
    *phTable = hTable;
    return GLOB_SUCCESS;
}

/******************************************************************************
 * Name:    SYMTABLE_Insert
 *****************************************************************************/
GLOB_ERROR SYMTABLE_Insert(HSYMTABLE_TABLE hTable,
                           const char *pszName,
                           SYMTABLE_SYMTYPE eType,
                           int nAddress,
                           BOOL bIsExtern) {
    int nIndex = 0;
    
    /* Check parameters */
    if (NULL == hTable) {
        return GLOB_ERROR_INVALID_PARAMETERS;
    }
    
    /* Can't insert new symbols to a finalized table */
    if (hTable->bIsFinalized) {
        return GLOB_ERROR_INVALID_STATE;
    }
    
    /* Check if the symbol is already exist */
    nIndex = symtable_FindSymbol(hTable, pszName);
    if (-1 != nIndex) {
        /* Symbol already exist in the table, there are some cases... */
        
        /* check if it exist because previous call to SYMTABLE_Insert */
        if (hTable->patTable[nIndex].bIsExtern
            || 0 != hTable->patTable[nIndex].nAddress) {
            /* Symbol already exist (as regular or extern) */
            return GLOB_ERROR_ALREADY_EXIST;
        }
        
        /* If we here, the symbol is marked for export
         * We can't allow to insert this symbol as extern */
        if (bIsExtern) {
            return GLOB_ERROR_EXPORT_AND_EXTERN;        
        }
        
        /* Update the record of the symbol */
        hTable->patTable[nIndex].eType = eType;
        hTable->patTable[nIndex].nAddress = nAddress;
        return GLOB_SUCCESS;
    }
    
    /* Insert a new symbol to the table */
    return symtable_InsertRecord(hTable, pszName, eType,
                                 nAddress, bIsExtern, FALSE);
}

/******************************************************************************
 * Name:    SYMTABLE_MarkForExport
 *****************************************************************************/
GLOB_ERROR SYMTABLE_MarkForExport(HSYMTABLE_TABLE hTable, const char *pszName) {
    int nIndex = 0;
    
    /* Check parameters */
    if (NULL == hTable) {
        return GLOB_ERROR_INVALID_PARAMETERS;
    }
    
    /* Can't modify finalized table */
    if (hTable->bIsFinalized) {
        return GLOB_ERROR_INVALID_STATE;
    }
    
    /* Check if the symbol already exist in the table */
    nIndex = symtable_FindSymbol(hTable, pszName);
    if (nIndex == -1) {
        /* Symbol not found. just add it. */
        return symtable_InsertRecord(hTable, pszName,
                SYMTABLE_SYMTYPE_CODE /*Unused*/, 0, FALSE, TRUE);
    }
    
    /* Extern symbol can't be marked for export */
    if (hTable->patTable[nIndex].bIsExtern) {
        return GLOB_ERROR_EXPORT_AND_EXTERN;
    }
    
    /* check if the symbol already marked for export */
    if (hTable->patTable[nIndex].bMarkedForExport) {
        return GLOB_ERROR_ALREADY_EXIST;
    }
    
    /* Update the record */
    hTable->patTable[nIndex].bMarkedForExport = TRUE;
    return GLOB_SUCCESS;
}

/******************************************************************************
 * Name:    SYMTABLE_Finalize
 *****************************************************************************/
GLOB_ERROR SYMTABLE_Finalize(HSYMTABLE_TABLE hTable, int nDataOffset) {
    /* Check parameters */
    if (NULL == hTable) {
        return GLOB_ERROR_INVALID_PARAMETERS;
    }
    
    /* Check if the table is already finalized */
    if (hTable->bIsFinalized) {
        return GLOB_ERROR_INVALID_STATE;
    }
    
    /* Go over all the records in the table */
    for (int nIndex = 0; nIndex < hTable->nUsedRecords; nIndex++) {
        
        /* Check if all export symbols got a value */
        if (0 == hTable->patTable[nIndex].nAddress
            && hTable->patTable[nIndex].bMarkedForExport) {
            return GLOB_ERROR_NOT_FOUND;
        }
        
        /* In case of data symbol we have to add the offset */
        if (hTable->patTable[nIndex].eType == SYMTABLE_SYMTYPE_DATA
                && !hTable->patTable[nIndex].bIsExtern) {
            hTable->patTable[nIndex].nAddress += nDataOffset;
        }
    }
    /* Set the table state */
    hTable->bIsFinalized = TRUE;
    return GLOB_SUCCESS;    
}

/******************************************************************************
 * Name:    SYMTABLE_GetSymbolInfo
 *****************************************************************************/
GLOB_ERROR SYMTABLE_GetSymbolInfo(HSYMTABLE_TABLE hTable,
                                  const char *pszName,
                                  int *pnAddress,
                                  BOOL *pbIsExtern) {
    int nIndex = 0;
    
    /* Check parameters */
    if (NULL == hTable) {
        return GLOB_ERROR_INVALID_PARAMETERS;
    }
    
    /* Check if the table is not finalized yet */
    if (!hTable->bIsFinalized) {
        return GLOB_ERROR_INVALID_STATE;
    }
    
    /* Search the symbol */
    nIndex = symtable_FindSymbol(hTable, pszName);
    if (nIndex == -1) {
        return GLOB_ERROR_NOT_FOUND;
    }
    
    /* Set the out parameters */
    *pnAddress = hTable->patTable[nIndex].nAddress;
    *pbIsExtern = hTable->patTable[nIndex].bIsExtern;
    return GLOB_SUCCESS;
}

/******************************************************************************
 * Name:    SYMTABLE_ForEach
 *****************************************************************************/
GLOB_ERROR SYMTABLE_ForEach(HSYMTABLE_TABLE hTable,
                            SYMTABLE_FOREACH_CALLBACK pfCallback,
                            void * pvContext) {
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    
    /* Check parameters */
    if (NULL == hTable) {
        return GLOB_ERROR_INVALID_PARAMETERS;
    }
    
    /* Check if the table is not finalized yet */
    if (!hTable->bIsFinalized) {
        return GLOB_ERROR_INVALID_STATE;
    } 
    
    /* Call to the callback for each record */
    for (int nIndex = 0; nIndex < hTable->nUsedRecords; nIndex++) {
        eRetValue = pfCallback(hTable->patTable[nIndex].pszName,
                hTable->patTable[nIndex].nAddress,
                hTable->patTable[nIndex].bMarkedForExport,
                pvContext);
        if (eRetValue) {
            return eRetValue;
        }
    }
    return GLOB_SUCCESS;
}

/******************************************************************************
 * Name:    SYMTABLE_Free
 *****************************************************************************/
void SYMTABLE_Free(HSYMTABLE_TABLE hTable) {
    if (NULL == hTable) {
        return;
    }
    
    /* free the strings of the symbols name */
    for (int nIndex = 0; nIndex < hTable->nUsedRecords; nIndex++) {
        if (NULL != hTable->patTable[nIndex].pszName) {
            free(hTable->patTable[nIndex].pszName);
        }
    } 
    
    /* free the array and the main structure. */
    free(hTable->patTable);
    free(hTable);
}
