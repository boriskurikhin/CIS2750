#include "CalendarParser.h"
#include "LinkedListAPI.h"
#include <ctype.h>
#include <strings.h>
#include <limits.h>
#define DEBUG 0

Calendar * calendar;

char ** readFile (char * , int *);
char * findProperty(char ** file, int beginIndex, int endIndex, char * propertyName );
char * getToken ( char * entireFile, int * index);
void trim (char ** );
int checkFormatting( char ** entireFile, int numLines);
char ** getAllPropertyNames (char ** file, int beginIndex, int endIndex, int * numElements, int * errors);
int validateStamp ( char * check );

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
    int numLines = 0, correctVersion = 1, fnLen = 0;
    /* If the filename is null, or the string is empty */
    if (fileName == NULL || strcasecmp(fileName, "") == 0) return OTHER_ERROR;
    /* If the calendar object doesn't point to anything */
    if (obj == NULL) return OTHER_ERROR;

    /* Check file extension */
    fnLen = strlen(fileName);
    if (toupper(fileName[fnLen - 1]) != 'S' || toupper(fileName[fnLen - 2]) != 'C' || toupper(fileName[fnLen - 3]) != 'I' || fileName[fnLen - 4] != '.') {
        return OTHER_ERROR;
    }

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
            for ( ; j < strlen(calendarProperties[x]) - 1; j++)
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

    /* Allocate some memory for the events */
    (*obj)->events = initializeList(printEvent, deleteEvent, compareEvents);

    /* Start tracking the events */
    for (int beginLine = 1; beginLine < numLines; ) {
        if (!strcasecmp(entire_file[beginLine], "BEGIN:VEVENT\r\n")) {
            #if DEBUG
                printf("NEW EVENT\n");
            #endif
            int endLine = beginLine + 1;
            /* Track the end */
            for ( ; endLine < numLines; endLine++) {
                if (!strcasecmp(entire_file[endLine], "END:VEVENT\r\n")) {
                    break;
                }
            }

            /* Required property for all events */
            char * UID = NULL;
            UID = findProperty(entire_file, beginLine, numLines, "UID:");
            
            if (UID == NULL || !strlen(UID)) {
                if (UID) free(UID); /* empty string or something */
                for (int i = 0; i < numLines; i++) free(entire_file[i]);
                free(entire_file);
                #if DEBUG
                    printf("Error! Some event is missing a UID\n");
                #endif
                return OTHER_ERROR;
            }

            char * DTSTAMP = NULL;
            DTSTAMP = findProperty(entire_file, beginLine, numLines, "DTSTAMP:");

            /* Quick stamp validation & null checking */
            if (DTSTAMP == NULL || !strlen(DTSTAMP) || !validateStamp(DTSTAMP)) {
                if (DTSTAMP) free(DTSTAMP);
                free(UID);
                for (int i = 0; i < numLines; i++) free(entire_file[i]);
                free(entire_file);
                #if DEBUG
                    printf("Error! Some event is missing a DTSTAMP or the DTSTAMP format is invalid..\n");
                #endif
                return OTHER_ERROR;
            }

            Event * newEvent = NULL;
            newEvent = (Event *) calloc ( 1, sizeof(Event) );

            /* Writing the date & time into their respective slots */
            int di = 0, ti = 0, index = 0;
            bool isUTC = (DTSTAMP[strlen(DTSTAMP) - 1] == 'Z' ? true : false);
            for ( ; index < (isUTC ? strlen(DTSTAMP) - 1 : strlen(DTSTAMP)); index++) {
                if (DTSTAMP[index] != 'T') {
                    if (index < 8) newEvent->creationDateTime.date[di++] = DTSTAMP[index];
                    else newEvent->creationDateTime.time[ti++] = DTSTAMP[index];
                }
            }
            /* UTC or NOT */
            newEvent->creationDateTime.UTC = isUTC;
            /* Append null terminators */
            strcat(newEvent->creationDateTime.date, "\0");
            strcat(newEvent->creationDateTime.time, "\0");
        
            strcpy(newEvent->UID, UID);

            free(UID);
            free(DTSTAMP);

            /* Once that's done, we can start parsing out other information */
            int pCount = 0, errors = 0;
            char ** eventProperties = getAllPropertyNames(entire_file, beginLine, numLines, &pCount, &errors);

            if (errors || !pCount) {
                #if DEBUG
                    printf("Error! While retrieving properties of event\n");
                #endif
                for (int x = 0; x < pCount; x++) free(eventProperties[x]);
                if (eventProperties) free(eventProperties);
                for (int x = 0; x < numLines; x++) free(entire_file[x]);
                free(entire_file);
                return OTHER_ERROR;
            }

            newEvent->properties = initializeList(printProperty, deleteProperty, compareProperties);

            for (int x = 0; x < pCount; x++) {
                /* We already have this one */
                if (strcasecmp(eventProperties[x], "UID:") && strcasecmp(eventProperties[x], "DTSTAMP:")) {
                    /* Now we check some other shit */
                    /* Have to create another event property */
                    if (!strcasecmp(eventProperties[x], "DTSTART:")) {

                    } else {
                        char * value = findProperty(entire_file, beginLine, numLines, eventProperties[x]);
                        
                        if (!value || !strlen(value)) {
                            #if DEBUG
                                printf("Error! While retrieving value of event property\n");
                            #endif
                            if (value) free(value);
                            for (int j = 0; j < pCount; j++) free(eventProperties[j]);
                            free(eventProperties);
                            for (int j = 0; j < numLines; j++) free(entire_file[j]);
                            free(entire_file);
                            return OTHER_ERROR;
                        }
                        
                        Property * prop = (Property *) calloc (1, sizeof(Property) + strlen(value) + 1);
                        int j = 0;
                        for (; j < strlen(eventProperties[x]) - 1; j++)
                            prop->propName[j] = eventProperties[x][j];
                        prop->propName[j] = '\0';
                        strcpy(prop->propDescr, value);
                        free(value);
                        insertBack(newEvent->properties, prop);
                    }
                }
                /* Free it after using it */
                free(eventProperties[x]);
            }
            free(eventProperties);

            /* Insert the event at the back of the queue in the calendar object */
            insertBack((*obj)->events, newEvent);

            /* Once we're done */
            beginLine = endLine + 1;
        } else {
            beginLine++;
        }
    }

    for (int i = 0; i < numLines; i++) free(entire_file[i]);
    free(entire_file);

    return OK;
}
/* Deletes the calendar */
void deleteCalendar(Calendar * obj) {
    if (obj == NULL) return;
    freeList( obj->properties );
    freeList( obj->events );
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
        } else if (entire_file[i][0] != ' ' && entire_file[i][0] != '\t' && entire_file[i][0] != ';') {
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
    int length = 10;

    result = (char *) calloc ( 1, length );
    strcpy(result, "CALENDAR\n");
    length += (8 + strlen(obj->prodID) + 1);
    result = (char *) realloc (result, length);
    strcat(result, "PRODID:");
    strcat(result, obj->prodID);
    strcat(result, "\n");

    char version[200 + 10];
    sprintf(version, "VERSION:%f", obj->version);
    length += strlen(version) + 1;
    result = (char *) realloc (result, length);
    strcat(result, version);

    char * otherProps = NULL;
    otherProps = toString(obj->properties);
    length += strlen(otherProps) + 2;
    result = (char *) realloc (result, length);
    strcat(result, otherProps);
    strcat(result, "\n");

    free(otherProps);

    /* At this point we have all calendar properties parsed and ready to go */
    /* This is where we going to have to implement events */
    
    length += 7;
    result = (char *) realloc( result, length);
    strcat(result, "EVENTS");

    char * events = NULL;
    events = toString(obj->events);
    length += strlen(events) + 1;
    result = (char *) realloc ( result, length);
    strcat(result, events);
    strcat(result, "\n");

    free(events);

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
        } else if (ln[0] == ' ' || ln[0] == '\t' || ln[0] == ';') {
        } else {
            if (opened - 1 == closed) {
                int colonIndex = 0, foundColon = 0;
                /* Find colon index */
                for (int i = 0; i < strlen(ln);  i++) {
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
                line = (char *) calloc (1, colonIndex + 1);

                for (int i = 0; i < colonIndex; i++) {
                    line[i] = ln[i];
                }
                strcat(line, "\0");

                /* After we found the event name */
                if (result == NULL) {
                    result = calloc (1, sizeof (char *) );
                } else {
                    result = realloc ( result, sizeof (char *) * ( *numElements + 1) ) ;
                }
                /* Copy it into the resulting array */
                int strLen = strlen(line) + 1;
                result[*numElements] = NULL;
                result[*numElements] = calloc(1, strLen );
                strcpy(result[*numElements], line);
                /* Free & append the null terminator */
                free(line);
                strcat(result[*numElements], "\0");
                /*
                result[*numElements][strlen(result[*numElements])] = '\0';
                */
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
    freeList(e->properties);
    free(e);
}
int compareEvents(const void* first, const void* second) {
    return 0;
}
/* Temporary print */
char* printEvent(void* toBePrinted) {
    if (toBePrinted == NULL) return NULL;

    Event * event = NULL;
    char * result = NULL;
    
    event = (Event *) toBePrinted;
    result = (char *) calloc(1, 8);
    strcpy(result, "\tEVENT:");
    result = (char *) realloc( result, strlen(result) + strlen(event->UID) + 1);
    strcat(result, event->UID);
    result = (char *) realloc (result, strlen(result) + 12);
    strcat(result, "\n\tDTSTAMP->");
    result = (char *) realloc (result, strlen(result) + strlen(event->creationDateTime.date) + strlen(event->creationDateTime.time) + 1);
    strcat(result, event->creationDateTime.date);
    strcat(result, event->creationDateTime.time);
    if (event->creationDateTime.UTC) {
        result = (char *) realloc (result, strlen(result) + 6);
        strcat(result, "(UTC)");
    }
    char * otherProps = NULL;
    otherProps = toString(event->properties);
    result = (char *) realloc (result, strlen(result) + strlen(otherProps) + 2);
    strcat(result, otherProps);
    free(otherProps);

    strcat(result, "\n");
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
    result = (char *) calloc ( 1, strlen(p->propName) + 2 );
    /* Copies the property name into the resulting string */
    strcpy(result, "\t");
    strcat(result, p->propName);
    /* Allocates some more memory for the property description */
    result = (char *) realloc (  result, strlen(result) + strlen(p->propDescr) + 3);
    /* Appends the description onto the result */
    strcat(result, "->");
    strcat(result, p->propDescr);
    /* Returns result */
    return result;
}
/* Not complete yet, but it does check for the correct date format */
int validateStamp ( char * check ) {
    int to = 0, len = 0,  mi = 0, di = 0, index = 0;
    
    len = strlen(check);
    for (int x = 0; x < len; x++) {
        if (check[x] == 'T') {
            to++;
        }
    }
    /* We need to know where the date ends */
    if (to != 1) return 0;
   
    char month[2];
    char day[2];

    while (index < 4) {
        if ( !isdigit(check[index++]) )
            return 0;
    }
   
    while (index < 4 + 2) {
        if ( !isdigit(check[index]) )
            return 0;
        month[mi++] = check[index++];
    }
   
    while (index < 4 + 2 + 2) {
        if ( !isdigit(check[index]) )
            return 0;
        day[di++] = check[index++];
    }
    /* So at this point we have the thing */
    if (check[index] != 'T') return 0;
    index++;

    /* NO LEAP YEARS??? */
    /* Might have to check each month individually */
    if (month[0] >= '2' || (month[0] == '1' && month[1] >= '3') || (month[0] == '0' && month[1] == '0')) return 0;
    if ( (day[0] == '0' && day[1] == '0') || (day[0] == '3' && day[1] >= '2') || day[0] >= '4' ) return 0;

    if ( index + 6 > strlen(check) ) return 0;
    
    char hour[2] = { check[index], check[index + 1] };
    char minute[2] = { check[index + 2], check[index + 3] };
    char second[2] = { check[index + 4], check[index + 5] };
    /* UTC token and size of check */
    if ( index + 6 < strlen(check) ) 
        if (check[index + 6] != 'Z' || (index + 7) < strlen(check)) return 0;

    if ( (hour[0] == '2' && hour[1] >= '4') || (hour[0] >= '3') ) return 0;
    if ( minute[0] >= '6' ) return 0;
    if ( second[0] >= '6' ) return 0;
    /* For now */
    return 1;
}