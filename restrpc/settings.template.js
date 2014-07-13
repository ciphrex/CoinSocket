///////////////////////////////////////////////////////////////////////////////
//
// coinsocketrestd settings
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.
//

var Settings = function Settings() {
    // Set the following values in a copy of this file called settings.js
    this.wsserverurl = 'ws://localhost:8080';
    this.accesskey = '';
    this.httpport = 80;
    this.eventcallbackurls = ['http://example.com/events'];
};

Settings.instance = null;
 
Settings.getInstance = function() {
    if(this.instance === null) {
        this.instance = new Settings();
    }
    return this.instance;
};
 
module.exports = Settings.getInstance();
