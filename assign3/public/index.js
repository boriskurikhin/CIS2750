var fileCount = 1;
var calendars = {};

$(document).ready(function() {
  fileLog();
  $('select').formSelect();
  $('.datepicker').datepicker();
  $('.timepicker').timepicker();  
 
  /* This is where we will re-render the calendar */
  $('#caldropdown').on('change', function() {
    $('#calview').empty();
    let caltable = '<table class="responsive-table"><thead><th>Event No</th><th>Start Date</th><th>Start Time</th><th>Summary</th><th>Props</th><th>Alarms</th></thead>';
    caltable += '<tbody>';
    for (var i = 0; i < calendars[$(this).val()].length; i++) {
      var row = calendars[$(this).val()][i];
      caltable += '<tr class="grey lighten-5"><td>' + (1 + i) + '</td><td>' + formatDate(row['startDT']['date']) + '</td><td>' + formatTime(row['startDT']['time']) + (row['startDT']['isUTC'] ? ' (UTC)' : '') + '</td><td>' + row['summary'] + '</td><td><a style="cursor: pointer" onclick="$(\'.properties_' + (i+1) + '\').toggle()">' + row['numProps'] + '</a></td><td><a style="cursor: pointer" onclick="$(\'.alarms_' + (i+1) + '\').toggle()">' + row['numAlarms'] + '</a></td></tr>';
      caltable += '<tr style="display:none" class="grey lighten-1 properties_' + (i + 1) + '"><th></th><th>Prop Name</th><th>Prop Value</th><th></th><th></th><th></th></tr>';
      for (var j = 0; j < row['props'].length; j++) {
        var prop = row['props'][j];
        caltable += '<tr style="display: none" class="properties_' + (i+1) + '"><td></td><td>' + prop['propName'] + '</td><td>' + prop['propDescr'] + '</td><td></td><td></td></tr>';
      }
      caltable += '<tr style="display:none" class="grey lighten-1 alarms_' + (i + 1) + '"><th></th><th>Action</th><th>Trigger</th><th>Props</th><th></th><th></tr>';
      for (var j = 0; j < row['alarms'].length; j++) {
        var alarm = row['alarms'][j];
        caltable += '<tr style="display:none" class="alarms_' + (i + 1) + '"><td></td><td>' + alarm['action'] + '</td><td>' + alarm['trigger'] + '</td>' + '<td><a style="cursor: pointer" onclick="$(\'.alarmprops_' + (i+1) + '_' + (j+1) +'\').toggle()">' +  + alarm['numProps'] + '</a></td><td></td><td></td></tr>';
        /* Alarm properties */
        caltable += '<tr style="display:none" class="grey lighten-1 alarmprops_' + (i+1) + '_' + (j+1) + '"><th></th><th></th><th>Prop Name</th><th>Prop Value</th><th></th><th></th></tr>';
        for (var k = 0; k < row['alarms'][j]['props'].length; k++) {
          caltable += '<tr style="display:none" class="grey lighten-3 alarmprops_' + (i+1) + '_' + (j+1) + '"><td></td><td></td><td>' + row['alarms'][j]['props'][k]['propName'] + '</td><td>' + row['alarms'][j]['props'][k]['propDescr'] + '</td><td></td><td></td></tr>';
        }
      }
    }
    caltable += '</tbody></table>';
    $('#calview').append(caltable);
  });
});

function formatDate(datestring) {
  return datestring.substring(0, 4) + '/' + datestring.substring(4, 6) + '/' + datestring.substring(6);
}

function formatTime(timestring) {
  return timestring.substring(0, 2) + ':' + timestring.substring(2, 4) + ':' + timestring.substring(4);
}

$('#addevent').click( function() {
  let numEvents = $('#events').children('.event').length;
  let num = numEvents + 1;
  /* Rowdy */
  let eventText = '<div class="event event_' + num + ' grey lighten-5 z-depth-1"><div class="row"><div class="col s12 center-align"><h6>Event ' + num + '</h6></div></div>';
  eventText += '<div class="row"><form class="col s12"><div class="input-field col s8"><textarea placeholder="UID" id="event_' + num + '_uid" type="text" class="materialize-textarea"></textarea>';
  eventText += '<label for="event_' + num + '_uid">UID</label></div>';
  
  eventText += '<div class="input-field col s2"><input type="text" id="event_' + num + '_dtstart_date" class="datepicker">';
  eventText += '<label for="event_' + num + '_dtstart_date">Start Date</label></div>'
  
  eventText += '<div class="input-field col s2"><input type="text" id="event_' + num + '_dtstart_time" class="timepicker">';
  eventText += '<label for="event_' + num + '_dtstart_time">Start Time</label></div>'
  
  /* Deleting event */
  eventText += '</form></div>';
  eventText += '<div class="row"><div class="col s12 right-align"><button class="btn-flat" onclick="$(\'.event_' + num + '\').remove()">Delete Event</button></div></div>';
  
  /* Event props */
  eventText += '<div id="event_' + num + '_props"></div><div class="row"><div class="col s12 right-align"><button class="btn-flat" onclick="addprop(\'event_' + num + '_props\', ' + num + ')">ADD PROP</button></div></div>';

  eventText += '</div></div>';
  /* Finally..*/
  $('#events').append(eventText);
  /* Update the pickers */
  $('.datepicker').datepicker();
  $('.timepicker').timepicker();
});

function pushError(errorMsg, errorCode) {
  /* A visual to see if there are any erorrs */
  if ( $('#status').hasClass('green') ) {
    $('#status').toggleClass('green red');
  }
  $('#errorList').append('<li class="collection-item">' + errorMsg + ' Code: ' + '<b>' + errorCode + '!</b></li>');
}

$('#btnClear').click(function() {
  if ( $('#status').hasClass('red') ) {
    $('#status').toggleClass('green red');
  }
  $('#errorList').empty();
});

$('#createcalendar').click(function() {
  let name = $('#new_name').val();
  let version = $('#new_version').val();
  let prodId = $('#new_prodid').val();
  let result = {};

  if (name.length === 0 || name.length > 100 || name.includes('.')) {
    pushError('Could not create calendar!', 'Invalid name');
  } else if (version.length === 0 || isNaN(version) ) {
    pushError('Could not create calendar!', 'Invaid version');
  } else if (prodId.length === 0 || prodId.length >  1000) {
    pushError('Could not create calendar!', 'Invalid prodId');
  } else {
    /* So all of these boys are good */
    let result = {
      'name' : name,
      'version' : parseFloat(version),
      'prodId' : prodId,
      'numEvents': 0,
      'events' : []
    };
    /* Now we need grab all the events */
    let numEvents = $('#events').children('.event').length;
    result['numEvents'] = parseInt(numEvents);

    for (let i = 1; i <= numEvents; i++) {
      let uid = $('#event_' + i + '_uid').val();

      if (uid.length === 0 || uid.length > 1000) {
        pushError('Could not create calendar!', 'Invalid UID, Event #' + i);
        return;
      }

      /* Need to check dates */
      let today = new Date();
      let dtstamp = new String(today.getFullYear()) + new String(today.getMonth() + 1).padStart(2, '0') + new String(today.getDate()).padStart(2, '0') + 'T' + new String(today.getHours()).padStart(2, '0') + new String(today.getMinutes()).padStart(2, '0') + new String(today.getSeconds()).padStart(2, '0') + 'Z';
      let startdate = $('#event_' + i + '_dtstart_date').val();
      let starttime = $('#event_' + i + '_dtstart_time').val();
      let dtstartraw = new Date(startdate + ' ' + starttime);
      let dtstart = new String(dtstartraw.getFullYear()) + new String(dtstartraw.getMonth() + 1).padStart(2, '0') + new String(dtstartraw.getDate()).padStart(2, '0') + 'T' + new String(dtstartraw.getHours()).padStart(2, '0') + new String(dtstartraw.getMinutes()).padStart(2, '0') + new String(dtstartraw.getSeconds()).padStart(2, '0') + 'Z';

      let event = {
        'uid' : uid,
        'dtstart' : dtstart,
        'dtstamp' : dtstamp,
        'numProps': 0,
        'props': []
      };

      var numProps = $('#event_' + i + '_props').children().length;
      event['numProps'] = numProps;

      for (let j = 1; j <= numProps; j++) {
        let name = $('#event_' + i + '_' + j + '_propname').val();
        let value = $('#event_' + i + '_' + j + '_propdescr').val();

        if (name.length === 0 || value.length === 0 || name.length > 200) {
          pushError('Could not create calendar!', 'Property ' + j + ' in Event #' + i);
          return;
        }
        event['props'].push({'propName' : name, 'propDescr' : value});
      }

      result['events'].push(event);
    }

    console.log(result);
    $.post('/create', result).done(function(result) {
      fileLog();
    });
  }
});

$('#btnFile').on('change', function() {
  if ($('#btnFile').val() !== '') {
    var file = new FormData();
    var filename = $('#btnFile').val().replace(/.*[\/\\]/, '');

    file.append("uploadFile", $('#btnFile').prop('files')[0] );
    $.ajax({
      url: '/uploads',
      type: 'POST',
      dataType: 'text',
      data: file,
      cache: false,
      contentType: false,
      processData: false,
      success: function() {
        fileLog();
      },
      error: function(errCode) {
        pushError('Could not upload "' + filename + '"!', errCode['responseText']);
      }
    });
  }
  /* $('#btnFile').val(''); */
});

function addprop ( to, eventnum ) {
  let propNum = $('#' + to).children().length + 1;
  let str = '<div class="row"><div class="col s12"><div class="input-field col s6"><input type="text" id="event_' + eventnum + '_' + propNum + '_propname" class="validate"><label for="event_' + eventnum + '_' + propNum + '_propname">Property Name</label></div>';
  str += '<div class="input-field col s6"><input type="text" id="event_' + eventnum + '_' + propNum + '_propdescr" class="validate"><label for="event_' + eventnum + '_' + propNum + '_propdescr">Property Value</label></div>';
  str += '</div></div>';
  $('#' + to).append(str);
}

/* Updates the file log */
function fileLog() {
  /* Clear */
  $('#statusMessage').empty();
  $('#filelogbody').empty();
  $('#caldropdown').empty();
  $('#caldropdown').append('<option value="" disabled selected>Choose a file</option>');
  $.ajax({
    type: "GET",
    url: "/getnumfiles",
    dataType: "JSON",
    success: function(res) {
      //Show how many files there are on the server
      if (res['numFiles'] == 0 ) {
        $('#statusMessage').append('There are <span class="pink-text">NO</span> valid files on the server.');
      } else {
        $('#statusMessage').append('There are <span class="pink-text">' + res['numFiles'] + '</span> valid files on the server.');
      }
      fileCount = res['numFiles'];
      //Append them
      res['files'].forEach(obj => {
        /* Show in file log panel only if it's valid */
        calendars[obj['filename']] = obj['eventList'];
        var row = '<tr><td><a href="uploads/' + obj['filename'] +'">' + obj['filename'] + '</a></td><td>' + obj['version'] + '</td><td>' + obj['prodID'] + '</td><td>' + obj['numEvents'] + '</td><td>' + obj['numProps'] + '</td></tr>';
        $('#filelogbody').append(row);
        $('#caldropdown').append('<option value="' + obj['filename'] + '">' + obj['filename'] + '</option>');
      });
    }
  });
}