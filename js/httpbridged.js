///////////////////////////////////////////////////////////////////////////////
//
// httpbridged.js
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.
//

var wsserverurl = 'ws://localhost:8080/C3bScn4mPs2mAecdgdF3dlrb1yLloLRL';
var httpport = 8888;
var eventcallbackurls = ['http://localhost/events'];

var request = require('request');

// WS Client
var rpcsocket = require('./rpcsocket');
rpcsocket.on('all', function(obj) {
    console.log('handler called.');
    console.log(obj);
    eventcallbackurls.forEach(function(url) {
        console.log("Sending %s to %s...", JSON.stringify(obj), url);
        var options = {
            url: url,
            method: 'POST',
            json: obj
        };
        request(options, function(error, response, body) {
            console.log("Received response: %s", body);
        });
    });
});
rpcsocket.connect(wsserverurl);

// HTTP Server
console.log('Starting http server on port ' + httpport + '...');
var http = require("http");
var server = http.createServer(function(request, response) {
    var fullcommand = request.url.split("/");
    console.log('fullcommand');
    console.log(fullcommand);
    if (fullcommand.length < 2) {
        returnError({message: "Invalid command.", code: -1}, response);
        return;
    }

    var method = fullcommand[1];
    var params = fullcommand.slice(2);
    execCommand(method, params, response);
});
server.listen(httpport);
console.log('http server is listening on port ' + httpport + '.');

function returnError(error, response) {
    response.writeHead(400, {"Content-Type": "application/json"});
    response.write(JSON.stringify(error));
    response.end();
}

function execCommand(method, params, response) {
    // Make integer parameters true integers
    var i;
    for (i = 0; i < params.length; i++) {
        if (params[i] % 1 === 0) { params[i] = parseInt(params[i]); }
    }

    var command = {
        method: method,
        params: params
    };
    console.log('Executing command:');
    console.log(command);
    rpcsocket.send(command, function(obj) {
        response.writeHead(200, {"Content-Type": "application/json"});
        response.write(JSON.stringify(obj));
        response.end();
    }); 
}
