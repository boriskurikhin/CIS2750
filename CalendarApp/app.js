/* 31735 */

'use strict'

// C library API
const ffi = require('ffi');

// Express App (Routes)
const express = require("express");
const app     = express();
const path    = require("path");
const fileUpload = require('express-fileupload');
const bodyParser = require('body-parser');

app.use(fileUpload());
app.use(bodyParser.urlencoded({
  extended: true
}));

// Minimization
const fs = require('fs');
const ref = require('ref');
const JavaScriptObfuscator = require('javascript-obfuscator');

// Important, pass in port as in `npm run dev 1234`, do not change
const portNum = process.argv[2];

let ICalErrorCode = ref.types.int;
let ICalErrorCodePtr = ref.refType(ICalErrorCode);
let Calendar = ref.types.void;
let CalendarPtr = ref.refType(Calendar);
let CalendarPtrPtr = ref.refType(CalendarPtr);

//Create the mapping from C
let parserLib = ffi.Library("./libcal.so", {
  "createCalendar" : [ICalErrorCode, ["string", CalendarPtrPtr] ],
  "calendarToJSON" : ["string", [CalendarPtr]],
  "printError" : ["string", [ICalErrorCode]],
  "validateCalendar" : [ICalErrorCode, [CalendarPtr]],
  "eventListToJSONWrapper" : ["string", [CalendarPtr]],
  "alarmListToJSONWrapper" : ["string", [CalendarPtr, ref.types.int]],
  "deleteCalendar" : [ref.types.void, [CalendarPtr]],
  "JSONtoEventWrapper": [ref.types.void, [CalendarPtr, "string"]],
  "writeCalendar" : [ICalErrorCode, ["string", CalendarPtr]]
});

function getCalendar(filename) {
  let calendar = ref.alloc(CalendarPtrPtr);
  let name = "./uploads/" + filename;
  let obj = parserLib.createCalendar(name, calendar);
  /* Validate that the Calendar parsed okay */
  if (getError(filename) === 'OK') {
    var retObj = JSON.parse(parserLib.calendarToJSON(calendar.deref()));
    retObj['eventList'] = JSON.parse(parserLib.eventListToJSONWrapper(calendar.deref()));
    for (var i = 0; i < retObj['eventList'].length; i++) {
      retObj['eventList'][i]['alarms'] = JSON.parse(parserLib.alarmListToJSONWrapper(calendar.deref(), i + 1));
    }
    parserLib.deleteCalendar(calendar.deref());
    return retObj;
  } else {
    /* Else return a NULL */
    return null;
  }
}

function addEventToCalendar(json) {
  let calendar = ref.alloc(CalendarPtrPtr);
  let name = "./uploads/" + json['filename'] + '.ics';
  let obj = parserLib.createCalendar(name, calendar);

  let filename = json['filename'] + '.ics';

  console.log(JSON.stringify(json));

  if (getError(filename) === 'OK') {
    delete json['filename'];
    parserLib.JSONtoEventWrapper(calendar.deref(), JSON.stringify(json));
    let writeStatus = parserLib.writeCalendar('uploads/' + filename, calendar.deref());
    parserLib.deleteCalendar(calendar.deref());
    /* Check */
    console.log(parserLib.printError(writeStatus));
    if (parserLib.printError(writeStatus) === 'OK') {
      console.log('Successfully added event and wrote calendar!');
      return 1;
    } else {
      return 0;
    }
  }
}

function getError(filename) {
  let calendar = ref.alloc(CalendarPtrPtr);
  let name = "./uploads/" + filename;
  let obj = parserLib.createCalendar(name, calendar);

  let errorCode = parserLib.printError(obj);

  if (errorCode === 'OK') {
    errorCode = parserLib.printError(parserLib.validateCalendar(calendar.deref()));
    if (errorCode === 'OK') {
      return 'OK';
    } else {
      return errorCode;
    }
  } else {
    return errorCode;
  }
}

// Send HTML at root, do not change
app.get('/',function(req,res){
  res.sendFile(path.join(__dirname+'/public/index.html'));
});

// Send obfuscated JS, do not change
app.get('/index.js',function(req,res){
  fs.readFile(path.join(__dirname + '/public/index.js'), 'utf8', function(err, contents) {
    const minimizedContents = JavaScriptObfuscator.obfuscate(contents, {compact: true, controlFlowFlattening: true});
    res.contentType('application/javascript');
    res.send(minimizedContents._obfuscatedCode);
  });
});

app.post('/addevent', function(req, res) {
  let json = req.body;
  if (addEventToCalendar(req.body) === 1) {
    res.status(200).send('Success!');
  } else {
    res.status(400).send('Could not write to file!');
  }
});

app.post('/create', function(req, res) {
  
  let json = req.body;

  var content = 'BEGIN:VCALENDAR\r\n';

  /* Basic shit */
  content += 'VERSION:' + json['version'] + '\r\n';
  content += 'PRODID:' + json['prodId'] + '\r\n';

  /* Event */
  let numEvents = parseInt(json['numEvents']);
  for (var i = 0; i < numEvents; i++) {
    content += 'BEGIN:VEVENT\r\n';

    content += 'UID:' + json['events'][i]['uid'] + '\r\n';
    content += 'DTSTAMP:' + json['events'][i]['dtstamp'] + '\r\n'; 
    content += 'DTSTART:' + json['events'][i]['dtstart'] + '\r\n';

    let numProps = parseInt(json['events'][i]['numProps']);
    for (let j = 0; j < numProps; j++) {
      content += json['events'][i]['props'][j]['propName'] + ':' + json['events'][i]['props'][j]['propDescr'] + '\r\n'; 
    }

    content += 'END:VEVENT\r\n';
  }
  content += 'END:VCALENDAR\r\n';

  fs.writeFile(path.join(__dirname + '/uploads/' + json['name'] + '.ics'), content, function(err) {
    
    if (err) {
      return res.status(400).send('Could not create file on the server!');
    }

    let createAttempt = getError(json['name'] + '.ics');

    if ( createAttempt !== 'OK') {
      fs.unlink(path.join(__dirname + '/uploads/' + json['name'] + '.ics'), function() { console.log('Attempted delete ' + json['name'])});
      return res.status(400).send(createAttempt);
    } else {
      return res.status(200).send('Nice bruv');
    }
  });

});

//Respond to POST requests that uploads files to uploads/ directory
app.post('/uploads', function(req, res) {
  if(!req.files) {
    return res.status(400).send('No files were uploaded.');
  }

  let uploadFile = req.files.uploadFile;

// Use the mv() method to place the file somewhere on your server
  uploadFile.mv(path.join(__dirname + '/uploads/' + uploadFile.name), function(err) {
    /* Test the file */
    let errCode = getError(uploadFile.name);
    if ( err ||  errCode !== 'OK') {
      fs.unlink(path.join(__dirname + '/uploads/' + uploadFile.name), function() {
        console.log('Attempted to delete ' + uploadFile.name);
      });
      return res.status(500).send(errCode);
    }

    res.redirect('/');
  });

});

//Respond to GET requests for files in the uploads/ directory
app.get('/uploads/:name', function(req , res){
  fs.stat(path.join(__dirname + '/uploads/' + req.params.name), function(err, stat) {
    console.log(err);
    if(err == null) {
      res.sendFile(path.join(__dirname+'/uploads/' + req.params.name));
    } else {
      res.send('');
    }
  });
});

//******************** Your code goes here ********************

//Sample endpoint

app.get('/getnumfiles', function(req, res) {
  fs.readdir('./uploads', (err, files) => {
    var calendars = [];
    let size = 0;
    files.forEach( file => {
        var json = getCalendar(file);
        if (json !== null) {
          size++;
          json['filename'] = file;
          calendars.push(json);
        }
    });
    res.send({
      numFiles: size,
      files: calendars
    });
  });
});

app.get('/getfile/:filename', function(req, res) {
  fs.readdir('./uploads', (err, files) => {
    var to_send = null;
    files.forEach( file => {
        var json = getCalendar(file);
        if (json !== null && json['filename'] === req.params.filename) {
          to_send = json;
        }
    });
    res.send({
      file: to_send
    });
  });
});

app.listen(portNum);
console.log('Running app at localhost: ' + portNum);
