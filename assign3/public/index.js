function init() {
  /* This function will get called on init */

}

$.ajax({
  type: "GET",
  url: "/getnumfiles",
  dataType: "JSON",
  success: function(res) {
    $('#numFiles').text(res['numFiles']);

    var insert = '<ul class="list-group flex-sm-row">';
    res['files'].forEach(obj => {
      insert += '<li class="list-group-item list-group-item-dark">';
      insert += '<strong>Filename: </strong><p>' + obj['filename'] + '</p>';
      insert += '</li>';

      insert += '<li class="list-group-item list-group-item-dark">';
      insert += '<strong>Version: </strong><p>' + obj['version'] + '</p>';
      insert += '</li>';

      insert += '<li class="list-group-item list-group-item-dark">';
      insert += '<strong>Prod ID: </strong><p>' + obj['prodID'] + '</p>';
      insert += '</li>';

      insert += '<li class="list-group-item list-group-item-dark">';
      insert += '<strong>Num Props: </strong><p>' + obj['numProps'] + '</p>';
      insert += '</li>';

      insert += '<li class="list-group-item list-group-item-dark">';
      insert += '<strong>Num Events: </strong><p>' + obj['numEvents'] + '</p>';
      insert += '</li>';

    });
    insert += '</ul>';
    $('#insertCalendars').append(insert);
  }
})
