///////////////////////////////////////////////////////////////////////////////
//
// rpcsocket.js
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.
//

var WebSocket = require('ws');

var rpcsocket = { }

rpcsocket.ws = null;
rpcsocket.url = null;
rpcsocket.sendOnOpen = null;
rpcsocket.sequence = 0;
rpcsocket.callbacks = { };
rpcsocket.handlers = { };

rpcsocket.connect = function(url) {
    var that = this;

    this.url = url;
    this.ws = new WebSocket(this.url);

    this.ws.onopen = function() {
        console.log('Connection to ' + this.url + ' open.');
        if (that.sendOnOpen) {
            var params = that.sendOnOpen.params;
            var callback = that.sendOnOpen.callback;
            that.send(params, callback);
        }
    }

    this.ws.onclose = function() {
        console.log('Connection to ' + this.url + ' closed.');
        that.ws = null;
        that.sequence = 0;
        that.callbacks = { };
    }

    this.ws.onmessage = function(e) {
        console.log('Received ' + e.data + ' from ' + that.url + '.')
        try {
            var obj = JSON.parse(e.data);
        }
        catch (err) {
            console.log('JSON parser error: ' + err);
            return;
        }

        // Callback original caller
        if (obj.id && that.callbacks.hasOwnProperty(obj.id)) {
            // Return true from callback to continue receiving calls.
            if (!(that.callbacks[obj.id](obj))) {
                delete that.callbacks[obj.id];
                console.log('Deleted callback for id ' + obj.id);
            }
        }

        // Invoke handler
        if (obj.hasOwnProperty('event') && obj.hasOwnProperty('data') && that.handlers.hasOwnProperty(obj.event)) {
            that.handlers[obj.event](obj.data);
        }
    }
}

rpcsocket.send = function(params, callback) {
    if (this.sendOnOpen)
        this.sendOnOpen = null;

    if (this.ws) {
        params.id = this.sequence.toString();
        var json = JSON.stringify(params);
        this.callbacks[params.id] = callback;
        this.sequence++;
        console.log(this.callbacks);
        console.log('Sending ' + json + ' to ' + this.url + '.');
        this.ws.send(json);
    }
    else {
        console.log('Attempting to reconnect...');
        this.sendOnOpen = { params: params, callback: callback };
        this.ws = new WebSocket(this.url);
    }
}

rpcsocket.close = function() {
    if (this.ws) this.ws.close();
};

rpcsocket.on = function(event, handler) {
    this.handlers[event] = handler;
};

rpcsocket.clearHandlers = function() {
    this.handlers = { };
};

module.exports = rpcsocket;
