/******************************************************************************
 * File:    helper.h
 * Author:  Doron Shvartztuch
 * The HELPER module has some non project-specific declarations and functions,
 * so we can reuse them in other projects
 *****************************************************************************/
#ifndef HELPER_H
#define HELPER_H

/******************************************************************************
 * CONSTANTS & MACROS
 *****************************************************************************/
#define FALSE                  0
#define TRUE                   1

/* Warning: double evaluation when using the MAX macro */
#define MAX(a,b)               ((a) > (b) ? (a) : (b))

/* The ARRAY_ELEMENTS returns the number of elements in the array.
 * Warning: the macro uses the sizeof operator and works only on static arrays*/
#define ARRAY_ELEMENTS(arr)    ((sizeof((arr))/sizeof((arr)[0])))

/* Set the last character of static string to '\0'. */
#define TERMINATE_STRING(str)  ((str)[sizeof((str))-1] = '\0')


/******************************************************************************
 * TYPEDEFS
 *****************************************************************************/
/* by using the BOOL type we can improve the readability of the code. */
typedef int BOOL;

/******************************************************************************
 * Name:    HELPER_ConcatStrings
 * Purpose: concatenate two strings to a new (dynamic allocated) string
 * Parameters:
 *          pszStr1 [IN] - first string
 *          pszStr2 [IN] - second string
 * Return Value:
 *          Upon successful completion, a pointer to the created string
 *          is returned. the caller should free it with the free function.
 *          If the function fails, NULL is returned. 
 *****************************************************************************/
char * HELPER_ConcatStrings(const char * pszStr1, const char * pszStr2);

/******************************************************************************
 * Name:    HELPER_FindInStringsArray
 * Purpose: Return the index of specified string in strings array
 * Parameters:
 *          paszStringsArray [IN] - array of strings
 *          nArrayElements [IN] - number of strings in the array 
 *          pszStr [IN] - the string we are looking for
 *          nStrLength [IN] - the length of the string we are looking for
 * 
 * Return Value:
 *          Upon successful completion, the index of the string in the array.
 *          If the string was not found, -1 is returned
 *****************************************************************************/
int HELPER_FindInStringsArray(const char ** paszStringsArray,
                              int nArrayElements,
                              const char * pszStr,
                              int nStrLength);
#endif /* HELPER_H */
