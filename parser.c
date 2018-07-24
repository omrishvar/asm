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

int parse_isTrashLine(char *singleLine){
    int i = 0;
    while (1) {
        if(singleLine[i] == '\n' || singleLine[i] == ';'){
            return 1;
        }
        else if(singleLine[i] != ' ' && singleLine[i] != '\t'){
            return 0;
        }
        i++;
    }
}

void parse_print_n_chars(char * str, int n)
{
    char t = str[n];
    str[n] = '\0';
    printf("%s\n", str);
    str[n] = t;
}


void parse_printToken(struct token x){
    printf("kind= %d\n", x.kind);
    printf("column= %d\n", x.column);
    printf("length= %d\n", x.length);
    
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

        parse_printToken(*x);
        //parse_print_n_chars(current, i);
        return i+1;
    } else {
        // word
        x->kind = 0;
        x->length = i;
        x->column = column;
        
        parse_printToken(*x);
        //parse_print_n_chars(current, i);
        return i;
    }
}


int parse_toToken(char *singleLine){
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
            printf("SPECIAL %c\n", *current);
            current++;
            tokenList.numberOfTokens++;
        }else if(*current == '"'){
            int i=1;
            while(current[i] != '"'){
                i++;
            }
            parse_print_n_chars(current+1, i-1);
            current += i+1;
            tokenList.numberOfTokens++;
        }else if(isdigit(*current) || *current == '-' || *current == '+'){
            int i=1;
            while(isdigit(current[i])){
                i++;   
            }
            parse_print_n_chars(current, i);
            current += i;
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
        if(0 == parse_isTrashLine(singleLine)) {
            parse_toToken(singleLine);
            
            //puts(singleLine);
        }
    }
    fclose(fPointer);
    return 0;
}

