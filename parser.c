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

void parse_parsePrintTokenList(struct token_list *x){
    printf("Token List:\n");
    printf("Source= %s\n",x->source_line);
    printf("Num of tokens= %d\n\n",x->numberOfTokens);
}

void parse_parsePrintToken(struct token x){
    printf("kind= %d\n", x.kind);
    printf("column= %d\n", x.column);
    printf("length= %d\n\n", x.length);
    
}

int parser_parseDirective(char *current, int column, struct token *x){
    int i=1;
    while(isalpha(current[i])){
        i++;
    }
    x->kind = 5;
    x->length = i;
    x->column = column;
    
    parse_parsePrintToken(*x);
    return i+1;
}

int parser_parseImmediateNum(char *current, int column, struct token *x){
    int i=1;
    if(current[i] != '-' || current[i] != '+' || isdigit(current[i])){
        i++;
        while(isdigit(current[i])){
            i++;
        }
    }
    x->kind = 4;
    x->length = i;
    x->column = column+1;
    
    parse_parsePrintToken(*x);
    return i+2;
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
    }else if(i == 2 && current[0] == 'r' && isdigit(current[1]) && current[1] != '8' && current[1] != '9'){ 
        //register
        x->kind = 5;
        x->length = i;
        x->column = column;
        
        parse_parsePrintToken(*x);
        return i;
    } else {
        // word
        x->kind = 0;
        x->length = i;
        x->column = column;
        
        parse_parsePrintToken(*x);
        return i;
    }
}



/*int parser_parseToToken(char *singleLine){
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
}*/

struct token_list* parser_parseToToken2(char *singleLine){
    char *current = singleLine; 
    struct token_list *pTokenList = malloc(sizeof *pTokenList);
    strcpy(pTokenList->source_line, singleLine);
    pTokenList->numberOfTokens=0;
    
    while (*current != '\n' && *current != '\0') {
        if (*current == ' ' || *current == '\t'){
            current++;
            continue;
        }
        if(*current ==';'){
            break;
        }
        if(strchr(",()", *current) != NULL) {
            pTokenList->tokens[pTokenList->numberOfTokens].kind = 6;
            pTokenList->tokens[pTokenList->numberOfTokens].length = 1;
            pTokenList->tokens[pTokenList->numberOfTokens].column = current-singleLine; 
            
            printf("SPECIAL %c\n", *current);
            parse_parsePrintToken(pTokenList->tokens[pTokenList->numberOfTokens]);
            
            current++;
            pTokenList->numberOfTokens++;
        }else if (*current =='.'){
            
            current += parser_parseDirective(current, current-singleLine+1, &pTokenList->tokens[pTokenList->numberOfTokens]);
            pTokenList->numberOfTokens++;
            
        }else if (*current =='#'){
            
            current += parser_parseImmediateNum(current, current-singleLine+1, &pTokenList->tokens[pTokenList->numberOfTokens]);
            pTokenList->numberOfTokens++;
            
        }else if(*current == '"'){
            
            current += parser_parseString(current, current-singleLine+1, &pTokenList->tokens[pTokenList->numberOfTokens]);
            pTokenList->numberOfTokens++;
            
        }else if(isdigit(*current) || *current == '-' || *current == '+'){
            current += parser_parseNumber(current, current-singleLine, &pTokenList->tokens[pTokenList->numberOfTokens]);
            pTokenList->numberOfTokens++;
            
        }else if(isalpha(*current)){
            current += parser_parseWordOrLabel(current, current-singleLine, &pTokenList->tokens[pTokenList->numberOfTokens]);
            pTokenList->numberOfTokens++;
            
        }else{
            printf("ERROR\n");
            break;
        }
    }
    return pTokenList;
}


struct token_list ** parser_parse(const char *fileName, int* usedRows){
    FILE *fPointer;
    char singleLine[81];
    struct token_list ** token_list_array =(struct token_list ** ) malloc(100* sizeof(*token_list_array));
    int i=0;
    fPointer = fopen(fileName,"r");
    
    if (fPointer == NULL) {
        printf("ERROR, The file is null\n");
        return NULL;
    }
    
    while(fgets(singleLine,81,fPointer) != NULL) {
        token_list_array[i]= parser_parseToToken2(singleLine); 
        parse_parsePrintTokenList(token_list_array[i]);
        i++;
    }
    *usedRows = i; 
    fclose(fPointer);
    return token_list_array;
}

