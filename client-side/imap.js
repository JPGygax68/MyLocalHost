
if (window.MozWebSocket) {
    console.log("Mozilla Web Sockets detected");
    window.WebSocket = window.MozWebSocket;
}

/* IMAP "namespace.
 */

var IMAP = (function() {
    
    //--- Private stuff -------------------------------------------------------
    
    var tagNum = 1;
    
    function makeTag() {
        return "A" + tagNum++; // TODO: make better use of initial letter
    }
    
    /* Return public functions.
     */
    return {

        //=== Client class ====================================================

        /* Constructor
         * TODO: replace with "factory" function ?
         */
        Client: function(url, username, password, options) {
            
            this.websock = new WebSocket(url);
            
            this.pending_commands = {};
            
            this.connected = false;
            
            this.queueCommand = function (cmd, resp_handler) {
                var tag = makeTag();
                this.pending_commands[tag] = {
                    onreceive: resp_handler
                }
                this.websock.send(tag + ' ' + cmd + "\n");
                console.log("queued command: " + cmd);
            }
            
            if (typeof(options) !== 'undefined') {
                if (typeof(options.onListData) !== 'undefined') this.onListData = options.onListData;
            }
            
            that = this;
            
            try {
                this.websock.onopen = function() {
                    console.log( "IMAP client websocket connection opened" );
                    //document.getElementById("output").innerHTML = '';
                }
                
                this.websock.onmessage = function got_packet(msg) {
                    console.log("IMAP client got_packet");
                    if (msg.data != "") {
                        console.log(msg.data);
                        var lines = msg.data.split("\n");
                        for (var i in lines) {
                            var line = lines[i];
                            console.log("Line of response data: " + line);
                            // Get command tag
                            var i   = line.indexOf(' ');
                            var tag = line.substr(0, i);
                            var data = line.substr(i + 1);
                            var ok = data.substr(0, 3) === 'OK ';
                            // Very first response from server ?
                            if (! that.connected) {
                                console.log("First response from server:\n" + data);
                                that.connected = true;
                                that.queueCommand("LOGIN "+username+" "+password, function(data, ok) {
                                    console.log("LOGIN response received");
                                    if (ok) {
                                        console.log("LOGIN succeeded");
                                        that.queueCommand("SELECT INBOX", function(data, ok) {
                                            if (ok) {
                                                console.log("Inbox successfully selected");
                                                that.queueCommand("LIST \"INBOX\" %", function(data, ok) {
                                                    console.log("LIST response data received");
                                                } );
                                            }
                                        } );
                                    }
                                } );
                            }
                            // No tag (unrequested data) ?
                            else if (tag == '*') {
                                console.log("Untagged data received:\n" + data);
                                var p = data.indexOf(' ');
                                var cat = data.substr(0, p);
                                p += 1;
                                var folder = {};
                                if (cat == 'LIST') {
                                    // Get attribute names
                                    if (data[p] != '(') throw "Received LIST data does not begin with name attributes (missing opening parenthesis)";
                                    var q = data.indexOf(')', p+1);
                                    if (q < 0) throw "Syntax error in received LIST data: attribute name list is missing closing parenthesis";
                                    var attrNames = data.substr(p + 1, q - (p+1)).split(' ');
                                    for (var i in attrNames) {
                                        var attrName = attrNames[i].substr(1, 1).toLowerCase() + attrNames[i].substr(2);
                                        folder[attrName] = true;
                                    }
                                    p = q + 1;
                                    // Get separator
                                    while (data[p] === ' ') p ++;
                                    if (data[p] !== '"') throw "Failed to get LIST data separator: expected double quotes, found '"+data[p]+"'";
                                    var q = data.indexOf('"', p + 1);
                                    if (q < 0) throw "Failed to get LIST data separator: can't find closing double quotes";
                                    folder.separator = data.substr(p + 1, q - (p+1));
                                    p = q + 1;
                                    // Get mailbox name
                                    while (data[p] == ' ') p ++;
                                    if (data[p] !== '"') throw "Failed get mailbox name from LIST data: expected double quote, found '"+data[p]+"'";
                                    var q = data.indexOf('"', p + 1);
                                    if (q < 0) throw "Failed to get mailbox name from LIST data: couldn't find closing double quote";
                                    folder.name = data.substr(p + 1, q - (p+1));
                                    // Call the onListData handler (if any)
                                    if (that.onListData !== 'undefined') {
                                        that.onListData(folder);
                                    }
                                }
                            }
                            // Tag listed in pending commands list ?
                            else if (tag in that.pending_commands) {
                                var cmd = that.pending_commands[tag];
                                console.log("Command \""+tag+"\" got response data:\n" + data);
                                cmd.onreceive(data, ok);
                            }
                        }

                        /*
                        console.log(msg.data);
                        if (!gotCaps) {
                            this.send(makeTag() + " CAPABILITY\n");
                            gotCaps = true;
                        }
                        */
                    }
                }
                
                this.websock.onclose = function() {
                    console.log("IMAP client websocket connection CLOSED" );
                }
                
            } catch(exception) {
                alert('<p>Error' + exception + '</p>');
            }
        }
        
    }; // return methods into namespace
    
})(); // self-invocation of the IMAP namespace encasing method
