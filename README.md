# Calendar App - CIS*2750

### How to install & run

Make sure to have `gcc`, `node` and `npm` installed prior to starting!

After you clone the repo:
 ```bash
 cd CalendarApp/
 npm install
 cd parser
 make
 cd ../
 ```
 If you are on **MacOS**, you have to do an extra step:
 ```bash
 cp libcal.so libcal.so.dylib
 cp liblist.so liblist.so.dylib
 ```
 Lastly (you can replace the 1337 with any port you want)
 ```bash
 node app.js 1337
 ```
 Then open up a browser and navigate to: `localhost:1337`!


This app was created for CIS*2750, a second year Software Systems Development & Integration course at University of Guelph.
This is an iCalendar application, meaning:

* It takes an .ics file as input, or lets you create your own
* Parses and validates the file
* Allows you to explore the file and modify some basic components (like Events)

Due to this being an assignment and done under a lot of time pressure, some code may be messy

### How it works (under the hood)

* Calendar parsing, validation, and modification is done using a [*~2k loc* library written entriely in C](https://github.com/boriskurikhin/CIS2750/blob/master/CalendarApp/parser/src/CalendarParser.c) 
* Users can upload or interact with existing calendars by hitting numerous [API endpoints made with NodeJS](https://github.com/boriskurikhin/CIS2750/blob/master/CalendarApp/app.js)
* Users can view Calendars, Events, and Alarms in the [client made with HTML/CSS/JavaScript](https://github.com/boriskurikhin/CIS2750/tree/master/CalendarApp/public)
* There was also a filtering component made with MySQL - but it is not included here for security purposes

### What it looks like

<img src="calendargif.gif" width="100%">
