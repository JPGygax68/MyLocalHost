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

function connectImap(username, password)
{
    console.log("connect()");
    
    var url = get_appropriate_ws_base_url();
    url += '/$tcp?host=' + escape('practicomp.ch') + '&port=143';
    console.log( url );
    
    var folderListDiv = document.getElementById('folderList');
    
    var imap = new IMAP.Client(url, username, password, {
        onListData: function (folder) {
            console.log("My onListData handler was called, folder name is \""+folder.name+"\"");
            //console.log(folder);
            var folderDiv = document.createElement("div");
            folderDiv.className = "folder";
            folderDiv.innerHTML = folder.name; // TODO: proper encoding
            folderListDiv.appendChild(folderDiv);
        }
    } );
}
