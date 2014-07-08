var WebSocket = require('ws');

var rpcsocket = { }

rpcsocket.ws = null;
rpcsocket.url = null;
rpcsocket.sendOnOpen = null;
rpcsocket.sequence = 0;
rpcsocket.callbacks = { };

// TODO: allow attaching and detaching multiple handlers
//rpcsocket.onTx = null;

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
/*
        // Transaction callbacks
        if (obj.type && obj.type === 'transaction' && obj.validated && that.onTx)
            that.onTx(obj);
*/
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
}

module.exports = rpcsocket;
