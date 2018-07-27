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

#include "symtable.h"
#include "global.h"
#include "compiler.h"


enum operand_type compiler_getOperandType(int kind) {
    switch (kind)
    {
        case 5: return directRegister;
        case 4: return immediate;
        case 0: return direct;
    }
}

void compiler_compile(struct token_list ** pTokenListArray, int usedRows){
    int IC = 0;
    int DC = 0;
    int i = 0;
    PSYMTABLE_TABLE symTable;
    BOOL X = SYMTABLE_Create(&symTable);
    
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
            currentToken++;
            L++;
            if (currentToken < pTokenListArray[i]->numberOfTokens) {
                // two operans
                // currentToken is comma
                currentToken++;
                pTokenListArray[i]->source_operand_type = pTokenListArray[i]->destination_operand_type;
                pTokenListArray[i]->destination_operand_type = compiler_getOperandType(pTokenListArray[i]->tokens[currentToken].kind);
                if (!(pTokenListArray[i]->source_operand_type == directRegister && 
                    pTokenListArray[i]->destination_operand_type == directRegister)){
                    L++;
                }   
            }
        } 
        IC = IC + L;
    }    
}
