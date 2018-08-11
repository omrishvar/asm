/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   helper.h
 * Author: boazgildor
 *
 * Created on August 3, 2018, 9:17 PM
 */

#ifndef HELPER_H
#define HELPER_H

#define MAX(a,b)  ((a) > (b) ? (a) : (b))
#define ARRAY_ELEMENTS(arr)        ((sizeof((arr))/sizeof((arr)[0])))

char * HELPER_ConcatStrings(const char * pszStr1, const char * pszStr2);
int HELPER_FindInStringsArray(const char ** paszStringsArray, int nArrayElements, const char * pszStr, int nStrLength);
#endif /* HELPER_H */
