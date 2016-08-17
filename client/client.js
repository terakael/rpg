(function() {
	var hsButton = document.getElementById("Handshake"),
		ws = null;
		
	hsButton.addEventListener("click", function() {
		if (window.WebSocket !== undefined) {
			ws = new WebSocket("ws://127.0.0.1:45555");
			
			ws.onopen = function() {
				//ws.send("Message to send");
				//alert("Message is sent!");
			}
			
			ws.onmessage = function(evt) {
				var received = evt.data;
				//alert("Message is received: " + received);
			}
			
			ws.onclose = function() {
				alert("Connection is closed");
				ws = null;
			}
			
			ws.onerror = function(evt) {
				alert("Error: " + evt);
				ws = null;
			}
		} else {
			alert("Not supported :S");
		}
	});
	
	var messageButton = document.getElementById("Message");
	messageButton.addEventListener("click", function() {
		if (!ws)
			return;
		
		//alert("sending message");
		ws.send(JSON.stringify({
			action: "logon",
			username: "sampleuser",
			password: "samplepassword"
		}));
	});
})();