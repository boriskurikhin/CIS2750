var fileCount = 0;

function init() {
  /* This function will get called on init */
  var elem = document.querySelector('.tabs');
  var instance = M.Tabs.init(elem, {});
}

$(document).ready(function() {
  fileLog(true);
});

function addCalendar(filename) {
  $.ajax({
    type: "GET",
    url: "/getfile/" + filename,
    dataType: "JSON",
    success: function(res) {
      //Show how many files there are on the server

      //Append them
      let obj = res['file'];
      console.log(obj);

      if (obj !== null && obj['filename'] === filename) {
      // <li class="tab"><a href="#test4">Test 1</a></li>
        $('#tabList').append('<li class="tab"><a href="#' + obj['filename'] + '">' + obj['filename'] + '</a></li>');
        // <div id="test4">Test 1</div>
        var table = '<div id="' + obj['filename'] + '"><table><thead><tr><th>File Name</th><th>Version</th><th>Product ID</th><th># Events</th><th># Properties</th></tr></thead><tbody><tr><td><a href="upload/' + obj['filename'] +'">' + obj['filename'] + '</a></td><td>' + obj['version'] + '</td><td>' + obj['prodID'] + '</td><td>' + obj['numEvents'] + '</td><td>' + obj['numProps'] + '</td></tr></tbody></table></div>';
        $('#tabVals').append(table);
        fileCount += 1;

        $('#statusMessage').empty();

        if (res['numFiles'] == 0 ) {
          $('#statusMessage').append('There are <span class="pink-text">NO</span> valid files on the server.');
        } else {
          $('#statusMessage').append('There are <span class="pink-text">' + fileCount + '</span> valid files on the server.');
        }
        init();
      } else {
        pushError('Could not upload "' + filename + '"!');
      }
    }
  });
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
        addCalendar(filename);
      },
      error: function() {
        pushError('Could not upload "' + filename + '"!');      }
    });
  }
  /* $('#btnFile').val(''); */
})

function fileLog(doInit) {
  /* Clear */
  $('#statusMessage').empty();
  $('#tabList').empty();
  $('#tabVals').empty();
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
        // <li class="tab"><a href="#test4">Test 1</a></li>
        $('#tabList').append('<li class="tab"><a href="#' + obj['filename'] + '">' + obj['filename'] + '</a></li>');
        // <div id="test4">Test 1</div>
        var table = '<div id="' + obj['filename'] + '"><table><thead><tr><th>File Name</th><th>Version</th><th>Product ID</th><th># Events</th><th># Properties</th></tr></thead><tbody><tr><td><a href="upload/' + obj['filename'] +'">' + obj['filename'] + '</a></td><td>' + obj['version'] + '</td><td>' + obj['prodID'] + '</td><td>' + obj['numEvents'] + '</td><td>' + obj['numProps'] + '</td></tr></tbody></table></div>';
        $('#tabVals').append(table);
      });
      if (doInit === true && fileCount > 0) {
        init();
      }
    }
  });
}
