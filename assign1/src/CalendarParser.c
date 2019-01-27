#include "CalendarParser.h"
#include "LinkedListAPI.h"
#include <ctype.h>
#include <strings.h>
#include <limits.h>
#define DEBUG 1

/* 
    Name: Boris Skurikhin
    ID: 1007339
*/

Calendar * calendar;

/* Function definitions */
char ** readFile (char * , int *, ICalErrorCode *);
char ** findProperty(char ** file, int beginIndex, int endIndex, char * propertyName, bool once, int * count, ICalErrorCode *);
char * getToken ( char * entireFile, int * index, ICalErrorCode *);
void trim (char ** );
ICalErrorCode checkFormatting( char ** entireFile, int numLines);
char ** getAllPropertyNames (char ** file, int beginIndex, int endIndex, int * numElements, int * errors);
int validateStamp ( char * check );
Alarm * createAlarm(char ** file, int beginIndex, int endIndex);

/* End of function definitions */ 

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
    char * errorCode = printError(createCal);
    printf("Parse Status: %s\n\n\n", errorCode);
    if (strcmp(errorCode, "OK")) {
        free(errorCode);
        return 0;
    }

    char * output = printCalendar(calendar);
    printf("%s", output);
    #if DEBUG
        FILE * out = fopen("output.txt", "w");
        fprintf(out, "%s", output);
        fclose(out);
    #endif
    free(output);
    free(errorCode);
    deleteCalendar(calendar);

    return 0;
}
char* printError(ICalErrorCode err) {
    char * result = (char *) calloc ( 1, 500);
    switch ( err ) {
        case OK:
            strcpy(result, "OK");
        break;
        case INV_FILE:
            strcpy(result, "INV_FILE");
        break;
        case INV_CAL:
            strcpy(result, "INV_CAL");
        break;
        case INV_VER:
            strcpy(result, "INV_VER");
        break;
        case DUP_VER:
            strcpy(result, "DUP_VER");
        break;
        case INV_PRODID:
            strcpy(result, "INV_PRODID");
        break;
        case DUP_PRODID:
            strcpy(result, "DUP_PRODID");
        break;
        case INV_EVENT:
            strcpy(result, "INV_EVENT");
        break;
        case INV_DT:
            strcpy(result, "INV_DT");
        break;
        case INV_ALARM:
            strcpy(result, "INV_ALARM");
        break;
        case WRITE_ERROR:
            strcpy(result, "WRITE_ERROR");
        break;
        default:
            strcpy(result, "OTHER_ERROR");
    }
    return result;
}
ICalErrorCode createCalendar(char* fileName, Calendar** obj) {
    int numLines = 0, correctVersion = 1, fnLen = 0;
    /* If the filename is null, or the string is empty */
    if (fileName == NULL || strcasecmp(fileName, "") == 0) return INV_FILE;
    /* If the calendar object doesn't point to anything */
    if (obj == NULL) return OTHER_ERROR;

    /* Check file extension */
    fnLen = strlen(fileName);
    if (toupper(fileName[fnLen - 1]) != 'S' || toupper(fileName[fnLen - 2]) != 'C' || toupper(fileName[fnLen - 3]) != 'I' || fileName[fnLen - 4] != '.') {
        return INV_FILE;
    }

    /* Attempt to open the file */
    FILE * calendarFile = fopen(fileName, "r");

    /* See if it's NULL */
    if (calendarFile == NULL) return INV_FILE;

    //At this point we already know the file exists, and is valid.
    fclose( calendarFile );
    ICalErrorCode __error__ = OK;
    /* Assuming everyt1hing has worked */
    char ** entire_file = readFile ( fileName, &numLines, &__error__);

    if (entire_file == NULL) return __error__;
    /* Does not begin or end properly */
    if (strcasecmp(entire_file[0], "BEGIN:VCALENDAR\r\n") || strcasecmp(entire_file[numLines - 1], "END:VCALENDAR\r\n")) {
        #if DEBUG
            printf("The file doesn't follow the correct start/end protocol. Exiting.\n");
        #endif
        for (int i = 0; i < numLines; i++) free(entire_file[i]);
        free(entire_file);
        /* Free each array, and then free the whole thing itself */
        return INV_CAL;
    }

    /* Validates the formatting of the file */
    ICalErrorCode formatCheck = checkFormatting(entire_file, numLines);
    if ( formatCheck != OK ) {
        #if DEBUG
            printf("It seems that the layout of the file events & alarms is incorrect\n");
        #endif
        for (int i = 0; i < numLines; i++) free(entire_file[i]);
        free(entire_file);
        return formatCheck;
    }

    __error__ = OK;
    *obj = (Calendar *) calloc ( 1, sizeof ( Calendar ) );
    /* Raw values */
    int vcount = 0, pcount = 0;
    char ** version = findProperty(entire_file, 0, numLines, "VERSION:", true, &vcount, &__error__);

    /* Trim any whitespace */
    /* Check to make sure that version is a number */
    if (version == NULL || strlen(version[0]) == 0) {
        #if DEBUG
            printf("Error: Version is NULL\n");
        #endif
        /* Free memory */
        for (int i = 0; i < numLines; i++) free(entire_file[i]);
        free(entire_file);
        deleteCalendar(*obj);
        *obj = NULL;
        
        if (__error__ == OTHER_ERROR) return DUP_VER;

        return INV_CAL;
    }
    
    char ** prodId = findProperty(entire_file, 0, numLines, "PRODID:", true, &pcount, &__error__);
    /* Trim whitespace */
    trim(&version[0]);
    /* Questionable */
    if (prodId == NULL || version == NULL || strlen(version[0]) == 0 || strlen(prodId[0]) == 0) {
        #if DEBUG
            printf("Error: Either version is empty after trimming, or prodId is empty.\n");
        #endif
        if (prodId) { free(prodId[0]); free(prodId); }
        if (version) { free(version[0]); free(version); } 
        for (int i = 0; i < numLines; i++) free(entire_file[i]);
        free(entire_file);
        deleteCalendar(*obj);
        *obj = NULL;
        if (__error__ == OTHER_ERROR) return DUP_PRODID;
        return INV_CAL;
    }
    /* It's not a number, cannot be represented by a float */
    for (int i = 0; i < strlen(version[0]); i++) {
        if ( (version[0][i] >= '0' && version[0][i] <= '9') || version[0][i] == '.'  || version[0][i] == '\r' || version[0][i] == '\n') continue;
        correctVersion = 0;
    }
    /* Basically, the version is not a number */
    if (!correctVersion) {
        #if DEBUG
            printf("Error: Version is not a floating point\n");
        #endif
        if (prodId) { free(prodId[0]); free(prodId); }
        if (version) { free(version[0]); free(version); } 
        for (int i = 0; i < numLines; i++) free(entire_file[i]);
        free(entire_file);
        deleteCalendar(*obj);
        *obj = NULL;
        return INV_VER;
    }
    /* Write the version & production id */
    (*obj)->version = (float) atof(version[0]);
    strcpy((*obj)->prodID, prodId[0]);

    /* Free */
    if (prodId) { free(prodId[0]); free(prodId); }
    if (version) { free(version[0]); free(version); } 

    /* After we're done extracting the version and prodId, we can begin extracting all other calendar properties */
    int N = 0, errors = 0;
    char ** calendarProperties = getAllPropertyNames(entire_file, 0, numLines, &N, &errors);
    /* If any errors occured */

    if (errors || !N) {
        for (int x = 0; x < N; x++) free(calendarProperties[x]);
        free(calendarProperties);
        for (int i = 0; i < numLines; i++) free(entire_file[i]);
        free(entire_file);
        deleteCalendar(*obj);
        *obj = NULL;
        return INV_CAL;
    }

    (*obj)->properties = initializeList(printProperty, deleteProperty, compareProperties);

    /* Grab all calendar properties */
    for (int x = 0; x < N; x++) {
        /* We already checked those guys */
        if (strcasecmp(calendarProperties[x], "PRODID:") && strcasecmp(calendarProperties[x], "VERSION:")) {
            int count = 0;
            /* All of these properties can have more than one value */
            __error__ = OK;
            char ** value = findProperty(entire_file, 0, numLines, calendarProperties[x], false, &count, &__error__);
            /* Then there is an error */
            if (value == NULL) {
                for (int x = 0; x < N; x++) free(calendarProperties[x]);
                free(calendarProperties);
                for (int i = 0; i < numLines; i++) free(entire_file[i]);
                free(entire_file);
                deleteCalendar(*obj);
                *obj = NULL;
                return INV_CAL;
            }
            for (int c = 0; c < count; c++) {
                Property * prop = NULL;
                prop = (Property *) calloc ( 1, sizeof (Property) + sizeof(char) * (1 + strlen(value[c])));
                int j = 0;
                /* We don't want to include the delimeter */
                for ( ; j < strlen(calendarProperties[x]) - 1; j++)
                    prop->propName[j] = calendarProperties[x][j];
                prop->propName[j] = '\0';
                strcpy(prop->propDescr, value[c]);
                insertBack((*obj)->properties, prop);
            }
            /* Free entire array */
            for (int c = 0; c < count; c++)
                free(value[c]);
            free(value);
        }
        /* Free the element */
        free(calendarProperties[x]);
    }
    /* Free the 2d array */
    free(calendarProperties);

    /* Allocate some memory for the events */
    (*obj)->events = initializeList(printEvent, deleteEvent, compareEvents);
    int eventCount = 0;

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
            char ** UID = NULL;
            int uidc = 0;
            __error__ = OK;
            UID = findProperty(entire_file, beginLine, numLines, "UID:", true, &uidc, &__error__);
            
            if (UID == NULL || !strlen(UID[0])) {
                if (UID) { free(UID[0]); free(UID); } /* empty string or something */
                for (int i = 0; i < numLines; i++) free(entire_file[i]);
                free(entire_file);
                #if DEBUG
                    printf("Error! Some event is missing a UID\n");
                #endif
                deleteCalendar(*obj);
                *obj = NULL;
                return INV_EVENT;
            }

            char ** DTSTAMP = NULL;
            int dtc = 0;
            DTSTAMP = findProperty(entire_file, beginLine, numLines, "DTSTAMP:", true, &dtc, &__error__);

            /* OLD TZID check */
            /*if (DTSTAMP == NULL || !strlen(DTSTAMP[0])) {
                if (DTSTAMP) { free(DTSTAMP[0]); free(DTSTAMP); }
                DTSTAMP = findProperty(entire_file, beginLine, numLines, "DTSTAMP;", true, &dtc, &__error__);

                if (DTSTAMP == NULL || !strlen(DTSTAMP[0])) {
                    if (DTSTAMP) { free(DTSTAMP[0]); free(DTSTAMP); }
                    if (UID) { free(UID[0]); free(UID); }
                    for (int i = 0; i < numLines; i++) free(entire_file[i]);
                    free(entire_file);
                    #if DEBUG
                        printf("Error! Some event is missing a DTSTAMP or the DTSTAMP format is invalid..\n");
                    #endif
                    deleteCalendar(*obj);
                    obj = NULL;
                    return INV_EVENT;
                }

                int quote = 0, l = 0, colonIndex = -1;
                // Find the index of the colon 
                for (; l < strlen(DTSTAMP[0]); l++) {
                    if (DTSTAMP[0][l] == '"') quote++;
                    if (DTSTAMP[0][l] == ':' && quote == 2) {
                        colonIndex = l;
                        break;
                    }
                }
                // The parameter was broken 
                if (quote != 2 || colonIndex < 0) {
                    if (DTSTAMP) { free(DTSTAMP[0]); free(DTSTAMP); }
                    if (UID) { free(UID[0]); free(UID); }
                    for (int i = 0; i < numLines; i++) free(entire_file[i]);
                    free(entire_file);
                    #if DEBUG
                        printf("Error! The time zone parameter is invalid.\n");
                    #endif
                    deleteCalendar(*obj);
                    obj = NULL;
                    return INV_DT;
                }
                // Essentially we just ignore the timezone parameter
                char * newDate = (char *) calloc ( 1, strlen(DTSTAMP[0]) - colonIndex + 1);
                for (int vi = colonIndex + 1, ii = 0; vi < strlen(DTSTAMP[0]); vi++, ii++)
                    newDate[ii] = DTSTAMP[0][vi];
                strcpy(DTSTAMP[0], newDate);
                free(newDate);
            } */
            if (DTSTAMP == NULL || !strlen(DTSTAMP[0])) {
                if (DTSTAMP) { free(DTSTAMP[0]); free(DTSTAMP); }
                __error__ = OK;
                DTSTAMP = findProperty(entire_file, beginLine, numLines, "DTSTAMP;", true, &dtc, &__error__);
                if (DTSTAMP && strlen(DTSTAMP[0])) {
                    free(DTSTAMP[0]); free(DTSTAMP);
                    if (UID) { free(UID[0]); free(UID); }
                    for (int i = 0; i < numLines; i++) free(entire_file[i]);
                    free(entire_file);
                    #if DEBUG
                        printf("Error! The dtstamp property was not found!\n");
                    #endif
                    deleteCalendar(*obj);
                    *obj = NULL;
                    return __error__ == OTHER_ERROR ? INV_EVENT : INV_DT;
                }
                if (DTSTAMP) { free(DTSTAMP[0]); free(DTSTAMP); }
                if (UID) { free(UID[0]); free(UID); }
                for (int i = 0; i < numLines; i++) free(entire_file[i]);
                free(entire_file);
                #if DEBUG
                    printf("Error! The dtstamp property was not found!\n");
                #endif
                deleteCalendar(*obj);
                *obj = NULL;
                return INV_EVENT;
            }
            /* If it was found, but it's invalid */
            if (!validateStamp(DTSTAMP[0])) {
                if (DTSTAMP) { free(DTSTAMP[0]); free(DTSTAMP); }
                if (UID) { free(UID[0]); free(UID); }
                for (int i = 0; i < numLines; i++) free(entire_file[i]);
                free(entire_file);
                #if DEBUG
                    printf("Error! The time zone format is invalid.\n");
                #endif
                deleteCalendar(*obj);
                obj = NULL;
                return INV_DT;
            }
    
            char ** DTSTART = NULL;
            int dts = 0;
            __error__ = OK;
            DTSTART = findProperty(entire_file, beginLine, numLines, "DTSTART:", true, &dts, &__error__);
            /* TZID check */
            /*
            if (DTSTART == NULL || !strlen(DTSTART[0])) {
                if (DTSTART) { free(DTSTART[0]); free(DTSTART); }
                dts = 0;
                DTSTART = findProperty(entire_file, beginLine, numLines, "DTSTART;", true, &dts, &__error__);
                // If it's still bad, off it
                if (DTSTART == NULL || !strlen(DTSTART[0])) {
                    if (DTSTAMP) { free(DTSTAMP[0]); free(DTSTAMP); }
                    if (UID) { free(UID[0]); free(UID); }
                    for (int i = 0; i < numLines; i++) free(entire_file[i]);
                    free(entire_file);
                    #if DEBUG
                        printf("Error! Some event is missing a DTSTART or the DTSTART format is invalid..\n");
                    #endif
                    deleteCalendar(*obj);
                    obj = NULL;
                    return INV_EVENT;
                }
                // Here we know that we have a timezone thing, and we need to extract it 
                int quote = 0, l = 0, colonIndex = -1;
                // Find the index of the colon 
                for (; l < strlen(DTSTART[0]); l++) {
                    if (DTSTART[0][l] == '"') quote++;
                    if (DTSTART[0][l] == ':' && quote == 2) {
                        colonIndex = l;
                        break;
                    }
                }
                // The parameter was broken 
                if (quote != 2 || colonIndex < 0) {
                    free(DTSTART[0]);
                    free(DTSTART);
                    if (DTSTAMP) { free(DTSTAMP[0]); free(DTSTAMP); }
                    if (UID) { free(UID[0]); free(UID); }
                    for (int i = 0; i < numLines; i++) free(entire_file[i]);
                    free(entire_file);
                    #if DEBUG
                        printf("Error! The time zone parameter is invalid.\n");
                    #endif
                    deleteCalendar(*obj);
                    obj = NULL;
                    return INV_DT;
                }
                // Essentially we just ignore the timezone parameter 
                char * newDate = (char *) calloc ( 1, strlen(DTSTART[0]) - colonIndex + 1);
                for (int vi = colonIndex + 1, ii = 0; vi < strlen(DTSTART[0]); vi++, ii++)
                    newDate[ii] = DTSTART[0][vi];
                strcpy(DTSTART[0], newDate);
                free(newDate);
            } */
            if (DTSTART == NULL || strlen(DTSTART[0]) == 0) {
                if (DTSTAMP) { free(DTSTAMP[0]); free(DTSTAMP); }
                if (DTSTART) { free(DTSTART[0]); free(DTSTART); }
                __error__ = OK;
                DTSTART = findProperty(entire_file, beginLine, numLines, "DTSTART;", true, &dts, &__error__);
                if (DTSTART && strlen(DTSTART[0])) {
                    free(DTSTART[0]); free(DTSTART);
                    if (UID) { free(UID[0]); free(UID); }
                    for (int i = 0; i < numLines; i++) free(entire_file[i]);
                    free(entire_file);
                    #if DEBUG
                        printf("Error! START DATE/TIME property was not found.\n");
                    #endif
                    deleteCalendar(*obj);
                    *obj = NULL;
                    return __error__ == OTHER_ERROR ? INV_FILE : INV_DT;
                }
                if (DTSTART) { free(DTSTART[0]); free(DTSTART); }
                if (UID) { free(UID[0]); free(UID); }
                for (int i = 0; i < numLines; i++) free(entire_file[i]);
                free(entire_file);
                #if DEBUG
                    printf("Error! START DATE/TIME property was not found.\n");
                #endif
                deleteCalendar(*obj);
                *obj = NULL;
                return INV_EVENT;
            }
            /* We found it, but it's not valid */
            if (!validateStamp(DTSTART[0])) {
                if (DTSTAMP) { free(DTSTAMP[0]); free(DTSTAMP); }
                if (DTSTART) { free(DTSTART[0]); free(DTSTART); }
                if (UID) { free(UID[0]); free(UID); }
                for (int i = 0; i < numLines; i++) free(entire_file[i]);
                free(entire_file);
                #if DEBUG
                    printf("Error! The time zone format is invalid.\n");
                #endif
                deleteCalendar(*obj);
                *obj = NULL;
                return INV_DT;
            }

            Event * newEvent = NULL;
            newEvent = (Event *) calloc ( 1, sizeof(Event) );
            newEvent->properties = initializeList(printProperty, deleteProperty, compareProperties);
            /* Writing the date & time into their respective slots */
            int di = 0, ti = 0, index = 0;

            bool isUTC = (DTSTAMP[0][strlen(DTSTAMP[0]) - 1] == 'Z' ? true : false);
            for ( ; index < (isUTC ? strlen(DTSTAMP[0]) - 1 : strlen(DTSTAMP[0])); index++) {
                if (DTSTAMP[0][index] != 'T') {
                    if (index < 8) newEvent->creationDateTime.date[di++] = DTSTAMP[0][index];
                    else newEvent->creationDateTime.time[ti++] = DTSTAMP[0][index];
                }
            }
            newEvent->creationDateTime.UTC = isUTC;

            /* Start date is also required apparently */

            di = ti = index = 0;
            isUTC = (DTSTART[0][strlen(DTSTART[0]) - 1] == 'Z' ? true : false);
            for ( ; index < (isUTC ? strlen(DTSTART[0]) - 1 : strlen(DTSTART[0])); index++) {
                if (DTSTART[0][index] != 'T') {
                    if (index < 8) newEvent->startDateTime.date[di++] = DTSTART[0][index];
                    else newEvent->startDateTime.time[ti++] = DTSTART[0][index];
                }
            }
            
            newEvent->startDateTime.UTC = isUTC;

            /* Append null terminators */
            strcat(newEvent->creationDateTime.date, "\0");
            strcat(newEvent->creationDateTime.time, "\0");

            strcat(newEvent->startDateTime.date, "\0");
            strcat(newEvent->startDateTime.time, "\0");
            
            strcpy(newEvent->UID, UID[0]);

            if (DTSTAMP) { free(DTSTAMP[0]); free(DTSTAMP); };
            if (DTSTART) { free(DTSTART[0]); free(DTSTART); }
            if (UID) { free(UID[0]); free(UID); }

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
                deleteCalendar(*obj);
                *obj = NULL;
                return INV_EVENT;
            }

            for (int x = 0; x < pCount; x++) {
                /* We already have this one */
                if (strcasecmp(eventProperties[x], "UID:") && strcasecmp(eventProperties[x], "DTSTAMP:") && strcasecmp(eventProperties[x], "DTSTAMP;") && strcasecmp(eventProperties[x], "DTSTART:") && strcasecmp(eventProperties[x], "DTSTART;")) {
                    
                    int valueCount = 0;
                    char ** value = findProperty(entire_file, beginLine, numLines, eventProperties[x], false, &valueCount, &__error__);
                    
                    if (value == NULL) {
                        for (int x = 0; x < pCount; x++) free(eventProperties[x]);
                        free(eventProperties);
                        for (int i = 0; i < numLines; i++) free(entire_file[i]);
                        free(entire_file);
                        deleteCalendar(*obj);
                        *obj = NULL;
                        return INV_EVENT;
                    }

                    for (int vc = 0; vc < valueCount; vc++) {
                        /* Basically, incorrect value */
                        if (!value || !strlen(value[vc])) {
                            #if DEBUG
                                printf("Error! While retrieving value of event property\n");
                            #endif
                            for (int j = 0; j < valueCount; j++) free(value[j]);
                            free(value);
                            for (int j = 0; j < pCount; j++) free(eventProperties[j]);
                            free(eventProperties);
                            for (int j = 0; j < numLines; j++) free(entire_file[j]);
                            free(entire_file);
                            deleteCalendar(*obj);
                            *obj = NULL;
                            return INV_EVENT;
                        }

                        Property * prop = (Property *) calloc (1, sizeof(Property) + strlen(value[vc]) + 1);
                        int j = 0;
                        for (; j < strlen(eventProperties[x]) - 1; j++)
                            prop->propName[j] = eventProperties[x][j];
                        prop->propName[j] = '\0';
                        strcpy(prop->propDescr, value[vc]);
                        insertBack(newEvent->properties, prop);
                    }
                    /* Free */
                    for (int vc = 0; vc < valueCount; vc++) free(value[vc]);
                    if (value) free(value); 
                }
                /* Free it after using it */
                free(eventProperties[x]);
            }
            free(eventProperties);

            /* Once we're done reading in the properties, we can begin reading in ALARMS */
            newEvent->alarms = initializeList(printAlarm, deleteAlarm, compareAlarms);
            for (int ai = beginLine + 1; ai < endLine; ai++) {
                if (!strcasecmp(entire_file[ai], "BEGIN:VALARM\r\n")) {
                    Alarm * alarm = createAlarm(entire_file, ai, endLine);
                    if (alarm == NULL) {
                        freeList(newEvent->alarms);
                        freeList(newEvent->properties);
                        free(newEvent);
                        for (int i = 0; i < numLines; i++) free(entire_file[i]);
                        free(entire_file);
                        #if DEBUG
                            printf("Error! Something wrong with the alarm.\n");
                        #endif
                        deleteCalendar(*obj);
                        *obj = NULL;
                        return INV_ALARM;
                    }
                    insertBack(newEvent->alarms, alarm);
                }
            }

            /* Insert the event at the back of the queue in the calendar object */
            insertBack((*obj)->events, newEvent);
            eventCount++;
            /* Once we're done */
            beginLine = endLine + 1;
        } else {
            beginLine++;
        }
    }

    for (int i = 0; i < numLines; i++) free(entire_file[i]);
    free(entire_file);

    /* There are no events */
    if (!eventCount) {
        #if DEBUG
            printf("There are no events in the calendar..");
        #endif
        deleteCalendar(*obj);
        *obj = NULL;
        return INV_CAL;
    }

    return OK;
}
/* Deletes the calendar */
void deleteCalendar(Calendar * obj) {
    if (obj == NULL) return;
    if (obj->properties) freeList( obj->properties );
    if (obj->events) freeList( obj->events );
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
char * getToken ( char * entireFile, int * index, ICalErrorCode * errorCode) {
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
                    *errorCode = INV_FILE;
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
            *errorCode = INV_FILE;
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
    *errorCode = INV_FILE;
    #if DEBUG
        printf("Was not able to tokenize string, for unknown reason.\n");
    #endif
    if (line) free(line);
    *index = -1;
    return NULL;
}
/* Reads an entire file, tokenizes it and exports an array of lines */
char ** readFile ( char * fileName, int * numLines, ICalErrorCode * errorCode ) {
    /* Local variables */
    FILE * calendarFile = fopen(fileName, "r");
    *errorCode = OK;

    /* Even though we already checked for this, might as well just do this again */
    if (calendarFile == NULL) {
        *errorCode = INV_FILE;
        return NULL;
    }

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
        *errorCode = INV_FILE;
        return NULL;
    }
    /* Prepare the array */
    index = 0;
    /* Tokenizes the String */
    while ( 1 ) {
        char * token = getToken(entireFile, &index, errorCode);
        /* -1 means that something was bad */
        if (token == NULL && index != -1) break;
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

        *errorCode = OK;

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
    *errorCode = OK;

    free(entireFile);
    *numLines = numTokens;
    /* END */
    return returnFile;
}
/* Extracts and returns an array of properties */
/* [Begin Index, End Index) */
char ** findProperty(char ** file, int beginIndex, int endIndex, char * propertyName, bool once, int * count, ICalErrorCode * __error__) {
    int li = beginIndex, opened = 0, closed = 0, currentlyFolding = 0, index = 0;
    char * result = NULL;

    /* Basically, returns an array of results */
    char ** results = NULL;
    int size = 0;

    int foundCount = 0;
    /* Doesn't start properly */
    if ( strcasecmp(file[li], "BEGIN:VCALENDAR\r\n") && strcasecmp(file[li], "BEGIN:VALARM\r\n") && strcasecmp(file[li], "BEGIN:VEVENT\r\n") )
        return NULL;
    while (li < endIndex) {
        if ( !strcasecmp(file[li], "BEGIN:VCALENDAR\r\n") || !strcasecmp(file[li], "BEGIN:VALARM\r\n") || !strcasecmp(file[li], "BEGIN:VEVENT\r\n") ) {
            opened++;
        } else if ( !strcasecmp(file[li], "END:VCALENDAR\r\n") || !strcasecmp(file[li], "END:VALARM\r\n") || !strcasecmp(file[li], "END:VEVENT\r\n") ) {
            closed++;
            if (closed == opened) {
                /* If we have reached the end of the current level */
                if (once) {
                    if (foundCount == 1) {
                        /* Whatever the first result was */
                        if (result) free(result);
                        return results;
                    } else {
                        if (result) free(result);
                        for (int j = 0; j < size; j++) free(results[j]);
                        if (results) free(results);
                        *count = 0;
                        *__error__ = OTHER_ERROR; /* duplicate */
                        return NULL;
                    }
                } else {
                    if (result) free(result);
                    *count = size;
                    return results;
                }
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
                            /* If it's empty or something */
                            if (result == NULL || strlen(result) == 0) {
                                if (result) free(result);
                                for (int j = 0; j < size; j++) free(results[j]);
                                if (results) free(results);
                                *count = 0;
                                return NULL;
                            }
                            foundCount++;
                            /* Dynamic allocation */
                            if (results == NULL) {
                                results = calloc ( 1, sizeof (char *) );
                            } else {
                                results = realloc ( results, (size + 1) * sizeof(char *) );
                            }
                            results[size] = (char *) calloc(1, strlen(result) + 1);
                            /* Essentially copy whatever into the 2d array, and re-set the string */
                            strcpy(results[size], result);
                            strcpy(result, "");
                            index = 0;
                            currentlyFolding = 0;
                            size++;
                        }
                    } else {
                        if (once) {
                            if (foundCount == 1) {
                                if (result) free(result);
                                return results;
                            } else {
                                if (result) free(result);
                                for (int j = 0; j < size; j++) free(results[j]);
                                if (results) free(results);
                                *count = 0;
                                *__error__ = OTHER_ERROR;
                                return NULL;
                            }
                        } else {
                            if (result) free(result);
                            *count = size;
                            return results;
                        }
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
                                    foundCount++;
                                    /* If it's empty or something */
                                    if (result == NULL || strlen(result) == 0) {
                                        if (result) free(result);
                                        for (int j = 0; j < size; j++) free(results[j]);
                                        if (results) free(results);
                                        *count = 0;
                                        return NULL;
                                    }
                                    /* Dynamic allocation */
                                    if (results == NULL) {
                                        results = calloc ( 1, sizeof (char *) );
                                    } else {
                                        results = realloc ( results, (size + 1) * sizeof(char *) );
                                    }
                                    results[size] = (char *) calloc(1, strlen(result) + 1);
                                    /* Essentially copy whatever into the 2d array, and re-set the string */
                                    strcpy(results[size], result);
                                    strcpy(result, "");
                                    index = 0;
                                    currentlyFolding = 0;
                                    size++;
                                }
                            } else {
                                if (once) {
                                    if (foundCount == 1) {
                                        if (result) free(result);
                                        *count = size;
                                        return results;
                                    } else {
                                        if (result) free(result);
                                        for (int j = 0; j < size; j++) free(results[j]);
                                        if (results) free(results);
                                        *count = 0;
                                        return NULL;
                                    }
                                } else {
                                    if (result) free(result);
                                    *count = size;
                                    return results;
                                }
                            }
                        }
                    }
                }
            }
        }
        li++;
    }
    /* NOT too sure what to do here so we just did the basic clean up */
    if (result) free(result);
    for (int j = 0; j < size; j++) free(results[j]);
    if (results) free(results);
    *count = 0;
    return NULL;
}
/* Verifies that all alarms & events are distributed correctly */
ICalErrorCode checkFormatting (char ** entire_file, int numLines ) {
    if (numLines < 2) return INV_FILE;
    
    if (strcasecmp(entire_file[0], "BEGIN:VCALENDAR\r\n")) {
        return INV_CAL;
    }
    if (strcasecmp(entire_file[numLines - 1], "END:VCALENDAR\r\n")) {
        return INV_CAL;
    }

    int isEventOpen = 0;
    int isAlarmOpen = 0;
    int numEventsOpened = 0, numEventsClosed = 0;
    int numAlarmsOpened = 0, numAlarmsClosed = 0;

    for (int i = 1; i < numLines - 1; i++) {
        if (strcasecmp(entire_file[i], "BEGIN:VEVENT\r\n") == 0) {
            if (isEventOpen) return INV_EVENT; /* There's an open event */
            if (isAlarmOpen) return INV_ALARM; /* There's an open alarm */

            isEventOpen = 1;
            numEventsOpened++;
        } else if (strcasecmp(entire_file[i], "END:VEVENT\r\n") == 0) {
            if (!isEventOpen) return INV_CAL; /* We have no ongoing events */
            if (isAlarmOpen) return INV_ALARM; /* There's an alarm open */
            if (numAlarmsOpened != numAlarmsClosed) {
                return INV_ALARM;
            }
            /* Each time we close an event we re-set alarms */
            numAlarmsOpened = numAlarmsClosed = 0;
            isEventOpen = 0;
            numEventsClosed++;
        } else if (strcasecmp(entire_file[i], "BEGIN:VALARM\r\n") == 0) {
            if (isAlarmOpen) return INV_ALARM;
            if (!isEventOpen) return INV_CAL;

            isAlarmOpen = 1;
            numAlarmsOpened++;
        } else if (strcasecmp(entire_file[i], "END:VALARM\r\n") == 0) {
            if (!isAlarmOpen) return INV_EVENT;
            if (!isEventOpen) return INV_CAL;

            isAlarmOpen = 0;
            numAlarmsClosed++;
        } else if (entire_file[i][0] != ' ' && entire_file[i][0] != '\t' && entire_file[i][0] != ';') {
            int lineLength = strlen(entire_file[i]);
            int colons = 0;
            for (int j = 1; j < lineLength; j++)
                colons += (entire_file[i][j] == ':' || entire_file[i][j] == ';');
            if (!colons) {
                if (isAlarmOpen) return INV_ALARM;
                if (isEventOpen) return INV_EVENT;
                return INV_CAL;
            }
        }
    }
    if (isAlarmOpen || numAlarmsOpened != numAlarmsClosed) return INV_ALARM;
    if (isEventOpen || numEventsOpened != numEventsClosed) return INV_EVENT;
    
    return OK;
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
    length += strlen(events) + 2;
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
                
                int repeat = 0;

                /* Make sure there's no duplicates */
                for (int check = 0; check < *numElements; check++) {
                    if (!strcasecmp(line, result[check])) {
                        repeat = 1;
                        free(line);
                        break;
                    }
                }

                if (!repeat) {
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
        }
        index++;
    }
    return result;
}
/* <-----HELPER FUNCTIONS------> */
/* EVENTS */
void deleteEvent(void* toBeDeleted) {
    Event * e = (Event *) toBeDeleted;
    if (e->properties) freeList(e->properties);
    if (e->alarms) freeList(e->alarms);
    free(e);
}
void deleteAlarm(void* toBeDeleted) {
    Alarm * a = (Alarm *) toBeDeleted;
    free(a->trigger);
    if (a->properties) freeList(a->properties);
    free(a);
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
    result = (char *) calloc(1, 22);
    strcpy(result, "\t=BEGIN EVENT=\n\tUID->");
    result = (char *) realloc( result, strlen(result) + strlen(event->UID) + 1);
    strcat(result, event->UID);
    result = (char *) realloc (result, strlen(result) + 12);
    strcat(result, "\n\tDTSTAMP->");
    result = (char *) realloc (result, strlen(result) + strlen(event->creationDateTime.date) + strlen(event->creationDateTime.time) + 1);
    strcat(result, event->creationDateTime.date);
    if (strlen(event->creationDateTime.time))
        strcat(result, event->creationDateTime.time);
    if (event->creationDateTime.UTC) {
        result = (char *) realloc (result, strlen(result) + 6);
        strcat(result, "(UTC)");
    }
    result = (char *) realloc(result, strlen(result) + strlen(event->startDateTime.date) + 12);
    strcat(result, "\n\tDTSTART->");
    strcat(result, event->startDateTime.date);
    if (strlen(event->startDateTime.time)) {
        result = (char *) realloc (result, strlen(result) + strlen(event->startDateTime.time) + 1);
        strcat(result, event->startDateTime.time);
    }
    if (event->startDateTime.UTC) {
        result = (char *) realloc (result, strlen(result) + 6);
        strcat(result, "(UTC)");
    }
    char * otherProps = NULL;
    otherProps = toString(event->properties);
    result = (char *) realloc (result, strlen(result) + strlen(otherProps) + 1);
    strcat(result, otherProps);
    free(otherProps);

    char * alarms = NULL;
    alarms = toString(event->alarms);
    result = (char *) realloc (result, strlen(result) + strlen(alarms) + 2);
    strcat(result, alarms);
    free(alarms);

    strcat(result, "\n");
    result = (char *) realloc(result, strlen(result) + 13);
    strcat(result, "\t=END EVENT=");

    return result;
}
char* printAlarm(void* toBePrinted) {
    Alarm * a = (Alarm *) toBePrinted;
    char * output = (char *) calloc ( 1, 16);
    strcpy(output, "\t=BEGIN ALARM=\n");
    output = (char *) realloc (output, strlen(output) + strlen(a->action) + 1 + 1 + 8 + 1 );
    strcat(output, "\tACTION->");
    strcat(output, a->action);
    strcat(output, "\n");
    output = (char *) realloc (output, strlen(output) + strlen(a->trigger) + 1 + 1 + 9 );
    strcat(output, "\tTRIGGER->");
    strcat(output, a->trigger);
    /* also properties */
    char * otherProps = NULL;
    otherProps = toString(a->properties);
    output = (char *) realloc (output, strlen(output) + strlen(otherProps) + 1);
    strcat(output, otherProps);
    
    free(otherProps);
    output = (char *) realloc(output, strlen(output) + 14);
    strcat(output, "\n\t=END ALARM=");
    return output;
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
int compareAlarms(const void * first, const void * second) {
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
    if (to != 1 && strlen(check) != 8) return 0;
   
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
    /* So at this point we know that the day is correct and that, 
    if the lenght is exactly 8 then the date is correct */
    if (strlen(check) == 8) return 1;

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
/* This creates an alarm object */
Alarm * createAlarm(char ** file, int beginIndex, int endIndex) {
    /* gotta make sure that the line begins correctly */
    if (strcasecmp(file[beginIndex], "BEGIN:VALARM\r\n")) return NULL;
    Alarm * alarm = (Alarm *) calloc ( 1, sizeof (Alarm) );
    alarm->properties = initializeList(printProperty, deleteProperty, compareProperties);
    /* Allocated memory for an alarm object */
    int numProps = 0, errors = 0;
    /* Grab alarm properties */
    char ** pnames = getAllPropertyNames(file, beginIndex, endIndex, &numProps, &errors);
    
    if (errors || !numProps) {
        #if DEBUG
            printf("Error! While retrieving properties of event (inside an alarm)\n");
        #endif
        for (int x = 0; x < numProps; x++) free(pnames[x]);
        if (pnames) free(pnames);
        freeList(alarm->properties);
        free(alarm);
        return NULL;
    }

    int ftrig = 0;
    int faction = 0;
    
    ICalErrorCode errorCode = OK;

    /* Run through props */
    for (int i = 0; i < numProps; i++) {
        /* We need to check some required ones here */
        int nump = 0;
        if (!strcasecmp(pnames[i], "ACTION:") || !strcasecmp(pnames[i], "ACTION;")) {
            char ** pval = findProperty(file, beginIndex, endIndex, pnames[i], true, &nump, &errorCode);
            /* Either not found, or bad, or more than one */
            if (pval == NULL) {
                for (int j = 0; j < numProps; j++) free(pnames[j]);
                free(pnames);
                freeList(alarm->properties);
                if (alarm->trigger) free(alarm->trigger);
                free(alarm);
                return NULL;
            }
            /* All goodie */
            strcpy(alarm->action, pval[0]);
            faction++;
            free(pval[0]);
            free(pval);
        } else if ( !strcasecmp (pnames[i], "TRIGGER:") || !strcasecmp(pnames[i], "TRIGGER;")) {
            char ** pval = findProperty(file, beginIndex, endIndex, pnames[i], true, &nump, &errorCode);
            if (pval == NULL) {
                for (int j = 0; j < numProps; j++) free(pnames[j]);
                free(pnames);
                freeList(alarm->properties);
                free(alarm);
                return NULL;
            }
            /* All goodie */
            alarm->trigger = (char *) calloc ( 1, strlen(pval[0]) + 1);
            strcpy(alarm->trigger, pval[0]);
            ftrig++;
            free(pval[0]);
            free(pval);
        } else {
            /* Handle others */
            char ** pval = findProperty(file, beginIndex, endIndex, pnames[i], false, &nump, &errorCode);
            if (pval == NULL) {
                for (int j = 0; j < numProps; j++) free(pnames[j]);
                free(pnames);
                freeList(alarm->properties);
                if (alarm->trigger) free(alarm->trigger);
                free(alarm);
                return NULL;
            }
            for (int j = 0; j < nump; j++) {
                Property * prop = (Property *) calloc ( 1, sizeof(Property) + strlen(pval[j]) + 1);

                strcpy(prop->propName, pnames[i]);
                strcpy(prop->propDescr, pval[j]);

                insertBack(alarm->properties, prop);
            }
            /* Free it, we don't need it anymore */
            for (int j = 0; j < nump; j++) free(pval[j]);
            free(pval);
        }
    }
    for (int i = 0; i < numProps; i++) free(pnames[i]);
    free(pnames);
    
    if (ftrig == 1 && faction == 1) return alarm;
    
    freeList(alarm->properties);
    free(alarm);
    return NULL;
}
/* Don't know about this one */
/*
ICalErrorCode validateCalendar(const Calendar* obj) {
    if (obj == NULL) return OTHER_ERROR;
    if (strlen(obj->prodID) == 0) return OTHER_ERROR;
    if (obj->events == NULL || obj->properties == NULL) return OTHER_ERROR;
    if (obj->events.length == 0 || obj->properties.length == 0) return OTHER_ERROR;
    return OK;
}
*/