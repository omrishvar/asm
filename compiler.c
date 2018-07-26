/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   compiler.c
 * Author: doron276
 * 
 * Created on 26 יולי 2018, 21:40
 */

#include "symtable.h"
#include "global.h"
#include "compiler.h"


void compiler_compile(struct token_list ** pTokenListArray, int usedRows){
    //BOOL SYMTABLE_Insert(PSYMTABLE_TABLE table, const char *name, int len, SYMTABLE_SYMTYPE type, int address, BOOL isExtern);
    int IC = 0;
    int DC = 0;
    int i = 0;
    PSYMTABLE_TABLE symTable;
    BOOL X = SYMTABLE_Create(&symTable);
    for(;i<usedRows;i++){
        if(pTokenListArray[i]->tokens[0].kind == 1){
            SYMTABLE_Insert(symTable, pTokenListArray[i]->source_line, pTokenListArray[i]->tokens[0].length, SYMTABLE_SYMTYPE_CODE, IC, FALSE);
            
        }
        IC++;  
    }
    
    
    
}
