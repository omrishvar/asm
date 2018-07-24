/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   parser.c
 * Author: doron276
 * 
 * Created on 21 יולי 2018, 17:34
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "parser.h"
#include "line.h"

void parse_parsePrintToken(struct token x){
    printf("kind= %d\n", x.kind);
    printf("column= %d\n", x.column);
    printf("length= %d\n\n", x.length);
    
}

int parser_parseString(char *current, int column, struct token *x){
    int i=1;
    while(current[i] != '"'){
        i++;
    }
    x->kind = 3;
    x->length = i-1;
    x->column = column;
    
    parse_parsePrintToken(*x);
    return i+1;
}

int parser_parseNumber(char *current, int column, struct token *x){
    int i=1;
    while(isdigit(current[i])){
        i++;   
    }
    x->kind = 2;
    x->length = i;
    x->column = column;
    
    parse_parsePrintToken(*x);
    return i;
}

int parser_parseWordOrLabel(char *current, int column, struct token *x){
    int i=1;
    while(isalpha(current[i]) || isdigit(current[i])){
        i++;
    }
    if (current[i] == ':') {
        // label
        x->kind = 1;
        x->length = i;
        x->column = column;

        parse_parsePrintToken(*x);
        return i+1;
    } else {
        // word
        x->kind = 0;
        x->length = i;
        x->column = column;
        
        parse_parsePrintToken(*x);
        return i;
    }
}// if it's mistake? DELETE!!!



int parser_parseToToken(char *singleLine){
    char *current = singleLine;
    struct token_list tokenList;
    strcpy(tokenList.source_line, singleLine);
    tokenList.numberOfTokens=0;
    
    while (*current != '\n') {
        if (*current == ' ' || *current == '\t'){
            current++;
            continue;
        }
        if(*current ==';'){
            break;
        }
        if(strchr(",.#()", *current) != NULL) {
            tokenList.tokens[tokenList.numberOfTokens].kind = 4;
            tokenList.tokens[tokenList.numberOfTokens].length = 1;
            tokenList.tokens[tokenList.numberOfTokens].column = current-singleLine; 
            
            printf("SPECIAL %c\n", *current);
            parse_parsePrintToken(tokenList.tokens[tokenList.numberOfTokens]);
            
            current++;
            tokenList.numberOfTokens++;
            
        }else if(*current == '"'){
            
            current += parser_parseString(current, current-singleLine+1, &tokenList.tokens[tokenList.numberOfTokens]);
            tokenList.numberOfTokens++;
            
        }else if(isdigit(*current) || *current == '-' || *current == '+'){
            current += parser_parseNumber(current, current-singleLine, &tokenList.tokens[tokenList.numberOfTokens] );
            tokenList.numberOfTokens++;
            
        }else if(isalpha(*current)){
            current += parser_parseWordOrLabel(current, current-singleLine, &tokenList.tokens[tokenList.numberOfTokens]);
            tokenList.numberOfTokens++;
            
        }else{
            printf("ERROR\n");
            break;
        }
    }
    return 0;
}


int parser_parse(const char *fileName){
    FILE *fPointer;
    char singleLine[81];
    fPointer = fopen(fileName,"r");
    
    if (fPointer == NULL) {
        printf("ERROR, The file is null\n");
        return 1;
    }
    
    while(fgets(singleLine,81,fPointer) != NULL) {
        parser_parseToToken(singleLine);
    }
    fclose(fPointer);
    return 0;
}

