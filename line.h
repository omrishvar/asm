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

struct token {
    int kind;
    int column;
    int length;
};
struct token_list {
    char source_line[81];
    struct token tokens[40];
    int numberOfTokens;
};


struct line {
    char label[32];
    char opcode[4];
    char parameters[80];
};

#endif /* LINE_H */

