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

BOOL SYMTABLE_Create(PSYMTABLE_TABLE *createdTable) {
    // allocate table
    PSYMTABLE_TABLE  table = NULL;
    table = (PSYMTABLE_TABLE)malloc(sizeof(SYMTABLE_TABLE));
    if (NULL == table) {
        // error
        printf("FATAL ERROR: malloc(%lu) failed", sizeof(SYMTABLE_TABLE));
        return FALSE;
    }
    table->table = (PSYMTABLE_RECORD)malloc(SYMTABLE_DEFAULT_TABLE_SIZE * sizeof(SYMTABLE_RECORD));
    if(NULL == table->table) {
        printf("FATAL ERROR: malloc(%lu) failed", SYMTABLE_DEFAULT_TABLE_SIZE * sizeof(SYMTABLE_RECORD));
        free(table);
        return FALSE;
    }
    table->allocatedRecords = SYMTABLE_DEFAULT_TABLE_SIZE;
    table->usedRecords = 0;
    table->isFinalized = FALSE;
    *createdTable = table;
    return TRUE;
}

int symtable_FindSymbol(PSYMTABLE_TABLE table, const char *name) {
    for (int i = 0; i < table->usedRecords; i++) {
        if (strcmp(name, table->table[i].name) == 0) {
            return i;
        }
    }
    return -1;
}
BOOL SYMTABLE_Insert(PSYMTABLE_TABLE table, const char *name, SYMTABLE_SYMTYPE type, int address, BOOL isExtern) {
    if (table->isFinalized) {
        printf("ERROR  CANT INSERT TO FINZALIZED TABLE\n");
        return FALSE;
    }
    // first we need to check the label isn't already exist
    if (-1 != symtable_FindSymbol(table, name)) {
        // TODO: add error handling
        printf("label %s already defined", name);
        return FALSE;
    }
    
    // check for free space
    if (table->usedRecords == table->allocatedRecords) {
        void *newTable = NULL;
        int newAllocatedRecords = SYMTABLE_ALLOCATION_FACTOR * table->allocatedRecords;
        // expand table
        newTable = realloc(table->table, newAllocatedRecords * sizeof(SYMTABLE_RECORD));
        if (newTable == NULL) {
            printf("FATAL ERROR: realloc(%lu) failed", newAllocatedRecords * sizeof(SYMTABLE_RECORD));
            return FALSE;
        }
        table->table = (PSYMTABLE_RECORD)newTable;
        table->allocatedRecords = newAllocatedRecords;
    }
    
    // insert the new record
    table->table[table->usedRecords].name = malloc(strlen(name) + 1);
    if (table->table[table->usedRecords].name == NULL) {
        printf("FATAL ERROR: malloc(%lu) failed", strlen(name) + 1);
        return FALSE;
    }
    strcpy(table->table[table->usedRecords].name, name);
    table->table[table->usedRecords].type = type;
    table->table[table->usedRecords].address = address;
    table->table[table->usedRecords].isExtern = isExtern;
    table->table[table->usedRecords].markedForExport = FALSE;
    table->usedRecords++;
    return TRUE;
}

BOOL SYMTABLE_Finalize(PSYMTABLE_TABLE table, int dataOffset) {
    if (table->isFinalized) {
        // already finalized;
        printf("ERROR TABLE ALREADY FINALIZED\n");
        return FALSE;
    }
    for (int i = 0; i < table->usedRecords; i++) {
        if (table->table[i].type == SYMTABLE_SYMTYPE_DATA
                && !table->table[i].isExtern) {
            table->table[i].address += dataOffset;
        }
    }
    table->isFinalized = TRUE;
    return TRUE;
}

BOOL SYMTABLE_MarkForExport(PSYMTABLE_TABLE table, const char *name) {
    int i = 0;
    if (!table->isFinalized) {
        printf("ERROR: SYMTABLE_MarkForExport should be called on finalized table\n");
        return FALSE;
    }
    i = symtable_FindSymbol(table, name);
    if (i == -1) {
        printf("ERROR: UNRECOGNIZED LABEL %s", name);
        return FALSE;
    }
    if (table->table[i].isExtern) {
        printf("ERROR: CANNOT EXPORT EXTERN LABEL %s", name);
        return FALSE;
    }
    table->table[i].markedForExport = TRUE;
    return TRUE;
}

BOOL SYMTABLE_GetSymbolInfo(PSYMTABLE_TABLE table, const char *name, SYMTABLE_SYMTYPE *type, int *address, BOOL *isExtern) {
    int i = 0;
    if (!table->isFinalized) {
        printf("ERROR: SYMTABLE_MarkForExport should be called on finalized table\n");
        return FALSE;
    }
    i = symtable_FindSymbol(table, name);
    if (i == -1) {
        printf("ERROR: UNRECOGNIZED LABEL %s", name);
        return FALSE;
    }
    *type = table->table[i].type;
    *address = table->table[i].address;
    *isExtern = table->table[i].isExtern;
    return TRUE;
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
