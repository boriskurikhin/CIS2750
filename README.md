# Calendar App - CIS*2750

This app was created for CIS*2750, a second year Software Systems Development & Integration course at University of Guelph.
This is an iCalendar application, meaning:

* It takes an .ics file as input, or lets you create your own
* Parses and validates the file
* Allows you to explore the file and modify some basic components (like Events)

# How it works (under the hood)

* Calendar parsing, validation, and modification is done using a *~2k loc* library written entriely in C 
* Users can upload or interact with existing calendars by hitting numerous API endpoints made with NodeJS
* Users can view Calendars, Events, and Alarms in the client made with HTML/CSS/JavaScript
* There was also a filtering component made with MySQL - but it is not included here for security purposes

Here's what it looks like

<img src="calendargif.gif" width="100%">
