var connectbutton; // = document.getElementById('connectbutton');
var output; // = document.getElementById('output');
var autoscroll; // = document.getElementById('autoscroll');
var statusbar; // = document.getElementById('statusbar');

var commandinput; // = document.getElementById('commandinput');

var urlinput; // = document.getElementById('url');
var serverurl;

var uridiv;

var ws = null;

var requestid = 0;

var onopen = function () {
    setstatus('Connected to ' + serverurl + '.');
    appendoutput('CONNECTED TO ' + serverurl + '.');
    connectbutton.value = 'Disconnect';
};

var onclose = function () {
    ws = null;
    setstatus('Connection closed.');
    appendoutput('DISCONNECTED FROM ' + serverurl + '.');
    connectbutton.value = 'Connect';
};

var onmessage = function (e) {
    console.log(e.data);
    var obj = JSON.parse(e.data);
    var json = JSON.stringify(obj, undefined, 2);
    output.value += json + '\n\n';
    if (autoscroll.checked)
        output.scrollTop = output.scrollHeight;

    // update uri
    if ((typeof obj === 'object') && (obj !== null) && obj.hasOwnProperty('result') && obj.result.hasOwnProperty('address') && obj.result.hasOwnProperty('uri')) {
        uridiv.innerHTML = '<a href="' + obj.result.uri + '">Send to ' +  obj.result.address + ' from local wallet...</a>';
    }
};

function connect() {
    if (ws) {
        // enablesubmit(false);
        ws.close();
        return;
    }

    serverurl = urlinput.value;
    setstatus('Connecting to ' + serverurl + '...');

    var accesskey = getCookie('accesskey');
    ws = new WebSocket(serverurl + '/' + accesskey);
    ws.onopen = onopen;
    ws.onclose = onclose;
    ws.onmessage = onmessage;
    // enablesubmit(true); 
}

function sendrequest(req) {
    if (!ws) return;

    commandinput.value = req;
    appendoutput('Sending ' + req  + ' ...');
    if (autoscroll.checked)
        output.scrollTop = output.scrollHeight;

    ws.send(req);
}

function appendoutput(o) {
    output.value += o + '\n\n';
}

function setstatus(s) {
    statusbar.innerHTML = s;
}

window.onload = function () {
    connectbutton = document.getElementById('connectbutton');
    output = document.getElementById('output');
    autoscroll = document.getElementById('autoscroll');
    statusbar = document.getElementById('statusbar');
    commandinput = document.getElementById('commandinput');
    urlinput = document.getElementById('url');
    uridiv = document.getElementById('uridiv');
};

// COMMANDS
function customcommand() {
    if (!ws) return;
    var req = commandinput.value;
    sendrequest(req);
}

// Global Operations
function getstatus() {
    if (!ws) return;
    var req = '{"method": "getstatus", "id": ' + requestid + '}';
    requestid++;
    sendrequest(req);
}

// Keychain Operations
function getkeychains() {
    if (!ws) return;
    var req = '{"method": "getkeychains", "id": ' + requestid + '}';
    requestid++;
    sendrequest(req);
}

function getkeychaininfo() {
    if (!ws) return;
    var keychainname = document.getElementById('getkeychaininfo_name').value;
    var req = '{"method": "getkeychaininfo", "params":["' + keychainname + '"], "id": ' + requestid + '}';
    requestid++;
    sendrequest(req);
}

function newkeychain() {
    if (!ws) return;
    var keychainname = document.getElementById('newkeychain_name').value;
    var req = '{"method": "newkeychain", "params":["' + keychainname + '"], "id": ' + requestid + '}';
    requestid++;
    sendrequest(req);
}

function renamekeychain() {
    if (!ws) return;
    var oldname = document.getElementById('renamekeychain_oldname').value;
    var newname = document.getElementById('renamekeychain_newname').value;
    var req = '{"method": "renamekeychain", "params":["' + oldname + '", "' + newname + '"], "id": ' + requestid + '}';
    requestid++;
    sendrequest(req);
}

// Account Operations
function getaccounts() {
    if (!ws) return;
    var req = '{"method": "getaccounts", "id": ' + requestid + '}';
    requestid++;
    sendrequest(req);
}

function getaccountinfo() {
    if (!ws) return;
    var accountname = document.getElementById('getaccountinfo_name').value;
    var req = '{"method": "getaccountinfo", "params": ["' + accountname + '"], "id": ' + requestid + '}';
    requestid++;
    sendrequest(req);
}

function newaccount() {
    if (!ws) return;
    var accountname = document.getElementById('newaccount_name').value;
    var minsigs = document.getElementById('newaccount_minsigs').value;
    var keychains = document.getElementById('newaccount_keychains').value;
    var keychainlist = keychains.split(',');
    var params = '"' + accountname + '", ' + minsigs;
    for (i in keychainlist) {
        params += ', "' + keychainlist[i].trim() + '"';
    }
    var req = '{"method": "newaccount", "params": [' + params + '], "id": ' + requestid + '}';
    requestid++;
    sendrequest(req);
}

function renameaccount() {
    if (!ws) return;
    var oldname = document.getElementById('renameaccount_oldname').value;
    var newname = document.getElementById('renameaccount_newname').value;
    var req = '{"method": "renameaccount", "params":["' + oldname + '", "' + newname + '"], "id": ' + requestid + '}';
    requestid++;
    sendrequest(req);
}

function issuescript() {
    if (!ws) return;
    var account = document.getElementById('issuescript_account').value;
    var label = document.getElementById('issuescript_label').value;
    var req = '{"method": "issuescript", "params":["' + account + '", "' + label + '"], "id": ' + requestid + '}';
    requestid++;
    sendrequest(req);
}

// Transaction Operations
function newtx() {
    if (!ws) return;
    var account = document.getElementById('newtx_account').value;
    var address = document.getElementById('newtx_address').value;
    var amount = document.getElementById('newtx_amount').value;
    var fee = document.getElementById('newtx_fee').value;
    var req = '{"method": "newtx", "params": ["' + account + '", "' + address + '", ' + amount + ', ' + fee + '], "id": ' + requestid + '}';
    requestid++;
    sendrequest(req);
}

function gethistory() {
    if (!ws) return;
    var startheight = document.getElementById('gethistory_startheight').value;
    var req = '{"method": "gethistory", "params": [' + startheight + '], "id": ' + requestid + '}';
    requestid++;
    sendrequest(req);
}

function getunsigned() {
    if (!ws) return;
    var req = '{"method": "getunsigned", "id": ' + requestid + '}';
    requestid++;
    sendrequest(req);
}

function gettx_int() {
    if (!ws) return;
    var txid = document.getElementById('gettx_int_id').value;
    var req = '{"method": "gettx", "params": [' + txid + '], "id": ' + requestid + '}';
    requestid++;
    sendrequest(req);
}

function gettx_string() {
    if (!ws) return;
    var hash = document.getElementById('gettx_string_hash').value;
    var req = '{"method": "gettx", "params": ["' + hash + '"], "id": ' + requestid + '}';
    requestid++;
    sendrequest(req);
}

// Blockchain Operations
function getchaintip() {
    if (!ws) return;
    var req = '{"method": "getchaintip", "id": ' + requestid + '}';
    requestid++;
    sendrequest(req);
}

function getblockheader_int() {
    if (!ws) return;
    var height = document.getElementById('getblockheader_int_height').value;
    var req = '{"method": "getblockheader", "params":[' + height + '], "id": ' + requestid + '}';
    requestid++;
    sendrequest(req);
}

function getblockheader_string() {
    if (!ws) return;
    var hash = document.getElementById('getblockheader_string_hash').value;
    var req = '{"method": "getblockheader", "params":["' + hash + '"], "id": ' + requestid + '}';
    requestid++;
    sendrequest(req);
}
