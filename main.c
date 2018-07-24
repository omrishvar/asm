/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.c
 * Author: doron276
 * Created on 21 יולי 2018, 12:41
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "global.h"
#include "parser.h"
#include "symtable.h"
void symbolSample()
{
    // create a table
    PSYMTABLE_TABLE table = NULL;
    SYMTABLE_SYMTYPE type;
    int address = 0;
    BOOL isExtern = FALSE;
    if (!SYMTABLE_Create(&table)) {
        printf("Create Failed\n");
    }
    if (!SYMTABLE_Insert(table, "LabelA", SYMTABLE_SYMTYPE_CODE, 100, FALSE)) {
        printf("INSERT FAILED\n");
        return;
    }
    if (!SYMTABLE_Insert(table, "LabelB", SYMTABLE_SYMTYPE_CODE, 200, FALSE)) {
        printf("INSERT FAILED\n");
        return;
    }
    if (!SYMTABLE_Insert(table, "LabelC", SYMTABLE_SYMTYPE_CODE, 0, TRUE)) {
        printf("INSERT FAILED\n");
        return;
    }
    if (!SYMTABLE_Insert(table, "LabelD", SYMTABLE_SYMTYPE_DATA, 0, FALSE)) {
        printf("INSERT FAILED\n");
        return;
    }
    if (!SYMTABLE_Finalize(table, 1000)) {
        printf("SYMTABLE_Finalize FAILED\n");
        return;
    }
    if (!SYMTABLE_MarkForExport(table, "LabelB")) {
        printf("SYMTABLE_MarkForExport FAILED\n");
        return;
    }
    
    SYMTABLE_GetSymbolInfo(table, "LabelA", &type, &address, &isExtern);
    SYMTABLE_GetSymbolInfo(table, "LabelB", &type, &address, &isExtern);
    SYMTABLE_GetSymbolInfo(table, "LabelC", &type, &address, &isExtern);
    SYMTABLE_GetSymbolInfo(table, "LabelD", &type, &address, &isExtern);
    SYMTABLE_Free(table);


}

int main(int argc, char *argv[]) { 
    
    /*char * x = malloc(200);
    strcpy(x, "abc,#,-778,9\n");
    parse_toToken(x);*/
    symbolSample(); /////////TO DELETE
    if(argc != 2) {
        printf("wrong number of arguments!\nUsage: %s <filename>\n", argv[0]);
        return 1;
    }
    
    if(0 != parser_parse(argv[1])){
            return 1;
    }
    
    return 0;
}

