/* 
 * File:   symtable.h
 * Author: doron276
 *
 * Created on 23 יולי 2018, 21:44
 */

#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "global.h"

typedef enum SYMTABLE_SYMTYPE {
    SYMTABLE_SYMTYPE_CODE,
    SYMTABLE_SYMTYPE_DATA,
} SYMTABLE_SYMTYPE;

typedef GLOB_ERROR (*SYMTABLE_FOREACH_CALLBACK)(const char *name, int address, void * context);

typedef struct SYMTABLE_TABLE SYMTABLE_TABLE, *PSYMTABLE_TABLE;
GLOB_ERROR SYMTABLE_Create(PSYMTABLE_TABLE *createdTable);
GLOB_ERROR SYMTABLE_Insert(PSYMTABLE_TABLE table, const char *name, SYMTABLE_SYMTYPE type, int address, BOOL isExtern);
GLOB_ERROR SYMTABLE_MarkForExport(PSYMTABLE_TABLE table, const char *name);
GLOB_ERROR SYMTABLE_Finalize(PSYMTABLE_TABLE table, int dataOffset);
GLOB_ERROR SYMTABLE_GetSymbolInfo(PSYMTABLE_TABLE table, const char *name, int *address, BOOL *isExtern);
GLOB_ERROR SYMTABLE_ForEachExport(PSYMTABLE_TABLE table, SYMTABLE_FOREACH_CALLBACK callback, void * context);

void SYMTABLE_Free(PSYMTABLE_TABLE table);
#endif /* SYMTABLE_H */
