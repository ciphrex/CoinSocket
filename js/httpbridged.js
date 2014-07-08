var wsserverurl = 'ws://localhost:8080/C3bScn4mPs2mAecdgdF3dlrb1yLloLRL';
var httpport = 8888;

// WS Client
var rpcsocket = require('./rpcsocket');
rpcsocket.on('syncstatuschanged', function(data) {
    console.log('syncstatuschanged handler called.');
    console.log(data);
});
rpcsocket.connect(wsserverurl);

// HTTP Server
console.log('Starting http server on port ' + httpport + '...');
var http = require("http");
var server = http.createServer(function(request, response) {
    console.log(request.url);
    rpcsocket.send({method: "getvaultinfo"}, function(obj) {
        response.writeHead(200, {"Content-Type": "application/json"});
        response.write(JSON.stringify(obj));
        response.end();
    }); 
});
server.listen(httpport);
console.log('http server is listening on port ' + httpport + '.');


/*
// WebSockets Client
console.log('Connecting to ' + wsserverurl + '...');
var WebSocket = require('ws')
  , ws = new WebSocket(wsserverurl);
ws.on('open', function() {
    console.log('Connected to ' + wsserverurl + '.');
});
ws.on('message', function(message) {
    console.log('received: %s', message);
});
ws.on('close', function() {
    console.log('WebSockets connection closed.');
});
*/

