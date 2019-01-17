#include "CalendarParser.h"
#include "LinkedListAPI.h"
#include <ctype.h>
#include <strings.h>
#include <limits.h>
#define DEBUG 1

Calendar * calendar;

char ** readFile (char * , int *);
char * findProperty(char ** file, int beginIndex, int endIndex, char * propertyName );
char * getToken ( char * entireFile, int * index);
void trim (char ** );
int checkFormatting( char ** entireFile, int numLines);
char ** getAllPropertyNames (char ** file, int beginIndex, int endIndex, int * numElements, int * errors);

/* ALL PROPERTIES OF CALENDAR */
const char calProperties[2][50] = { "CALSCALE", "METHOD" };
const char calPropertiesMULTI[2][50] = {"X-PROP", "IANA-PROP"};

/* ALL PROPERTIES FOR EVENT */
const char eventPropertiesREQUIRED[3][50] = { "UID", "DTSTAMP", "DTSTART" };
const char eventPropertiesONCE[14][50] = { "CLASS", "CREATED", "DESCRIPTION", "GEO", "LAST-MOD", "LOCATION", "ORGANIZER", "PRIORITY", "SEQ", "STATUS", "SUMMARY", "TRANSP", "URL", "RECURID" };
const char eventPropertiesMULTI[15][50] = { "RRULE", "DTEND", "DURATION", "ATTACH", "ATTENDEE", "CATEGORIES", "COMMENT", "CONTACT", "EXDATE", "RSTATUS", "RELATED", "RESOURCES", "RDATE", "X-PROP", "IANA-PROP"};

int main(int argv, char ** argc) {
    if (argv != 2) return 0;

    ICalErrorCode createCal = createCalendar(argc[1], &calendar);

    if (createCal == OK) printf("Calendar parsed OK\n");
    else {
        printf("Error occured\n");
        return 0;
    }

    char * output = printCalendar(calendar);
    printf("%s", output);
    free(output);

    deleteCalendar(calendar);

    return 0;
}

ICalErrorCode createCalendar(char* fileName, Calendar** obj) {
    int numLines = 0, correctVersion = 1;
    /* If the filename is null, or the string is empty */
    if (fileName == NULL || strcasecmp(fileName, "") == 0) return OTHER_ERROR;
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
    if (strcasecmp(entire_file[0], "BEGIN:VCALENDAR\r\n") || strcasecmp(entire_file[numLines - 1], "END:VCALENDAR\r\n")) {
        #if DEBUG
            printf("The file doesn't follow the correct start/end protocol. Exiting.\n");
        #endif
        for (int i = 0; i < numLines; i++) free(entire_file[i]);
        free(entire_file);
        /* Free each array, and then free the whole thing itself */
        return OTHER_ERROR;
    }

    /* Validates the formatting of the file */
    if (!checkFormatting(entire_file, numLines)) {
        #if DEBUG
            printf("It seems that the layout of the file events & alarms is incorrect\n");
        #endif
        for (int i = 0; i < numLines; i++) free(entire_file[i]);
        free(entire_file);
        return OTHER_ERROR;
    }

    *obj = (Calendar *) calloc ( 1, sizeof ( Calendar ) );
    /* Raw values */
    char * version = findProperty(entire_file, 0, numLines, "VERSION:");
    char * prodId = findProperty(entire_file, 0, numLines, "PRODID:");


    /* Trim any whitespace */
    /* Check to make sure that version is a number */
    if (version == NULL || strlen(version) == 0) {
        #if DEBUG
            printf("Error: Version is NULL\n");
        #endif
        /* Free memory */
        if (prodId) free(prodId);
        for (int i = 0; i < numLines; i++) free(entire_file[i]);
        free(entire_file);

        return OTHER_ERROR;
    }
    /* Trim whitespace */
    trim(&version);
    /* Questionable */
    if (prodId == NULL || version == NULL || strlen(version) == 0 || strlen(prodId) == 0) {
        #if DEBUG
            printf("Error: Either version is empty after trimming, or prodId is empty.\n");
        #endif
        if (prodId) free(prodId);
        if (version) free(version);
        for (int i = 0; i < numLines; i++) free(entire_file[i]);
        free(entire_file);

        return OTHER_ERROR;
    }
    /* It's not a number, cannot be represented by a float */
    for (int i = 0; i < strlen(version); i++) {
        if ( (version[i] >= '0' && version[i] <= '9') || version[i] == '.'  || version[i] == '\r' || version[i] == '\n') continue;
        correctVersion = 0;
    }
    /* Basically, the version is not a number */
    if (!correctVersion) {
        #if DEBUG
            printf("Error: Version is not a floating point\n");
        #endif
        free(prodId);
        free(version);
        for (int i = 0; i < numLines; i++) free(entire_file[i]);
        free(entire_file);
        return OTHER_ERROR;
    }
    /* Write the version & production id */
    (*obj)->version = (float) atof(version);;
    strcpy((*obj)->prodID, prodId);

    /* Free */
    if (version) free (version);
    if (prodId) free (prodId);

    /* After we're done extracting the version and prodId, we can begin extracting all other calendar properties */
    int N = 0, errors = 0;
    char ** calendarProperties = getAllPropertyNames(entire_file, 0, numLines, &N, &errors);
    /* If any errors occured */

    if (errors || !N) {
        for (int x = 0; x < N; x++) free(calendarProperties[x]);
        free(calendarProperties);
        for (int i = 0; i < numLines; i++) free(entire_file[i]);
        free(entire_file);
        return OTHER_ERROR;
    }

    (*obj)->properties = initializeList(printProperty, deleteProperty, compareProperties);

    /* Grab all calendar properties */
    for (int x = 0; x < N; x++) {
        /* We already checked those guys */
        if (strcasecmp(calendarProperties[x], "PRODID:") && strcasecmp(calendarProperties[x], "VERSION:")) {
            char * value = findProperty(entire_file, 0, numLines, calendarProperties[x]);
            Property * prop = NULL;
            prop = (Property *) calloc ( 1, sizeof (Property) + sizeof(char) * (1 + strlen(value)));
            int j = 0;
            /* We don't want to include the delimeter */
            for ( ; j < strlen(calendarProperties[x]); j++)
                prop->propName[j] = calendarProperties[x][j];
            prop->propName[j] = '\0';
            strcpy(prop->propDescr, value);
            insertBack((*obj)->properties, prop);

            free(value);
        }
        /* Free the element */
        free(calendarProperties[x]);
    }
    /* Free the 2d array */
    free(calendarProperties);

    //(*obj)->events = initializeList(printEvent, deleteEvent, compareEvents);

    // for (int k = 0; k < numLines; k++) {
    //     int numE = 0, errors = 0;
    //     char ** test = getAllPropertyNames(entire_file, k, numLines, &numE, &errors);
    //     if (test == NULL) continue;
    //     for (int j = 0; j < numE; j++) {
    //         char * t = findProperty(entire_file, k, numLines, test[j]);
    //         printf("%s%s\n", test[j], t);
    //     }
    //     printf("========================\n");
    // }

    for (int i = 0; i < numLines; i++) free(entire_file[i]);
    free(entire_file);

    return OK;
}
/* Deletes the calendar */
void deleteCalendar(Calendar * obj) {
    if (obj == NULL) return;
    /* gonna have to delete the lists */
    // if (obj->events) freeList(obj->events);
    // if (obj->properties) freeList(obj->properties);
    /* lastly */
    freeList( obj->properties );
    free ( obj );
}
/* Assumes everything ends with \r\n */
void trim (char ** string) {
    char * str = *string;
    char * newString = NULL;
    if (str == NULL || !strlen(str)) return;

    int beginIndex = 0, length = strlen(str), endIndex = length - 2, newLength, i, j;
    while (beginIndex < length && isspace(str[beginIndex])) beginIndex++;
    while (endIndex >= beginIndex && isspace(str[endIndex])) endIndex--;

    newLength = endIndex - beginIndex + 1;
    newString = (char *) calloc (1, newLength + 3 );
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
                line = (char *) calloc (1, 2 );
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
            entireFile = (char *) calloc( 1, 1 );
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

    fclose(calendarFile);

    if (!index || !entireFile) {
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
            #if DEBUG
                printf("Successfully freed all memory!\n");
            #endif
            return NULL;
        }

        char * line = NULL;
        line = (char *) calloc(1, tokenSize + 3 );
        /* Create line from token, adding back in the delimeter */
        strcpy(line, token);
        free (token);
        /* Add line break, and \0 for the meme */
        line[tokenSize] = '\r';
        line[tokenSize + 1] = '\n';
        line[tokenSize + 2] = '\0';
        /* Allocate/Reallocate memory dynamically */
        if (returnFile == NULL) {
            returnFile = calloc(1, sizeof(char *) );
        } else {
            returnFile = realloc(returnFile, sizeof(char *) * (numTokens + 1));
        }
        /* Create enough space in that index for the line of text */
        returnFile[numTokens] = (char *) calloc (1,  strlen(line) + 1);
        strcpy(returnFile[numTokens], line);
        free(line);
        /* Add the \0 to indicate line ending */
        returnFile[numTokens][strlen(returnFile[numTokens])] = '\0';
        numTokens++;

    }
    /* Free memory & close File */
    free(entireFile);
    *numLines = numTokens;
    /* END */
    return returnFile;
}
/* Extracts property */
/* [Begin Index, End Index) */
char * findProperty(char ** file, int beginIndex, int endIndex, char * propertyName ) {
    int li = beginIndex, opened = 0, closed = 0, currentlyFolding = 0, index = 0;
    char * result = NULL;
    /* Doesn't start properly */
    if ( strcasecmp(file[li], "BEGIN:VCALENDAR\r\n") && strcasecmp(file[li], "BEGIN:VALARM\r\n") && strcasecmp(file[li], "BEGIN:VEVENT\r\n") )
        return NULL;
    while (li < endIndex) {
        if ( !strcasecmp(file[li], "BEGIN:VCALENDAR\r\n") || !strcasecmp(file[li], "BEGIN:VALARM\r\n") || !strcasecmp(file[li], "BEGIN:VEVENT\r\n") ) {
            opened++;
        } else if ( !strcasecmp(file[li], "END:VCALENDAR\r\n") || !strcasecmp(file[li], "END:VALARM\r\n") || !strcasecmp(file[li], "END:VEVENT\r\n") ) {
            closed++;
            if (closed == opened) {
                /* Basically exit the program */
                if (result) free (result);
                return NULL;
            }
        } else {
            /* If we're on the current layer */
            if (opened - 1 == closed) {
                if ((file[li][0] == '\t' || file[li][0] == ' ') && currentlyFolding) {
                    for (int j = 1; j < strlen(file[li]) - 2; j++) {
                        if (!result) result = calloc(1, 2);
                        else result = realloc(result, strlen(result) + 2);
                        result[index] = file[li][j];
                        result[index + 1] = '\0';
                        index++;
                    }
                    if (li + 1 < endIndex) {
                        if (file[li + 1][0] == ' ' || file[li + 1][0] == '\t') {
                            currentlyFolding = 1;
                        } else {
                            return result;
                        }
                    } else {
                        return result;
                    }
                } else {
                    if (strlen(propertyName) <= strlen(file[li]) - 2) {
                        int match = 1, j = 0;
                        for (; j < strlen(propertyName); j++)
                            if (propertyName[j] != file[li][j]) match = 0;
                        if (match) {
                            for (; j < strlen(file[li]) - 2; j++) {
                                if (!result) result = calloc(1, 2);
                                else result = realloc(result, strlen(result) + 2);
                                result[index] = file[li][j];
                                result[index + 1] = '\0';
                                index++;
                            }
                            if (li + 1 < endIndex) {
                                if (file[li + 1][0] == ' ' || file[li + 1][0] == '\t') {
                                    currentlyFolding = 1;
                                } else {
                                    return result;
                                }
                            } else {
                                return result;
                            }
                        }
                    }
                }
            }
        }
        li++;
    }
    return NULL;
}
/* Verifies that all alarms & events are distributed correctly */
int checkFormatting (char ** entire_file, int numLines) {
    if (numLines < 2) return 0;
    if (strcasecmp(entire_file[0], "BEGIN:VCALENDAR\r\n")) return 0;
    if (strcasecmp(entire_file[numLines - 1], "END:VCALENDAR\r\n")) return 0;

    int isEventOpen = 0;
    int isAlarmOpen = 0;
    int numEventsOpened = 0, numEventsClosed = 0;
    int numAlarmsOpened = 0, numAlarmsClosed = 0;
    int error = 0;

    for (int i = 1; i < numLines - 1; i++) {
        if (strcasecmp(entire_file[i], "BEGIN:VEVENT\r\n") == 0) {
            if (isEventOpen) return 0; /* There's an open event */
            if (isAlarmOpen) return 0; /* There's an open alarm */

            isEventOpen = 1;
            numEventsOpened++;
        } else if (strcasecmp(entire_file[i], "END:VEVENT\r\n") == 0) {
            if (!isEventOpen) return 0; /* We have no ongoing events */
            if (isAlarmOpen) return 0; /* There's an alarm open */
            if (numAlarmsOpened != numAlarmsClosed) {
                return 0; /* something is bad */
            }
            /* Each time we close an event we re-set alarms */
            numAlarmsOpened = numAlarmsClosed = 0;
            isEventOpen = 0;
            numEventsClosed++;
        } else if (strcasecmp(entire_file[i], "BEGIN:VALARM\r\n") == 0) {
            if (isAlarmOpen) return 0;
            if (!isEventOpen) return 0;

            isAlarmOpen = 1;
            numAlarmsOpened++;
        } else if (strcasecmp(entire_file[i], "END:VALARM\r\n") == 0) {
            if (!isAlarmOpen) return 0;
            if (!isEventOpen) return 0;

            isAlarmOpen = 0;
            numAlarmsClosed++;
        } else if (entire_file[i][0] != ' ' && entire_file[i][0] != '\t') {
            int lineLength = strlen(entire_file[i]);
            int colons = 0;
            for (int j = 1; j < lineLength; j++)
                colons += (entire_file[i][j] == ':' || entire_file[i][j] == ';');
            if (!colons) error = 1;
        }
    }
    /* Big Return */
    return !isAlarmOpen && !isEventOpen && numAlarmsOpened == numAlarmsClosed && numEventsOpened == numEventsClosed && !error;
}
/* prints the calendar */
char * printCalendar (const Calendar * obj) {
    char * result = NULL;

    if (obj == NULL) return NULL;
    result = (char *) calloc ( 1, 10 );
    strcpy(result, "CALENDAR\n");
    result = (char *) realloc (result, strlen(result) + 8 + strlen(obj->prodID) + 1);
    strcat(result, "PRODID:");
    strcat(result, obj->prodID);
    strcat(result, "\n");

    char version[200 + 10];
    sprintf(version, "VERSION:%f", obj->version);

    result = (char *) realloc (result, strlen(result) + strlen(version) + 1);
    strcat(result, version);

    char * otherProps = toString(obj->properties);
    result = (char *) realloc (result, strlen(result) + strlen(otherProps) + 2);
    strcat(result, otherProps);
    strcat(result, "\n");

    free(otherProps);

    return result;
}
/* Returns a list of strings representing property names given an index */
/* All strings are terminated with \0 and nothing else */
char ** getAllPropertyNames (char ** file, int beginIndex, int endIndex, int * numElements, int * errors) {
    int index = beginIndex;
    int opened = 0;
    int closed = 0;
    char ** result = NULL;

    /* Must start with begin or end index */
    if ( strcasecmp(file[index], "BEGIN:VCALENDAR\r\n") && strcasecmp(file[index], "BEGIN:VALARM\r\n") && strcasecmp(file[index], "BEGIN:VEVENT\r\n") ) {
        return NULL;
    }

    while (index < endIndex) {
        char * ln = file[index];
        /* BEGIN */
        if (toupper(ln[0]) == 'B' && toupper(ln[1]) == 'E' && toupper(ln[2]) == 'G' && toupper(ln[3]) == 'I' && toupper(ln[4]) == 'N') {
            opened++;
        /* END */
        } else if (toupper(ln[0]) == 'E' && toupper(ln[1]) == 'N' && toupper(ln[2]) == 'D') {
            closed++;
            if (opened == closed) {
                break;
            }
        /* FOLDED */
        } else if (ln[0] == ' ' || ln[0] == '\t') {
        } else {
            if (opened - 1 == closed) {
                int colonIndex = 0, foundColon = 0;
                /* Find colon index */
                for (int i = 0; i < strlen(ln); i++) {
                    colonIndex++;
                    if (ln[i] == ':' || ln[i] == ';') {
                        foundColon = 1;
                        break;
                    }
                }
                /* if we have not found a colon of any sort it's an error */
                *errors += !foundColon;

                if (*errors) {
                    #if DEBUG
                        printf("Seems that there is a content line that is missing a colon of sorts\n");
                    #endif 
                    for (int j = 0; j < *numElements; j++)
                        free ( result[j] );
                    if (result) free(result);
                    return NULL;
                }

                char * line = NULL;
                line = (char *) calloc (1, colonIndex );

                for (int i = 0; i < colonIndex; i++) {
                    line[i] = ln[i];
                }
                line[colonIndex] = '\0';

                /* After we found the event name */
                if (result == NULL) {
                    result = calloc (1, sizeof (char *) );
                } else {
                    result = realloc ( result, sizeof (char *) * ( *numElements + 1) ) ;
                }
                /* Copy it into the resulting array */
                result[*numElements] = calloc(1, strlen(line) + 1 );
                strcpy(result[*numElements], line);
                /* Free & append the null terminator */
                free(line);
                result[*numElements][strlen(result[*numElements])] = '\0';
                /* Increase the size of array */
                *numElements += 1;
            }
        }
        index++;
    }
    return result;
}
/* <-----HELPER FUNCTIONS------> */

/* EVENTS */
void deleteEvent(void* toBeDeleted) {
    Event * e = (Event *) toBeDeleted;
    /* Delete all the other things */
    free(e);
}
int compareEvents(const void* first, const void* second) {
    return 0;
}
char* printEvent(void* toBePrinted) {
    char * result = NULL;
    if (toBePrinted == NULL) {
        /* NULL EVENT */
        result = (char *) calloc (1, 11);
        strcpy(result, "NULL EVENT");
        return result;
    }
    Event * e = (Event *) toBePrinted;

    result = (char *) calloc(1, 19);
    strcpy(result, "====EVENT====\nUID:");
    result = (char *) realloc(result, strlen(result) + strlen(e->UID) + 2);
    strcpy(result, e->UID);
    strcpy(result, "\n");

    return result;
}
/* PROPERTIES */
void deleteProperty(void* toBeDeleted) {
    Property * p = (Property *) toBeDeleted;
    /* We don't have to free the flexible array */
    free(p);
}
int compareProperties(const void* first, const void* second) {
    return 0;
}
/* Prints an individual property */
char* printProperty(void* toBePrinted) {
    if (toBePrinted == NULL) return NULL;
    Property * p = (Property *) toBePrinted;
    /* Allocates enough space for the property name and null-terminator */
    char * result = NULL;
    result = (char *) calloc ( 1, strlen(p->propName) + 1 );
    /* Copies the property name into the resulting string */
    strcpy(result, p->propName);
    /* Allocates some more memory for the property description */
    result = (char *) realloc (  result, strlen(result) + strlen(p->propDescr) + 1);
    /* Appends the description onto the result */
    strcat(result, p->propDescr);
    /* Returns result */
    return result;
}
