function init() {
  /* This function will get called on init */
  var elem = document.querySelector('.tabs'); 
  var instance = M.Tabs.init(elem, {});
}



$.ajax({
  type: "GET",
  url: "/getnumfiles",
  dataType: "JSON",
  success: function(res) {
    $('#numFiles').text(res['numFiles']);
    if (res['numFiles'] == 0 ) {
      $('#statusMessage').append('There are <span class="pink-text">NO</span> files on the server.');
    } else {
      $('#statusMessage').append('There are <span class="pink-text">' + res['numFiles'] + '</span> files on the server.');
    }
    res['files'].forEach(obj => {
      // <li class="tab"><a href="#test4">Test 1</a></li>
      $('#tabList').append('<li class="tab"><a href="#' + obj['filename'] + '">' + obj['filename'] + '</a></li>');
      // <div id="test4">Test 1</div>
      var table = '<div id="' + obj['filename'] + '"><table><thead><tr><th>File Name</th><th>Version</th><th>Product ID</th><th># Events</th><th># Properties</th></tr></thead><tbody><tr><td>' + obj['filename'] + '</td><td>' + obj['version'] + '</td><td>' + obj['prodID'] + '</td><td>' + obj['numEvents'] + '</td><td>' + obj['numProps'] + '</td></tr></tbody></table></div>';
      $('#tabVals').append(table);
    });
    init();
  }
});
