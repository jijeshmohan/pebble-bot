

Pebble.addEventListener("ready",
    function(e) {
        console.log("Hello world! - Sent from your javascript application.");
    }
);


Pebble.addEventListener("appmessage",
  function(e) {
    console.log("Received message: " + e.payload);
    controlBot(e.payload.angle,e.payload.speed);
  }
);


function controlBot(angle,speed) {
  
  var req = new XMLHttpRequest();
  // build the POST request
  req.open('POST', "http://192.168.1.5:3000/speed/" + speed + "/angle/" + angle, true);
  req.send(null);
}