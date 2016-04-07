// Global reference to the status display SPAN
var statusDisplay = null;

// POST the data to the server using XMLHttpRequest
function postItems(event) {
    // Cancel the form submit
    event.preventDefault();

    // The URL to POST our data to
    var postUrl = 'testgui01storeitems';

    // Set up an asynchronous AJAX POST request
    var xhr = new XMLHttpRequest();
    
    // Prepare the data to be POSTed by URLEncoding each field's contents
    var item1 = encodeURIComponent(document.getElementById('item1').value);
    var item2 = encodeURIComponent(document.getElementById('item2').value);

    var params = 'item1=' + item1 + 
                 '&item2=' + item2;
    
    showItem1.innerHTML = 'item1: [' + item1 + ']';
    showItem2.innerHTML = 'item2: [' + item2 + ']';
    params_1.innerHTML = 'params_1: [' + params + ']';

    // Replace any instances of the URLEncoded space char with +
    params = params.replace(/%20/g, '+');
    params_2.innerHTML = 'params_2: [' + params + ']';

    var mustUsePost = true;
    if (mustUsePost) {
      xhr.open('POST', postUrl, true);
    }
    else {
      xhr.open('GET', postUrl + '?' + params, true);
      getString.innerHTML = 'getString: [' + postUrl + '?' + params + ']';
    }

    // Set correct header for form data. (Case-sensitive in the current Sming implementation)
    xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');

    // Set the header to mark this as an AJAX request. (Case-sensitive in the current Sming implementation)
    xhr.setRequestHeader('HTTP_X_REQUESTED_WITH', 'xmlhttprequest');

    // Handle request state change events
    xhr.onreadystatechange = function() { 
        // If the request completed
        if (xhr.readyState == 4) {
            statusDisplay.innerHTML = '';
            if (xhr.status == 200) {
                // If it was a success, close the popup after a short delay
                statusDisplay.innerHTML = 'Stored!';

                // To get the response text: xyz = xhr.responseText;
            } else {
                // Show what went wrong
                statusDisplay.innerHTML = 'Error saving: ' + xhr.statusText;
            }
        }
    };

    // Send the request and set status
    if (mustUsePost) {
      xhr.send(params);
    }
    else {
      xhr.send();
    }
    statusDisplay.innerHTML = 'Saving...';
    
    // Next is needed for Firefox which doesn't react on the preventDefault().
    return false;
}

// When the page HTML has loaded.
window.addEventListener('load', function(evt) {
    // Cache a reference to the status display SPAN
    statusDisplay = document.getElementById('statusDisplay');
    // Handle the postItems form submit event with our postItems function
    document.getElementById('postItems').addEventListener('submit', postItems);
});
