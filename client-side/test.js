
if (window.MozWebSocket) {
    console.log("Mozilla Web Sockets detected");
    window.WebSocket = window.MozWebSocket;
}

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

function make_directory_command( folder_path )
{
    var cmd;
    cmd = "$directory?path=" + encodeURIComponent(folder_path);
    return cmd;
}

function read_directory( folder_path )
{
    console.log("read_directory( \""+folder_path+"\" )");
    
    var url = get_appropriate_ws_base_url();
    url += '/' + make_directory_command( folder_path );
    console.log( url );
    
    var websock = new WebSocket(url);
    
    try {
        websock.onopen = function() {
            console.log( "websocket connection opened" );
            document.getElementById("output").innerHTML = '';
        }
        
        websock.onmessage = function got_packet(msg) {
			if (msg.data != "") {
				var data = JSON.parse( msg.data );
				//console.log( "directory = " + data.isDirectory );
				var div = document.createElement("div");
				div.className = data.isDirectory ? "folder" : "file";
				div.appendChild( document.createTextNode( data.name ) );
				// TODO: handle "." and ".." correctly
				div.path = folder_path + data.name + (data.isDirectory ? '/' : '');
				if ( data.isDirectory ) 
				{
					div.addEventListener( 'click', function() {
						read_directory( this.path );
					}, false );
				}
				document.getElementById("output").appendChild( div );
			}
        }
        
        websock.onclose = function() {
            console.log( "websocket connection CLOSED" );
        }
        
    } catch(exception) {
        alert('<p>Error' + exception + '</p>');
    }
}

function init()
{
	console.log("init()");
    window.setTimeout(function() { read_directory( '/' ); }, 500 );
}
