/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   parser.h
 * Author: doron276
 *
 * Created on 21 יולי 2018, 17:34
 */

#ifndef PARSER_H
#define PARSER_H

struct token_list ** parser_parse(const char *fileName, int* useRows);

// TO_DELETE
int parse_toToken(char *singleLine);
#endif /* PARSER_H */
