/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   line.h
 * Author: doron276
 *
 * Created on 22 יולי 2018, 11:38
 */

#ifndef LINE_H
#define LINE_H

enum operand_type {
    immediate = 0,
    direct = 1,
    jumpParam = 2,
    directRegister = 3,
    invalid = 4
};


enum opcode {
    MOV = 0x1000,
    CMP = 0x1001,
    ADD = 0x1002,
    SUB = 0x1003,
    NOT = 0x1004,
    CLR = 0x1005,
    LEA = 0x1006,
    INC = 0x1007,
    DEC = 0x1008,
    JMP = 0x1009,
    BNE = 0x100A,
    RED = 0x100B,
    PRN = 0x100C,
    JSR = 0x100D,
    RTS = 0x100E,
    STOP = 0x100F
};

struct token {
    int kind;
    int column;
    int length;
};
struct token_list {
    char source_line[81];
    struct token tokens[40];
    int numberOfTokens;
    enum opcode opcode;
    enum operand_type source_operand_type;
    enum operand_type destination_operand_type;
    int source_operand_index;
    int destination_operand_index;
    int first_word;
    int second_word;
    int third_word;
    int L;
};


struct line {
    char label[32];
    char opcode[4];
    char parameters[80];
};

#endif /* LINE_H */

