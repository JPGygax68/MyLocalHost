
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
            document.getElementById("output").innerHTML = '<div class="directory"></div>';
        }
        
        websock.onmessage = function got_packet(msg) {
			console.log('websock.onmessage');
			if (msg.data != "") {
				console.log(msg.data);
				var data = JSON.parse(msg.data);
				if (data.error) {
					alert(unescape(data.error));
				}
				else {
					var filename = unescape(data.name);
					console.log("Entry \""+filename+"\" is a: " + (data.isDirectory ? "folder" : "file"));
					var div = document.createElement("div");
					div.className = data.isDirectory ? "folder" : "file";
					div.appendChild( document.createTextNode(filename) );
					// TODO: handle "." and ".." correctly
					div.path = folder_path + data.name + (data.isDirectory ? '/' : '');
					if ( data.isDirectory ) 
					{
						div.addEventListener( 'click', function() {
							read_directory( this.path );
						}, false );
					}
					else {
						div.addEventListener( 'click', function() {
							read_file( this.path );
						}, false );
					}
					document.getElementById("output").firstChild.appendChild( div );
				}
			}
        }
        
		websock.onerror = function(e) {
			console.log('WebSocket reports error:');
			console.log(e);
		}

        websock.onclose = function(e) {
            console.log("websocket connection CLOSED:" );
			console.log(e);
			console.log("Code:   " + e.code);
			console.log("Reason: " + e.reason);
        }
        
    } catch(exception) {
        alert('<p>Error' + exception + '</p>');
    }
}

function make_readfile_command( file_path )
{
    var cmd;
    cmd = "$readfile?path=" + encodeURIComponent(file_path);
    return cmd;
}

function read_file( file_path )
{
    console.log("read_file( \""+file_path+"\" )");
    
    var url = get_appropriate_ws_base_url();
    url += '/' + make_readfile_command( file_path );
    console.log( url );
    
    var websock = new WebSocket(url);

	var text = "";
    
    try {
        websock.onopen = function() {
            console.log( "websocket connection opened" );
            document.getElementById("output").innerHTML = '';
        }
        
        websock.onmessage = function got_packet(msg) {
			console.log('websock.onmessage');
			if (msg.data != "") {
				//console.log(msg.data);
				text += msg.data;
			}
        }
        
		websock.onerror = function(e) {
			console.log('WebSocket reports error:');
			console.log(e);
		}

        websock.onclose = function(e) {
            console.log("websocket connection CLOSED:" );
			//console.log(e);
			//console.log("Code:   " + e.code);
			//console.log("Reason: " + e.reason);
			console.log(text);
			var pre = document.createElement("pre");
			pre.className = "file_content";
			pre.appendChild( document.createTextNode(text) );
			document.getElementById("output").appendChild( pre );
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
