///////////////////////////////////////////////////////////////
//                                                           //
//                    CONSTANT STATE                         //

/** Value for socket.readyState */
var SOCKET_CONNECTED = 1;

var lastMessage = '';

var MessageType = {
    COMMENT: 0,

    IMAGE: 1,

    // Matches G3D::GEventType::KEY_DOWN
    KEY_DOWN: 2,

    // Matches G3D::GEventType::KEY_UP
    KEY_UP: 3,

    SEND_IMAGE: 1000
};

var host = window.location.host;
if (host === '') {
    host = 'localhost:8080';
}
console.log('host = ' + host);

///////////////////////////////////////////////////////////////
//                                                           //
//                     MUTABLE STATE                         //

/** Connection to server, created in onSetup */
var socket;

/** Virtual directional pad */
var dpad;

/** Current image being displayed */
var img;

///////////////////////////////////////////////////////////////
//                                                           //
//                      EVENT RULES                          //
var bkg;

// When setup happens...
function onSetup() {
    if (! supportsWebSocket() || ! supportsJSON()) {
        alert('This device cannot act as a controller because it does not contain a full-featured HTML5 web browser.');
        return;
    }

    createDPad();
    createSocket(onReceiveMessage);
}


// When a key is pushed
function onKeyStart(key) {
    // Send G3D lower-case keys
    sendMessage(
    { type: MessageType.KEY_DOWN,
        key: {
            keysym: {
                sym: g3dKeysym(key)
            }
        }
    });
}


function onKeyEnd(key) {
    sendMessage(
    { type: MessageType.KEY_UP,
        key: {
            keysym: {
                sym: g3dKeysym(key)
            }
        } 
    });
}


// Called 30 times or more per second
function onTick() {
    clearScreen();

    if (img) {
        var scale = min(screenWidth / img.width, screenHeight / img.height);
        drawImage(img, (screenWidth - scale * img.width) / 2, (screenHeight - scale * img.height) / 2, scale * img.width, scale * img.height);
    }

    drawCentered(dpad);

    // To see the messages being sent from the client
    // fillText(lastMessage, 100, 100, makeColor(1, 1, 1), '60px sans-serif');

    drawStatus();
}

function drawStatus() {
    var text = '';
    var textColor = '#FFF';
    if (socket) {
        if (socket.readyState === SOCKET_CONNECTED) {
            textColor = '#0F0'
            text = 'connected to ' + host;
        } else {
            text = 'connecting to ' + host + '...';
            textColor = '#FF0'
        }
    } else {
        text = 'disconnected';
        textColor = '#F00'
    }
    fillText(text, screenWidth - 40, 40, textColor, '40px sans-serif', 'right', 'top');
}

/** Called with an object that is the message.  This object has a type field.
    If the message has binary form, then the data is an array which 
    contains the message data.
*/
function onReceiveMessage(msg) {
    switch (msg.type) {
        case MessageType.IMAGE:
            var newimg = new Image();

            // When the image has completed loading, replace the old one.
            newimg.onload = function () { img = newimg; };
            newimg.onerror = function (evt) { console.log('error loading image'); };

            // Data URI.  Switch to base 64 encoding of the actual data portion
            var uri = 'data:' + msg.mimeType + ';base64,' + btoa(String.fromCharCode.apply(undefined, msg.data));
            newimg.src = uri;

            // Now that we've received this one, ask the server for a new image.  It will be transmitted
            // while this one is encoding.
            sendMessage({ type: MessageType.SEND_IMAGE });
            break;

    case MessageType.COMMENT:
        console.log('Server says: ' + msg.value);
        break;

    default:
        console.log('Received message of unknown type ' + msg.type);
        break;
    }
}

///////////////////////////////////////////////////////////////
//                                                           //
//                      HELPER RULES                         //

function createDPad() {
    // Create the directional pad.  Note that images might not have
    // loaded yet, so we can't refer to their width and height.
    var w = 400;
    var h = 400;

    // Radius of a button
    var r = 200;

    dpad = new Object();

    dpad.image = loadImage("dpad.png");

    dpad.position = new Object();
    dpad.position.x = w * 0.75;
    dpad.position.y = screenHeight * 0.75 - h / 2;

    dpad.up = false;
    dpad.dn = false;
    dpad.lt = false;
    dpad.rt = false;

    // Top row
    setTouchKeyCircle(asciiCode("W"),
                      dpad.position.x,
                      dpad.position.y - h / 2, r);

    // Bottom row
    setTouchKeyCircle(asciiCode("S"),
                      dpad.position.x,
                      dpad.position.y + h / 2, r);

    // Left column
    setTouchKeyCircle(asciiCode("A"),
                      dpad.position.x - w / 2,
                      dpad.position.y, r);

    // Right column
    setTouchKeyCircle(asciiCode("D"),
                      dpad.position.x + w / 2,
                      dpad.position.y, r);
}

/** onReceiveMessage is a function that accepts a message, which will be a JavaScript object.
    Binary messages always have a 'data' field that contains the binary data. */
function createSocket(onReceiveMessage) {
    socket = new WebSocket('ws://' + host);
    socket.binaryType = "arraybuffer";

    socket.onopen = function () {
        console.log('socket opened');
    };

    socket.onmessage = function (evt) {
        var msg;

        if (evt.data instanceof ArrayBuffer) {
            // Binary message

            // http://www.adobe.com/devnet/html5/articles/real-time-data-exchange-in-html5-with-websockets.html
            // Get a view of the data
            var dataArray = new Uint8Array(evt.data);

            // The first four bytes are the length of the JSON portion of the message
            // in network byte order
            var textLength =
              (dataArray[0] << 24) +
              (dataArray[1] << 16) +
              (dataArray[2] << 8) +
              dataArray[3];

            // Convert the text portion of the message
            var msgText = String.fromCharCode.apply(undefined, dataArray.subarray(4, 4 + textLength));

            // Parse the message
            msg = JSON.parse(msgText);

            // Convert everything else into a binary dump in the 'data' field.  Since
            // dataArray is a view, taking a subarray should not incur a copy.
            msg.data = dataArray.subarray(4 + textLength);

        } else {
            // Text message
            msg = JSON.parse(evt.data);
        }

        // Invoke our real message handler with the decoded message
        if (onReceiveMessage) { onReceiveMessage(msg); }
    };

    socket.onclose = function () {
        console.log('socket closed');
        socket = undefined;
    };

    socket.onerror = function (evt) {
        console.log('socket error: ' + evt);
    };

    // Once every two seconds, send a packet to keep the connection alive
    // for servers that don't send ping packets on their own.
    setInterval(
    function () {
        if (socket) {
            // Send a ping to prevent automatic disconnection
            sendMessage('ping');
        } else {
            // Try to re-open the connection if anything goes wrong
            createSocket(onReceiveMessage);
        }
    }, 2000);
}


function supportsWebSocket() {
    return ('WebSocket' in window);
}

function supportsJSON() {
    return ('JSON' in window);
}


function drawCentered(object) {
    drawImage(object.image,
              object.position.x - object.image.width / 2,
              object.position.y - object.image.height / 2);
}


/** Pass an arbitrary object */
function sendMessage(msg) {
    if (socket && (socket.readyState === SOCKET_CONNECTED)) {
        msg = JSON.stringify(msg);
        lastMessage = msg;
        socket.send(msg);
    } else {
        console.log('tried to send before connected');
    }
}

/** Convert an ASCII code from keyStart/keyEnd to a G3D keysym value. */
function g3dKeysym(ascii) {
    // G3D uses lower-case ASCII for key codes.
    return asciiCode(toLowerCase(asciiCharacter(ascii)));
}
