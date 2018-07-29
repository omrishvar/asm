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

int main(int argc, char *argv[]) { 
    int usedRows=0;
    struct token_list **pTokenListArray;
    
    if(argc != 2) {
        printf("wrong number of arguments!\nUsage: %s <filename>\n", argv[0]);
        return 1;
    }
    pTokenListArray=parser_parse(argv[1], &usedRows);
    
    compiler_compile(pTokenListArray,usedRows);
    
    return 0;
}

