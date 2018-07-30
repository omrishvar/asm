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
#include "compiler.h"

#include "LINESTR.h"
#include "LEX.h"

int main(int argc, char *argv[]) { 
    FILE * ptFilePointer;
    char singleLine[81];
    /*int usedRows=0;
    struct token_list **pTokenListArray;*/
    
    
    ///////////////

    LINESTR_Open("sample");
     

    while (1) {
        PLINESTR_LINE ptLine = NULL;
        if (!LINESTR_GetNextLine(&ptLine))
        {
            
            break;
        }
        
        printf("%d\t%lu\t%s\n", ptLine->nLineNumber, strlen(ptLine->szLine), ptLine->szLine);
        LINESTR_FreeLine(ptLine);

    }
    return 0;
    ///////////////
    
    if(2 != argc) {
        printf("wrong number of arguments!\nUsage: %s <filename>\n", argv[0]);
        return 1;
    }
    if(FALSE == LEX_Open(argv[1])){
        printf("ERROR");
        return 1;
    }
    
    while(fgets(singleLine,81,ptFilePointer) != NULL) {
        puts(singleLine);
    }

    
    /*pTokenListArray=parser_parse(argv[1], &usedRows);
    
    compiler_compile(pTokenListArray,usedRows);*/
    
    return 0;
}

