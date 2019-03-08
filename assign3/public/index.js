function init() {
  /* This function will get called on init */

}

$.ajax({
  type: "GET",
  url: "/getnumfiles",
  dataType: "JSON",
  success: function(res) {
    $('#numFiles').text(res['numFiles']);
  }
})
