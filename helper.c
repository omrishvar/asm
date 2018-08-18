/******************************************************************************
 * File:    helper.c
 * Author:  Doron Shvartztuch
 * The HELPER module has some non project-specific declarations and functions,
 * so we can reuse them in other projects
 *****************************************************************************/


/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include <stdlib.h>
#include <string.h>
#include "helper.h"

/******************************************************************************
 * EXTERNAL FUNCTIONS
 * ------------------
 * See function-level documentation in the header file
 *****************************************************************************/

/******************************************************************************
 * Name:    HELPER_ConcatStrings
 *****************************************************************************/
char * HELPER_ConcatStrings(const char * pszStr1, const char * pszStr2) {
    int nResultLength = 0;
    char * pszResult = NULL;
    
    /* Calculate the length of the result, +1 for the '\0' */
    nResultLength = strlen(pszStr1) + strlen(pszStr2) + 1;
    
    /* Allocate memory */
    pszResult = malloc(nResultLength);
    if (NULL != pszResult) {
        /* Copy the strings */
        strcpy(pszResult, pszStr1);
        strcat(pszResult, pszStr2);
    }
    return pszResult;
}    

/******************************************************************************
 * Name:    HELPER_FindInStringsArray
 *****************************************************************************/
int HELPER_FindInStringsArray(const char ** paszStringsArray,
                              int nArrayElements,
                              const char * pszStr,
                              int nStrLength) {
    /* go over the array */
    for (int nIndex = 0; nIndex < nArrayElements; nIndex++) {
        /* Check if we found the string.
         * Remark: we compare only the first nStrLength chars from pszStr. */
        if (strlen(paszStringsArray[nIndex]) == nStrLength
                && 0 == strncmp(paszStringsArray[nIndex], pszStr, nStrLength)) {
            return nIndex;
        }
    }
    
    /* Not found */
    return -1;
}
