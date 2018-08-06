/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   helper.c
 * Author: boazgildor
 * 
 * Created on August 3, 2018, 9:17 PM
 */

#include <string.h>
#include "helper.h"

char * HELPER_ConcatStrings(const char * pszStr1, const char * pszStr2) {
    int nResultLength = 0;
    char * pszResult = NULL;
    nResultLength = strlen(pszStr1) + strlen(pszStr2) + 1;
    pszResult = malloc(nResultLength);
    if (NULL != pszResult) {
        strcpy(pszResult, pszStr1);
        strcat(pszResult, pszStr2);
    }
    return pszResult;
    
}    

int HELPER_FindInStringsArray(const char ** paszStringsArray, int nArrayElements, const char * pszStr, int nStrLength) {
    for (int i = 0; i < nArrayElements; i++) {
        if (strlen(paszStringsArray[i]) == nStrLength && 0 == strncmp(paszStringsArray[i], pszStr, nStrLength)) {
            return i;
        }
    }
    return -1;
}
