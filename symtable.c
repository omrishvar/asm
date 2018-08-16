/* 
 * File:   symtable.c
 * Author: doron276
 * 
 * Created on 23 יולי 2018, 21:44
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtable.h"

#define SYMTABLE_DEFAULT_TABLE_SIZE 1
#define SYMTABLE_ALLOCATION_FACTOR 2

typedef struct SYMTABLE_RECORD {
    char * name;
    SYMTABLE_SYMTYPE type;
    int address;
    BOOL isExtern;
    BOOL markedForExport;
} SYMTABLE_RECORD, *PSYMTABLE_RECORD;

struct SYMTABLE_TABLE {
    BOOL isFinalized;
    int allocatedRecords;
    int usedRecords;
    PSYMTABLE_RECORD table;
};

GLOB_ERROR SYMTABLE_Create(PSYMTABLE_TABLE *createdTable) {
    // allocate table
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    PSYMTABLE_TABLE  table = NULL;
    table = (PSYMTABLE_TABLE)malloc(sizeof(SYMTABLE_TABLE));
    if (NULL == table) {
        return GLOB_ERROR_SYS_CALL_ERROR();
    }
    table->table = (PSYMTABLE_RECORD)malloc(SYMTABLE_DEFAULT_TABLE_SIZE * sizeof(SYMTABLE_RECORD));
    if(NULL == table->table) {
        eRetValue = GLOB_ERROR_SYS_CALL_ERROR();
        free(table);
        return eRetValue;
    }
    table->allocatedRecords = SYMTABLE_DEFAULT_TABLE_SIZE;
    table->usedRecords = 0;
    table->isFinalized = FALSE;
    *createdTable = table;
    return GLOB_SUCCESS;
}

int symtable_FindSymbol(PSYMTABLE_TABLE table, const char *name) {
    for (int i = 0; i < table->usedRecords; i++) {
        if (strcmp(name, table->table[i].name) == 0) {
            return i;
        }
    }
    return -1;
}

GLOB_ERROR symtable_InsertRecord(PSYMTABLE_TABLE table, const char *name, SYMTABLE_SYMTYPE type, int address, BOOL isExtern, BOOL markedForExport) {
    // check for free space
    if (table->usedRecords == table->allocatedRecords) {
        void *newTable = NULL;
        int newAllocatedRecords = SYMTABLE_ALLOCATION_FACTOR * table->allocatedRecords;
        // expand table
        newTable = realloc(table->table, newAllocatedRecords * sizeof(SYMTABLE_RECORD));
        if (newTable == NULL) {
            return GLOB_ERROR_SYS_CALL_ERROR();
        }
        table->table = (PSYMTABLE_RECORD)newTable;
        table->allocatedRecords = newAllocatedRecords;
    }
    
    // insert the new record
    table->table[table->usedRecords].name = malloc(strlen(name) + 1);
    if (table->table[table->usedRecords].name == NULL) {
        return GLOB_ERROR_SYS_CALL_ERROR();
    }
    strcpy(table->table[table->usedRecords].name, name);
    table->table[table->usedRecords].type = type;
    table->table[table->usedRecords].address = address;
    table->table[table->usedRecords].isExtern = isExtern;
    table->table[table->usedRecords].markedForExport = markedForExport;
    table->usedRecords++;
    return GLOB_SUCCESS;
}
GLOB_ERROR SYMTABLE_Insert(PSYMTABLE_TABLE table, const char *name, SYMTABLE_SYMTYPE type, int address, BOOL isExtern) {
    int nIndex = 0;
    if (table->isFinalized) {
        return GLOB_ERROR_INVALID_STATE;
    }
    
    nIndex = symtable_FindSymbol(table, name);
    // first we need to check the label isn't already exist
    if (-1 != nIndex &&
            (table->table[nIndex].isExtern || 0 != table->table[nIndex].address)) {
        return GLOB_ERROR_ALREADY_EXIST;
    }
    
    if (-1 != nIndex) {
        // Already defined for export. update the record
        table->table[nIndex].type = type;
        table->table[nIndex].address = address;
        return GLOB_SUCCESS;
    }
    return symtable_InsertRecord(table, name, type, address, isExtern, FALSE);
}

GLOB_ERROR SYMTABLE_Finalize(PSYMTABLE_TABLE table, int dataOffset) {
    if (table->isFinalized) {
        return GLOB_ERROR_INVALID_STATE;
    }
    for (int i = 0; i < table->usedRecords; i++) {
        if (table->table[i].type == SYMTABLE_SYMTYPE_DATA
                && !table->table[i].isExtern) {
            table->table[i].address += dataOffset;
        }
    }
    table->isFinalized = TRUE;
    return GLOB_SUCCESS;    
}

GLOB_ERROR SYMTABLE_MarkForExport(PSYMTABLE_TABLE table, const char *name) {
    int i = 0;
    if (table->isFinalized) {
        return GLOB_ERROR_INVALID_STATE;
    }
    i = symtable_FindSymbol(table, name);
    if (i == -1) {
        return symtable_InsertRecord(table, name, SYMTABLE_SYMTYPE_CODE, 0, FALSE, TRUE);
    }
    if (table->table[i].isExtern) {
        return GLOB_ERROR_EXPORT_AND_EXTERN;
    }
    table->table[i].markedForExport = TRUE;
    return GLOB_SUCCESS;
}

GLOB_ERROR SYMTABLE_GetSymbolInfo(PSYMTABLE_TABLE table, const char *name, int *address, BOOL *isExtern) {
    int i = 0;
    if (!table->isFinalized) {
        return GLOB_ERROR_INVALID_STATE;
    }
    i = symtable_FindSymbol(table, name);
    if (i == -1) {
        return GLOB_ERROR_NOT_FOUND;
    }
    *address = table->table[i].address;
    *isExtern = table->table[i].isExtern;
    return GLOB_SUCCESS;
}

GLOB_ERROR SYMTABLE_ForEachExport(PSYMTABLE_TABLE table, SYMTABLE_FOREACH_CALLBACK callback, void * context) {
    GLOB_ERROR eRetValue = GLOB_ERROR_UNKNOWN;
    if (!table->isFinalized) {
        return GLOB_ERROR_INVALID_STATE;
    }
    
    for (int i = 0; i < table->usedRecords; i++) {
        if (table->table[i].markedForExport) {
            eRetValue = callback(table->table[i].name, table->table[i].address, context);
            if (eRetValue) {
                return eRetValue;
            }
        }
    }
    return GLOB_SUCCESS;
}

void SYMTABLE_Free(PSYMTABLE_TABLE table) {
    if (table == NULL) {
        return;
    }
    for (int i = 0; i < table->usedRecords; i++) {
        if (NULL != table->table[i].name) {
            free(table->table[i].name);
        }
    } 
    free(table->table);
    free(table);
}
