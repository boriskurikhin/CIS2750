/* 31735 */

'use strict'

// C library API
const ffi = require('ffi');

// Express App (Routes)
const express = require("express");
const app     = express();
const path    = require("path");
const fileUpload = require('express-fileupload');

app.use(fileUpload());

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
let parserLib = ffi.Library("./parser/bin/libcal.so", {
  "createCalendar" : [ICalErrorCode, ["string", CalendarPtrPtr] ],
  "calendarToJSON" : ["string", [CalendarPtr]],
  "printError" : ["string", [ICalErrorCode]],
  "validateCalendar" : [ICalErrorCode, [CalendarPtr]],
  "eventListToJSONWrapper" : ["string", [CalendarPtr]],
  "alarmListToJSONWrapper" : ["string", [CalendarPtr, ref.types.int]]
});

function getCalendar(filename) {
  let calendar = ref.alloc(CalendarPtrPtr);
  let name = "./uploads/" + filename;
  let obj = parserLib.createCalendar(name, calendar);
  /* Validate that the Calendar parsed okay */
  if (parserLib.printError(obj) === 'OK' && parserLib.printError(parserLib.validateCalendar(calendar.deref())) === 'OK') {
    var retObj = JSON.parse(parserLib.calendarToJSON(calendar.deref()));
    retObj['eventList'] = JSON.parse(parserLib.eventListToJSONWrapper(calendar.deref()));
    for (var i = 0; i < retObj['eventList'].length; i++) {
      retObj['eventList'][i]['alarms'] = JSON.parse(parserLib.alarmListToJSONWrapper(calendar.deref(), i + 1));
    }
    return retObj;
  } else {
    /* Else return a NULL */
    return null;
  }
}

function getError(filename) {
  let calendar = ref.alloc(CalendarPtrPtr);
  let name = "./uploads/" + filename;
  let obj = parserLib.createCalendar(name, calendar);
  /* Validate that the Calendar parsed okay */
  if (parserLib.printError(obj) !== 'OK') {
    return parserLib.printError(obj);
  } else {
    return parserLib.printError(parserLib.validateCalendar(calendar.deref()));
  }
}


// Send HTML at root, do not change
app.get('/',function(req,res){
  res.sendFile(path.join(__dirname+'/public/index.html'));
});

// Send Style, do not change
app.get('/style.css',function(req,res){
  //Feel free to change the contents of style.css to prettify your Web app
  res.sendFile(path.join(__dirname+'/public/style.css'));
});

// Send obfuscated JS, do not change
app.get('/index.js',function(req,res){
  fs.readFile(path.join(__dirname+'/public/index.js'), 'utf8', function(err, contents) {
    const minimizedContents = JavaScriptObfuscator.obfuscate(contents, {compact: true, controlFlowFlattening: true});
    res.contentType('application/javascript');
    res.send(minimizedContents._obfuscatedCode);
  });
});

//Respond to POST requests that uploads files to uploads/ directory
app.post('/uploads', function(req, res) {
  if(!req.files) {
    return res.status(400).send('No files were uploaded.');
  }

  let uploadFile = req.files.uploadFile;

// Use the mv() method to place the file somewhere on your server
  uploadFile.mv('uploads/' + uploadFile.name, function(err) {
    /* Test the file */
    let errCode = getError(uploadFile.name);
    if ( err ||  errCode !== 'OK') {
      console.log(errCode);
      return res.status(500).send(errCode);
    }

    res.redirect('/');
  });

});

//Respond to GET requests for files in the uploads/ directory
app.get('/uploads/:name', function(req , res){
  fs.stat('uploads/' + req.params.name, function(err, stat) {
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
