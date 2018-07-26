/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   compiler.h
 * Author: doron276
 *
 * Created on 26 יולי 2018, 21:40
 */

#ifndef COMPILER_H
#define COMPILER_H

#include "line.h"

void compiler_compile(struct token_list ** pTokenListArray, int usedRows);


#endif /* COMPILER_H */
