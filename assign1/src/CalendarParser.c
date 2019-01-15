#include "CalendarParser.h"
#include "LinkedListAPI.h"
#include <ctype.h>
#define DEBUG 1

Calendar * calendar;

char ** readFile (char * , int *);
char * findProperty(char ** file, int beginIndex, int endIndex, char * propertyName, char );
char * getToken ( char * entireFile, int * index);
void trim (char ** );

int main() {
    ICalErrorCode createCal = createCalendar("test2.ical", &calendar);
    if (createCal == OK) printf("Calendar parsed OK\n");
    else printf("Error occured\n");
    deleteCalendar(calendar);
    return 0;
}

ICalErrorCode createCalendar(char* fileName, Calendar** obj) {
    int numLines = 0, correctVersion = 1, singlePeriod = 0;
    /* If the filename is null, or the string is empty */
    if (fileName == NULL || strcmp(fileName, "") == 0) return OTHER_ERROR;
    /* If the calendar object doesn't point to anything */
    if (obj == NULL) return OTHER_ERROR;

    /* Attempt to open the file */
    FILE * calendarFile = fopen(fileName, "r");

    /* See if it's NULL */
    if (calendarFile == NULL) return OTHER_ERROR;

    //At this point we already know the file exists, and is valid.
    fclose( calendarFile );
    /* Assuming everyt1hing has worked */
    char ** entire_file = readFile ( fileName, &numLines);

    if (entire_file == NULL) return OTHER_ERROR;
    /* Does not begin or end properly */
    if (strcmp(entire_file[0], "BEGIN:VCALENDAR\r\n") || strcmp(entire_file[numLines - 1], "END:VCALENDAR\r\n")) {
        #if DEBUG
            printf("The file doesn't follow the correct start/end protocol. Exiting.\n");
        #endif
        for (int i = 0; i < numLines; i++) free(entire_file[i]);
        free(entire_file);
        /* Free each array, and then free the whole thing itself */
        return OTHER_ERROR;
    }
    *obj = (Calendar *) malloc ( sizeof ( Calendar ) );
    /* Raw values */
    char * version = findProperty(entire_file, 1, numLines, "VERSION", ':');
    char * prodId = findProperty(entire_file, 1, numLines, "PRODID", ':');

    /* Trim any whitespace */
    /* Check to make sure that version is a number */
    if (version == NULL || strlen(version) == 0) {
        #if DEBUG
            printf("Error: Version is NULL\n");
        #endif 
        /* Free memory */
        if (prodId) free(prodId);
        free ( *obj );
        for (int i = 0; i < numLines; i++) free(entire_file[i]);
        free(entire_file);

        return OTHER_ERROR;
    }
    /* Trim whitespace */
    trim(&version);
    /* Questionable */
    if (prodId == NULL || strlen(version) == 0 || version == NULL || strlen(prodId) == 0) {
        #if DEBUG
            printf("Error: Either version is empty after trimming, or prodId is empty.\n");
        #endif
        if (prodId) free(prodId);
        if (version) free(version);
        free ( *obj );
        for (int i = 0; i < numLines; i++) free(entire_file[i]);
        free(entire_file);

        return OTHER_ERROR;
    }
    /* It's not a number, cannot be represented by a float */
    for (int i = 0; i < strlen(version); i++) {
        if (isdigit(version[i])) continue;
        if (version[i] == '.' && !singlePeriod) {
            singlePeriod = 1;
            continue;
        }
        /* End of line can be ignored for now */
        if (version[i] == '\r' || version[i] == '\n') continue;
        correctVersion = 0;
    }
    /* Basically, the version is not a number */
    if (!correctVersion) {
        #if DEBUG
            printf("Error: Version is not a floating point\n");
        #endif
        free(prodId);
        free(version);
        free( *obj );
        for (int i = 0; i < numLines; i++) free(entire_file[i]);
        free(entire_file);
        return OTHER_ERROR;
    }
    /* Write the version & production id */
    (*obj)->version = (float) atof(version);;
    strcpy((*obj)->prodID, prodId);

    printf("Version: %.2f\n", (*obj)->version);
    printf("ProdId: %s\n", prodId);

    /* Close the file */
    free (version);
    free (prodId);
    for (int i = 0; i < numLines; i++) free(entire_file[i]);
    free(entire_file);

    return OK;
}
void deleteCalendar(Calendar * obj) {
    if (obj == NULL) return;
    /* gonna have to delete the lists */

    /* lastly */
    free ( obj );
}
/* Assumes everything ends with \r\n */
void trim (char ** string) {
    char * str = *string;
    char * newString;
    if (str == NULL || !strlen(str)) return;

    int beginIndex = 0, length = strlen(str), endIndex = length - 3, newLength, i, j;
    while (beginIndex < length && isspace(str[beginIndex])) beginIndex++;
    while (endIndex >= beginIndex && isspace(str[endIndex])) endIndex--;

    newLength = endIndex - beginIndex + 1;
    newString = (char *) malloc (newLength + 3);
    for (i = beginIndex, j = 0; i <= endIndex; i++, j++) {
        newString[j] = str[i];
    }
    newString[newLength + 0] = '\r';
    newString[newLength + 1] = '\n';
    newString[newLength + 2] = '\0';
    /* Free the old string and re-assign it */
    if (str) free(str);
    *string = newString;
}
/* Reads tokens, returns -1 for index if the tokens are bad */
char * getToken ( char * entireFile, int * index) {
    char * line = NULL;
    int lineIndex = 0;
    /* Quick checks */
    if (entireFile == NULL || strlen(entireFile) == 0) return NULL;
    if (*index == strlen(entireFile)) return NULL;
    if (index == NULL || *index < 0) return NULL;

    /* Loop until the end */
    while (*index < strlen(entireFile)) {
        /* If we reached what seems to be a line ending */
        /* printf("Current character is %c\n", entireFile[*index]); */
        if (entireFile[*index] == '\r') {
            /* Check to make sure we can check the next character */
            if ((*index + 1) < strlen(entireFile)) {
                /* If it's a \n */
                if (entireFile[*index + 1] == '\n') {
                    /* Skip the next character */
                    *index += 2;
                    /* Return output */
                    return line;
                } else {
                    /* Random \r??? */
                    #if DEBUG
                        printf("Encountered a backslash-r without backslash-n. Exiting.\n");
                    #endif
                    if (line) free(line);
                    /* Return NULL */
                    *index = -1;
                    return NULL;
                }
            }
        } else if (entireFile[*index] == '\n') {
            #if DEBUG
                printf("Encountered a backslash-n. Exiting.\n");
            #endif 
            if (line) free(line);
            *index = -1;
            return NULL;
        } else {
            if (line == NULL) {
                line = (char *) malloc (2);
                line[lineIndex] = entireFile[*index];
                line[lineIndex + 1] = '\0';
            } else {
                line = (char *) realloc (line, lineIndex + 2);
                line[lineIndex] = entireFile[*index];
                line[lineIndex + 1] = '\0';
            }
            *index += 1;
            lineIndex += 1;
            /* Should be good */
        }
    }
    #if DEBUG
        printf("Was not able to tokenize string, for unknown reason.\n");
    #endif
    if (line) free(line);
    *index = -1;
    return NULL;
}
/* Reads an entire file, tokenizes it and exports an array of lines */
char ** readFile ( char * fileName, int * numLines ) {
    /* Local variables */
    FILE * calendarFile = fopen(fileName, "r");

    char buffer = EOF;
    char * entireFile = NULL;
    char ** returnFile = NULL;
    int index = 0, tokenSize, numTokens = 0;

    /* Read until we hit end of file */
    while ( 1 ) {
        buffer = fgetc(calendarFile);
        if (buffer == EOF) {
            break;
        }
        /* Realloc to insert new character */
        if (entireFile == NULL) {
            entireFile = (char *) malloc( 1 );
            entireFile[0] = (char) buffer;
        } else {
            entireFile = realloc(entireFile, index + 2);
            entireFile[index] = (char) buffer;
            /* Adds backslash zero */
            entireFile[index + 1] = '\0';
        }
        /* Incremenet index either way */
        index++;
    }
    
    if (!index || !entireFile) {
        fclose(calendarFile);
        return NULL;
    }

    /* Prepare the array */
    index = 0;
    /* Tokenizes the String */
    while ( 1 ) {
        char * token = getToken(entireFile, &index);
        /* Minor error checking */
        if (token == NULL) break;
        tokenSize = strlen(token);

        if (index == -1 || tokenSize == 0) {
            #if DEBUG
                printf("Error: tokenizer returned: (index: %d, tokenSize: %d)\n", index, tokenSize);
                printf("Attempting to free all memory!\n");
            #endif
            /* Free everything */
            if (token) free (token);
            if (entireFile) free(entireFile);
            /* Whatever we've allocated before */
            /* Free it */
            if (returnFile) {
                for (int i = 0; i < numTokens; i++)
                    free ( returnFile[i] );
                free(returnFile);
            }
            fclose(calendarFile);
            #if DEBUG
                printf("Successfully freed all memory!\n");
            #endif
            return NULL;
        }

        char * line = (char *) malloc(tokenSize + 3);
        /* Create line from token, adding back in the delimeter */
        strcpy(line, token);
        free (token);
        /* Add line break, and \0 for the meme */
        line[tokenSize] = '\r';
        line[tokenSize + 1] = '\n';
        line[tokenSize + 2] = '\0';
        /* Allocate/Reallocate memory dynamically */
        if (returnFile == NULL) {
            returnFile = malloc(sizeof(char *));
        } else {
            returnFile = realloc(returnFile, sizeof(char *) * (numTokens + 1));
        }
        /* Create enough space in that index for the line of text */
        returnFile[numTokens] = (char *) malloc ( strlen(line) + 1);
        strcpy(returnFile[numTokens], line);
        free(line);
        /* Add the \0 to indicate line ending */
        returnFile[numTokens][strlen(returnFile[numTokens])] = '\0';
        numTokens++;
        
    }
    /* Free memory & close File */
    free(entireFile);
    fclose(calendarFile);
    *numLines = numTokens;
    /* END */
    return returnFile;
}
/* Extracts property */
/* [Begin Index, End Index) */
char * findProperty(char ** file, int beginIndex, int endIndex, char * propertyName, char matchDelim) {
    int pLen = strlen(propertyName);
    char * result = NULL;
    int index = 0, foundProperty = 0, blockBegan = 0, blockEnded = 0;
    /* Doesn't make sense */
    if (endIndex < beginIndex) return NULL;
    /* Run through ea. line */
    for (int i = beginIndex; i < endIndex; i++) {
        int len = strlen(file[i]);
        /* See if we're already tracking down a property */
        if (foundProperty && blockBegan == blockEnded) {
            if (isspace(file[i][0])) {
                /* Only if it's folded */
                for (int j = 1; j < len && file[i][j] != '\r'; j++) {
                    if (result == NULL) result = (char *) malloc(2);
                    else result = realloc(result, index + 2);
                    result[index] = file[i][j];
                    result[index + 1] = '\0';
                    index++;
                }
            } else {
                /* Only returns result if it's within the range */
                return result;
            }
        } else {
            /* Check for blocks */
            if (len > 3) if (toupper(file[i][0]) == 'E' && toupper(file[i][1]) == 'N' && toupper(file[i][2]) == 'D' && file[i][3] == ':') blockEnded++;
            if (len > 5) if (toupper(file[i][0]) == 'B' && toupper(file[i][1]) == 'E' && toupper(file[i][2]) == 'G' && toupper(file[i][3]) == 'I' && toupper(file[i][4]) == 'N' && file[i][5] == ':') blockBegan++;
            /* Make sure it fits (that's what she said) and that we're not inside some block */
            if (len > (pLen + 1) && (blockBegan == blockEnded)) {
                int matches = 1;
                int j = 0;
                for (; j < pLen; j++) {
                    if (tolower(file[i][j]) != tolower(propertyName[j])) {
                        matches = 0;
                        break;
                    }
                }
                /* After we matched property name, check the delimeter too */
                if (matches && file[i][j] == matchDelim) {
                    ++j;
                    /* Skip delimeter and began writing */
                    for ( ; j < len && file[i][j] != '\r'; j++) {
                        if (result == NULL) result = (char *) malloc(2);
                        else result = realloc(result, index + 2);
                        result[index] = file[i][j];
                        result[index + 1] = '\0';
                        index++;

                        foundProperty = 1;
                    }
                }
            }
        }
    }
    #if DEBUG
        printf("Property name %s was not found within %d and %d\n", propertyName, beginIndex, endIndex);
    #endif
    if (result) free(result);
    /* Found nothing useful */
    return NULL;
}
