/* The "appropriate" base URL is, by definition, one that replaces "http" with 
 * "ws" or "https" with "wss", and removes the last part of the path of the
 * currently loaded document.
 */
function get_appropriate_ws_base_url()
{
    var pcol;
    var u = document.URL;
    
    /* We open the websocket encrypted if this page came on an
     * https:// url itself, otherwise unencrypted
     */
    
    if (u.substring(0, 5) == "https") {
        pcol = "wss://";
        u = u.substr(8);
    } else {
        pcol = "ws://";
        if (u.substring(0, 4) == "http")
            u = u.substr(7);
    }
    
    u = u.split('/');
    var url = pcol;
    for (var i = 0; i < (u.length-1); i ++) {
        url += '/' + u[i];
    }
    
    //console.log( "ws base url is \"" + url + "\"" );
    return url;
}

function connect()
{
    console.log("connect()");
    
    var url = get_appropriate_ws_base_url();
    url += '/$tcp?host=' + escape('practicomp.ch') + '&port=143';
    console.log( url );
    
    var imap = new IMAP.Client(url);
}

function init()
{
	console.log("init()");
    window.setTimeout(function() { connect(); }, 100 );
}
