// Require the appropriate modules
var mp = require('msgpack');
var zmq = require('zmq');
var WebSocketServer = require('ws').Server;

// Globals
var wskts = []
var counter = 0;

// Send data to clients at a set interval
// For now, this is 15fps
var fps = 15;
setInterval(  function(){
  counter++;
}, 1000/fps);

// Listen to IPC sensor messages
var zmq_img = zmq.socket('sub');
zmq_img.connect('ipc:///tmp/omap');
zmq_img.subscribe('');
console.log('IPC | Connected to omap');
// Process lidar
var last_img_cntr = counter;
zmq_img.on('message', function(msg){
  //console.log('IPC | Got omap message!')
  if( counter>last_img_cntr ) {
    for(var s=0;s<wskts.length;s++) {
      //console.log('sent!')
      wskts[s].send(msg,{binary:true},function(){
      });
    }
    last_img_cntr = counter;
  }
});

// Set up a Websocket server on 9002
var wss = new WebSocketServer({port: 9002});
// Listen to binary websockets
wss.on('connection', function(ws) {
  console.log('A client is Connnected!');
  // Client message?
  ws.on('message', function(message) {
    console.log('received: %s', message);
  });
  wskts.push(ws)
//  ws.send('something');
});


