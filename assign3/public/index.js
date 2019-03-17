var fileCount = 0;
var calendars = {};

$(document).ready(function() {
  fileLog();
  $('select').formSelect();
  /* This is where we will re-render the calendar */
  $('#caldropdown').on('change', function() {
    $('#calview').empty();
    let caltable = '<table><thead><th>Event No</th><th>Start Date</th><th>Start Time</th><th>Summary</th><th>Props</th><th>Alarms</th></thead>';
    caltable += '<tbody>';
    for (var i = 0; i < calendars[$(this).val()].length; i++) {
      var row = calendars[$(this).val()][i];
      caltable += '<tr><td>' + (1 + i) + '</td><td>' + formatDate(row['startDT']['date']) + '</td><td>' + formatTime(row['startDT']['time']) + (row['startDT']['isUTC'] ? ' (UTC)' : '') + '</td><td>' + row['summary'] + '</td><td>' + row['numProps'] + '</td><td><a style="cursor: pointer" onclick="$(\'.alarms_' + (i+1) + '\').toggle()">' + row['numAlarms'] + '</a></td></tr>';
      caltable += '<tr style="display:none" class="grey lighten-1 alarms_' + (i + 1) + '"><th>Action</th><th>Trigger</th><th>Props</th></tr>';
      for (var j = 0; j < row['alarms'].length; j++) {
        var alarm = row['alarms'][j];
        caltable += '<tr style="display:none" class="grey lighten-2 alarms_' + (i + 1) + '"><td>' + alarm['action'] + '</td><td>' + alarm['trigger'] + '</td>' + '<td>' + alarm['numProps'] + '</td></tr>';
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

function pushError(errorMsg) {
  /* A visual to see if there are any erorrs */
  if ( $('#status').hasClass('green') ) {
    $('#status').removeClass('green');
    $('#status').addClass('red');
  }
  $('#errorList').append('<li class="collection-item">' + errorMsg + '</li>');
}

$('#btnClear').click(function() {
  if ( $('#status').hasClass('red') ) {
    $('#status').removeClass('red');
    $('#status').addClass('green');
  }
    $('#errorList').empty();
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
      error: function() {
        pushError('Could not upload "' + filename + '"!');      }
    });
  }
  /* $('#btnFile').val(''); */
});

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
