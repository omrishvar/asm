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

typedef struct SYMTABLE_TABLE SYMTABLE_TABLE, *PSYMTABLE_TABLE;
BOOL SYMTABLE_Create(PSYMTABLE_TABLE *createdTable);
BOOL SYMTABLE_Insert(PSYMTABLE_TABLE table, const char *name, int len, SYMTABLE_SYMTYPE type, int address, BOOL isExtern);
BOOL SYMTABLE_Finalize(PSYMTABLE_TABLE table, int dataOffset);
BOOL SYMTABLE_MarkForExport(PSYMTABLE_TABLE table, const char *name);
BOOL SYMTABLE_GetSymbolInfo(PSYMTABLE_TABLE table, const char *name, int length, SYMTABLE_SYMTYPE *type, int *address, BOOL *isExtern);
void SYMTABLE_Free(PSYMTABLE_TABLE table);
#endif /* SYMTABLE_H */
