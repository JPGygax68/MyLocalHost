
if (window.MozWebSocket) {
    console.log("Mozilla Web Sockets detected");
    window.WebSocket = window.MozWebSocket;
}

/* Namespace "IMAP"
 */

var IMAP = (function() {
    
    /* Private stuff */
    
    var tagNum = 1;
    
    function makeTag() {
        return "A" + tagNum++;
    }
    
    /* Return public functions.
     */
    return {
       
        /* Constructor
         */
        Client: function(url) {
            this.websock = new WebSocket(url);
            
            var gotCaps = false; // TODO: remove
            
            try {
                this.websock.onopen = function() {
                    console.log( "IMAP client websocket connection opened" );
                    //document.getElementById("output").innerHTML = '';
                }
                
                this.websock.onmessage = function got_packet(msg) {
                    console.log("IMAP client got_packet");
                    if (msg.data != "") {
                        console.log(msg.data);
                        if (!gotCaps) {
                            this.send(makeTag() + " CAPABILITY\n");
                            gotCaps = true;
                        }
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
