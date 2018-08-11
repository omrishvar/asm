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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "symtable.h"
#include "global.h"
#include "compiler.h"

#if 0
/* Function to convert a decinal number to binary number */
void decimalToBinary(int n) {
    char x[15] = { 0 };
    for (int i = 13; i >= 0; i--) {
        if (n & 1) {
            x[i] = '1';
        } else {
            x[i] = '0';
        }
        n >>= 1;
    }
    printf("%s", x);
}


enum operand_type compiler_getOperandType(int kind) {
    switch (kind)
    {
        case 5: return directRegister;
        case 4: return immediate;
        case 0: return direct;
        default: return invalid;
    }
    
}
int compiler_compileEncodeAnotherWord(struct token_list* pTokenList, enum operand_type source, enum operand_type dest, PSYMTABLE_TABLE symTable, int L, int* secondWord, int* thirdWord){
    int destNum = 0;
    int sourceNum = 0;
    SYMTABLE_SYMTYPE type;
    int address;
    BOOL isExtern;
    if(L == 3){
        if(source == immediate){
            sourceNum = atoi(pTokenList->source_line+pTokenList->tokens[pTokenList->source_operand_index].column);
            *secondWord = (sourceNum<<2);
            if(dest == directRegister){
                destNum = atoi(pTokenList->source_line+pTokenList->tokens[pTokenList->destination_operand_index].column+1);
                *thirdWord = (destNum<<2);
            }if(dest == direct){
                SYMTABLE_GetSymbolInfo(symTable, pTokenList->source_line+pTokenList->tokens[pTokenList->destination_operand_index].column,
                    pTokenList->tokens[pTokenList->destination_operand_index].length, &type, &address, &isExtern);
                *thirdWord = ((address<<2) | 2);
            }
            
        }else if(source == direct && dest == directRegister){
            SYMTABLE_GetSymbolInfo(symTable, pTokenList->source_line+pTokenList->tokens[pTokenList->source_operand_index].column,
                    pTokenList->tokens[pTokenList->destination_operand_index].length, &type, &address, &isExtern);
            *secondWord = ((address<<2) | 2);
            
            destNum = atoi(pTokenList->source_line+pTokenList->tokens[pTokenList->destination_operand_index].column+1);
            *thirdWord = (destNum<<2);
                
        }else if(source == directRegister && dest == direct){
            sourceNum = atoi(pTokenList->source_line+pTokenList->tokens[pTokenList->source_operand_index].column+1);
            *secondWord = (sourceNum<<2);
            
            SYMTABLE_GetSymbolInfo(symTable, pTokenList->source_line+pTokenList->tokens[pTokenList->destination_operand_index].column,
                    pTokenList->tokens[pTokenList->destination_operand_index].length, &type, &address, &isExtern);
            *thirdWord = ((address<<2) | 2);
            
        }


    }else if(L == 4){
        return 0;
    }
}
int compiler_compileEncodeFirstWord(enum opcode opcode, enum operand_type source, enum operand_type dest) {
    return ((opcode & 0xF)<<6) | ((source & 3)<<4) | ((dest & 3)<<2);   
}
void compiler_compileEncodeSecondWord(struct token_list* pTokenList, enum operand_type source, enum operand_type dest, PSYMTABLE_TABLE symTable){
    int destNum = 0;
    int sourceNum = 0;
    if(source == invalid){ // one operand
        if(dest == immediate){
            destNum = atoi(pTokenList->source_line+pTokenList->tokens[pTokenList->destination_operand_index].column);
            pTokenList->second_word = (destNum<<2);
            return;
        }
        if(dest == direct){
            SYMTABLE_SYMTYPE type;
            int address;
            BOOL isExtern;
            SYMTABLE_GetSymbolInfo(symTable, pTokenList->source_line+pTokenList->tokens[pTokenList->destination_operand_index].column,
                    pTokenList->tokens[pTokenList->destination_operand_index].length, &type, &address, &isExtern);
            pTokenList->second_word = ((address<<2) | 2);
            return;
        }
        if(dest == directRegister){
            destNum = atoi(pTokenList->source_line+pTokenList->tokens[pTokenList->destination_operand_index].column+1);
            pTokenList->second_word = (destNum<<2);
            return;
        }
    //two opreand:
    }else if(source == directRegister && dest == directRegister){ 
        sourceNum = atoi(pTokenList->source_line+pTokenList->tokens[pTokenList->source_operand_index].column+1);
        destNum = atoi(pTokenList->source_line+pTokenList->tokens[pTokenList->destination_operand_index].column+1);
        pTokenList->second_word =((sourceNum<<8) | (destNum<<2));
        return;

    }else{
        compiler_compileEncodeAnotherWord(pTokenList, source, dest, symTable, pTokenList->L, &pTokenList->second_word, &pTokenList->third_word);
    }

}

int compiler_compileFirstPhase(struct token_list ** pTokenListArray, int usedRows, int * DC, PSYMTABLE_TABLE symTable){
    int IC = 100;
    int i = 0;
    for(;i<usedRows;i++){
        int L=1; // number of words
        int currentToken = 0;
        if(pTokenListArray[i]->tokens[currentToken].kind == 1){ // if label
            SYMTABLE_Insert(symTable, pTokenListArray[i]->source_line, pTokenListArray[i]->tokens[currentToken].length, SYMTABLE_SYMTYPE_CODE, IC, FALSE);
            currentToken++;
        }
        // opcode should be in currentToken
        if(pTokenListArray[i]->tokens[currentToken].kind == 0){
            char * opcodeString =pTokenListArray[i]->source_line+pTokenListArray[i]->tokens[currentToken].column;
            int opcodeLen = pTokenListArray[i]->tokens[currentToken].length;
            enum opcode opcode;
            
            if(0 == strncmp(opcodeString,"mov", 3) && 3 == opcodeLen) opcode=MOV;
            if(0 == strncmp(opcodeString,"cmp", 3) && 3 == opcodeLen) opcode=CMP;
            if(0 == strncmp(opcodeString,"add", 3) && 3 == opcodeLen) opcode=ADD;
            if(0 == strncmp(opcodeString,"sub", 3) && 3 == opcodeLen) opcode=SUB;
            if(0 == strncmp(opcodeString,"not", 3) && 3 == opcodeLen) opcode=NOT;
            if(0 == strncmp(opcodeString,"clr", 3) && 3 == opcodeLen) opcode=CLR;
            if(0 == strncmp(opcodeString,"lea", 3) && 3 == opcodeLen) opcode=LEA;
            if(0 == strncmp(opcodeString,"inc", 3) && 3 == opcodeLen) opcode=INC;
            if(0 == strncmp(opcodeString,"dec", 3) && 3 == opcodeLen) opcode=DEC;
            if(0 == strncmp(opcodeString,"jmp", 3) && 3 == opcodeLen) opcode=JMP;
            if(0 == strncmp(opcodeString,"bne", 3) && 3 == opcodeLen) opcode=BNE;
            if(0 == strncmp(opcodeString,"red", 3) && 3 == opcodeLen) opcode=RED;
            if(0 == strncmp(opcodeString,"prn", 3) && 3 == opcodeLen) opcode=PRN;
            if(0 == strncmp(opcodeString,"jsr", 3) && 3 == opcodeLen) opcode=JSR;
            if(0 == strncmp(opcodeString,"rts", 3) && 3 == opcodeLen) opcode=RTS;
            if(0 == strncmp(opcodeString,"stop", 4) && 4 == opcodeLen) opcode=STOP;
            
            pTokenListArray[i]->opcode = opcode;
            currentToken++;
        }
        
        pTokenListArray[i]->source_operand_type = invalid;
        pTokenListArray[i]->destination_operand_type = invalid;

        if(currentToken < pTokenListArray[i]->numberOfTokens){
            pTokenListArray[i]->destination_operand_type = compiler_getOperandType(pTokenListArray[i]->tokens[currentToken].kind);
            pTokenListArray[i]->destination_operand_index = currentToken;
            currentToken++;
            L++;
            if (currentToken < pTokenListArray[i]->numberOfTokens) {
                // two operans
                // currentToken is comma
                currentToken++;
                pTokenListArray[i]->source_operand_type = pTokenListArray[i]->destination_operand_type;
                pTokenListArray[i]->source_operand_index = pTokenListArray[i]->destination_operand_index;
                pTokenListArray[i]->destination_operand_type = compiler_getOperandType(pTokenListArray[i]->tokens[currentToken].kind);
                pTokenListArray[i]->destination_operand_index = currentToken;

                if (!(pTokenListArray[i]->source_operand_type == directRegister && 
                    pTokenListArray[i]->destination_operand_type == directRegister)){
                    L++;
                }   
            }
        }
        
        // now we can encode the first word
        pTokenListArray[i]->first_word = compiler_compileEncodeFirstWord(pTokenListArray[i]->opcode, pTokenListArray[i]->source_operand_type, pTokenListArray[i]->destination_operand_type);
        pTokenListArray[i]->L = L;
        IC = IC + L;
    }
    *DC = 0;
    return IC;
}

int compiler_compileSecondPhase(struct token_list ** pTokenListArray, int usedRows, PSYMTABLE_TABLE symTable){
    for(int i=0; i<usedRows;i++){
         compiler_compileEncodeSecondWord(pTokenListArray[i], pTokenListArray[i]->source_operand_type, pTokenListArray[i]->destination_operand_type, symTable);
        //printf("first: %s\n",decimalToBinary(pTokenListArray[i]->first_word));
         printf("first: ");
        decimalToBinary(pTokenListArray[i]->first_word);
        printf("\nsecond:");
        decimalToBinary(pTokenListArray[i]->second_word);
        printf("\nthird: ");
        decimalToBinary(pTokenListArray[i]->third_word);
        printf("\n");
    }
    return 0;
}

    

void compiler_compile(struct token_list ** pTokenListArray, int usedRows){
    
    int DC = 0;
    int IC = 0;
    
    PSYMTABLE_TABLE symTable;
    BOOL X = SYMTABLE_Create(&symTable);
    
    IC = compiler_compileFirstPhase(pTokenListArray, usedRows, &DC, symTable);
    SYMTABLE_Finalize(symTable, IC);
    
    compiler_compileSecondPhase(pTokenListArray, usedRows, symTable);
 
}    
#endif
int x;