(function() {
	var hsButton = document.getElementById("Handshake"),
		ws = null;
		
	hsButton.addEventListener("click", function() {
		if (window.WebSocket !== undefined) {
			ws = new WebSocket("ws://127.0.0.1:45555");
			
			ws.onopen = function() {
				document.getElementById("connected").value = "yes";
			}
			
			ws.onmessage = function(evt) {
				console.log(evt.data);
			}
			
			ws.onclose = function() {
				document.getElementById("connected").value = "no";
				ws = null;
			}
			
			ws.onerror = function(evt) {
				document.getElementById("connected").value = evt;
				ws = null;
			}
		} else {
			alert("Not supported :S");
		}
	});
	
	var messageButton = document.getElementById("submit");
	messageButton.addEventListener("click", function() {
		if (!ws)
			return;
		
		var msg = {
			action: "message",
			message: document.getElementById("message").value
		};
		ws.send(JSON.stringify(msg));
		
		document.getElementById("message").value = "";
	});
})();