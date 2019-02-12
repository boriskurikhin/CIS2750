#include "CalendarParser.h"
#include "LinkedListAPI.h"

int main(int argv, char ** argc) {
    if (argv != 2) return 0;
    Calendar * calendar;
    ICalErrorCode createCal = createCalendar(argc[1], &calendar);
    char * errorCode = printError(createCal);
    printf("Parse Status: %s\n\n\n", errorCode);
    if (strcmp(errorCode, "OK")) {
        free(errorCode);
        return 0;
    }
    char * output = printCalendar(calendar);
    if (output) {
        printf("%s", output);
    }
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