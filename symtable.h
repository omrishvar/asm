/******************************************************************************
 * File:    symtable.h
 * Author:  Doron Shvartztuch
 * The SYMTABLE module provides functions to create and manipulate
 * the symbols table.
 *****************************************************************************/

#ifndef SYMTABLE_H
#define SYMTABLE_H

/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include "global.h"

/******************************************************************************
 * TYPEDEFS
 *****************************************************************************/

/* The SYMTABLE_SYMTYPE enumerate contains the possible types of symbols. */
typedef enum SYMTABLE_SYMTYPE {
    SYMTABLE_SYMTYPE_CODE,
    SYMTABLE_SYMTYPE_DATA,
} SYMTABLE_SYMTYPE;

/* The SYMTABLE_FOREACH_CALLBACK is the prototype of the callback function
 * when enumerating the symbols marked for export.
 * See SYMTABLE_ForEach for more information.
 * Parameters:
 *          szSymbolName [IN] - the symbol name
 *          nAddress [IN] - the address of the symbol
 *          bIsMarkedForExport [IN] - whether the symbol is for export
 *          pvContext [IN] - the context passed to the enumerator
 * Return Value:
 *          GLOB_SUCCESS - to continue the enumeration.
 *          any other value - stop the enumeration */
typedef GLOB_ERROR (*SYMTABLE_FOREACH_CALLBACK)(const char *szSymbolName,
                                                int nAddress,
                                                BOOL bIsMarkedForExport,
                                                void * pvContext);

/* The SYMTABLE_TABLE represents a handle to symbols table. You can create
 * a new symbols table with SYMTABLE_Create. Always free the table at the end
 * with SYMTABLE_Free.  */
typedef struct SYMTABLE_TABLE SYMTABLE_TABLE, *HSYMTABLE_TABLE;

/******************************************************************************
 * FUNCTIONS
 *****************************************************************************/

/******************************************************************************
 * Name:    SYMTABLE_Create
 * Purpose: Creates a new symbols table
 * Parameters:
 *          phTable [OUT] - the handle to the created symbols table
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          In this case, a handle to the opened file is returned in phTable and
 *          the caller must free it with SYMTABLE_Free.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
GLOB_ERROR SYMTABLE_Create(HSYMTABLE_TABLE *phTable);

/******************************************************************************
 * Name:    SYMTABLE_Insert
 * Purpose: Inserts a symbol to the symbols table
 * Parameters:
 *          hTable [IN] - the handle to the symbols table
 *          pszName [IN] - the symbol name to insert
 *          eType [IN] - the type of the symbol. meaningless for extern symbols
 *          nAddress [IN] - the address of the symbol. 0 for extern symbols
 *          bIsExtern [IN] - whether the symbol is declared as extern.
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          GLOB_ERROR_INVALID_STATE - The table is already finalized.
 *          GLOB_ERROR_ALREADY_EXIST - The symbol is already exist in the table.
 *          If the function fails, an error code is returned.
 * Remarks:
 *          For symbols to export, you have to call both SYMTABLE_Insert and
 *          SYMTABLE_MarkForExport. You can call them in any order.
 *****************************************************************************/
GLOB_ERROR SYMTABLE_Insert(HSYMTABLE_TABLE hTable,
                           const char *pszName,
                           SYMTABLE_SYMTYPE eType,
                           int nAddress,
                           BOOL bIsExtern);

/******************************************************************************
 * Name:    SYMTABLE_MarkForExport
 * Purpose: Mark a symbol for export
 * Parameters:
 *          hTable [IN] - the handle to the symbols table
 *          pszName [IN] - the symbol name to export
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          GLOB_ERROR_INVALID_STATE - The table is already finalized.
 *          GLOB_ERROR_ALREADY_EXIST - The symbol is already exist in the table.
 *          GLOB_ERROR_EXPORT_AND_EXTERN - The symbol already declared
 *                                         as external. You can't mark it also
 *                                         for export.
 *          If the function fails, an error code is returned.
 * Remarks:
 *          For symbols to export, you have to call both SYMTABLE_Insert and
 *          SYMTABLE_MarkForExport. You can call them in any order.
 *****************************************************************************/
GLOB_ERROR SYMTABLE_MarkForExport(HSYMTABLE_TABLE hTable, const char *pszName);

/******************************************************************************
 * Name:    SYMTABLE_Finalize
 * Purpose: Finalize the table by updating the addresses to their final values
 *          and check all symbols are properly defined.
 * Parameters:
 *          hTable [IN] - the handle to the symbols table
 *          nDataOffset [IN] - the offset to add to all symbols of the data type
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          GLOB_ERROR_INVALID_STATE - The table is already finalized.
 *          GLOB_ERROR_NOT_FOUND - Some symbols marked for export, but have not
 *                                 been inserted to the table.
 *          If the function fails, an error code is returned.
 * Remarks:
 *          If the function fails, the content of the table may be invalid.
 *          Free the table with SYMTABNLE_Free.
 *****************************************************************************/
GLOB_ERROR SYMTABLE_Finalize(HSYMTABLE_TABLE hTable, int nDataOffset);

/******************************************************************************
 * Name:    SYMTABLE_GetSymbolInfo
 * Purpose: Gets information about a symbol
 * Parameters:
 *          hTable [IN] - the handle to the symbols table
 *          pszName [IN] - the symbol name to read
 *          pnAddress [OUT] - the address of the symbol
 *          pbIsExtern [OUT] - whether the symbol is declared as extern.
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned. the out
 *          parameters will be filled with information about the symbol.
 *          GLOB_ERROR_INVALID_STATE - The table is not finalized.
 *          GLOB_ERROR_NOT_FOUND - The symbol is not found in the table
 *          If the function fails, an error code is returned.
 * Remarks:
 *          For symbols to export, you have to call both SYMTABLE_Insert and
 *          SYMTABLE_MarkForExport. You can call them in any order.
 *****************************************************************************/
GLOB_ERROR SYMTABLE_GetSymbolInfo(HSYMTABLE_TABLE hTable,
                                  const char *pszName,
                                  int *pnAddress,
                                  BOOL *pbIsExtern);

/******************************************************************************
 * Name:    SYMTABLE_ForEach
 * Purpose: Enumerate all symbols marked for export
 * Parameters:
 *          hTable [IN] - the handle to the symbols table
 *          pfCallback [IN] - pointer to the callback function to use
 *          pvContext [IN] - context to pass to the callback function
 * Return Value:
 *          Upon successful completion, GLOB_SUCCESS is returned.
 *          If the callback function returns value other than GLOB_SUCCESS,
 *          the same value will be returned.
 *          GLOB_ERROR_INVALID_STATE - The table is not finalized.
 *          If the function fails, an error code is returned.
 *****************************************************************************/
GLOB_ERROR SYMTABLE_ForEach(HSYMTABLE_TABLE hTable,
                            SYMTABLE_FOREACH_CALLBACK pfCallback,
                            void * pvContext);

/******************************************************************************
 * Name:    SYMTABLE_Free
 * Purpose: Frees a table created previously with SYMTABLE_Create
 * Parameters:
 *          hTable [IN] - the handle to the symbols table
 *****************************************************************************/
void SYMTABLE_Free(HSYMTABLE_TABLE hTable);

#endif /* SYMTABLE_H */
