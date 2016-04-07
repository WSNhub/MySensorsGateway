/* cross browser way of creating XMLHttpRequests */
function createXMLHttpRequest() {
    if (typeof XMLHttpRequest != "undefined") {
        return new XMLHttpRequest();
    } else if (typeof ActiveXObject != "undefined") {
        return new ActiveXObject("Microsoft.XMLHTTP");
    } else {
        throw new Error("XMLHttpRequest not supported");
    }
}

/* function updates the "output" element with the result from the 
   request url then calls itself again in 1 second.
*/
var counter = 0;
function updater() {
  //document.getElementById("counter").innerHTML = ++counter;
  var request = createXMLHttpRequest();
  // Add time to GET request to prevent caching issues
  request.open("GET", "systemtimecurrenttime", true);
  request.onreadystatechange = function() {
    if (request.readyState == 4) {
      //alert("request.readyState == 4");
      //alert(document.getElementById("currentTime").innerHTML);
      //document.getElementById("currentTime").innerHTML = "[[[" + request.responseText + "]]]";
      var splitResponseText = request.responseText.split("[DELIM]");
      document.getElementById("currentTime").innerHTML = splitResponseText[0];
      document.getElementById("systemFreeHeap").innerHTML = splitResponseText[1];
    }
  }
  request.send(null);
}

// Run the updater and schedule the updater to run after intervals.
// Note: One could make this run earlier in the page load sequence with use of 'DOMContentLoaded.
window.onload = function() {
  updater();
  setInterval(updater, 5000);
}
