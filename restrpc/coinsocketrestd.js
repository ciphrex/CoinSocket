///////////////////////////////////////////////////////////////////////////////
//
// coinsocketrestd.js
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.
//

var settings = require('./settings.js');

var request = require('request');

// WS Client
var rpcsocket = require('./rpcsocket');
rpcsocket.on('all', function(obj) {
    console.log('handler called.');
    console.log(obj);
    settings.eventcallbackurls.forEach(function(url) {
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
var wsendpoint = settings.wsserverurl + '/' + settings.accesskey;
console.log('Connecting to ' + wsendpoint);
rpcsocket.connect(wsendpoint);

// HTTP Server
console.log('Starting http server on port ' + settings.httpport + '...');
var http = require("http");
var server = http.createServer(function(request, response) {
    var fullcommand = request.url.split("/");
    console.log('fullcommand');
    console.log(fullcommand);
    if (fullcommand.length < 3) {
        returnError({ message: "Invalid command.", code: -1 }, response);
        return;
    }

    var accesskey = fullcommand[1];
    if (accesskey !== settings.accesskey) {
        returnError({ message: "Invalid access key.", code: -2 }, response);
        return;
    }

    var method = fullcommand[2];
    var params = fullcommand.slice(3);
    execCommand(method, params, response);
});
server.listen(settings.httpport);
console.log('http server is listening on port ' + settings.httpport + '.');

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
