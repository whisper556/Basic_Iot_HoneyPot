/* 
 * Project : Basic IoT HoneyPot
 * Description: A simple TCP server that listens for incoming connections and responds to HTTP requests.
 *              It simulates a fake API response for specific requests and redirects others to a website.
 *              It also logs connection attempts and suspicious activity.
 * Author: Arul Dsena
 * Date: 
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */

// Include Particle Device OS APIs
#include "Particle.h"

// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(AUTOMATIC);

// Run the application and system concurrently in separate threads
SYSTEM_THREAD(ENABLED);

// Show system, cloud connectivity, and application logs over USB
// View logs with CLI using 'particle serial monitor --follow'
SerialLogHandler logHandler(LOG_LEVEL_INFO);

TCPServer server(8080);
TCPClient client;

// setup() runs once, when the device is first turned on
void setup() {
  Serial.begin(9600);

  WiFi.connect(); // Connect to Wi-Fi if not already connected
  waitUntil(WiFi.ready); // Wait until Wi-Fi is ready
  Log.info("Connected to Wi-Fi");


  // Start the TCP server
  if (server.begin()) {
    Log.info("TCP server started on port 8080");
  } else {
    Log.error("Failed to start TCP server");
  }

  Serial.println("Setup complete");
  Particle.publish("HP status", "Setup complete", PRIVATE);


}

void sendFakeAPIResponse(TCPClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  client.println("[{\"id\":\"plug1\",\"status\":\"off\"}]");
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
  client = server.available(); // Check for incoming client connections

  if(client && client.connected()) {

    //turn on the LED to indicate a connection
    RGB.control(true);
    RGB.color(255, 165, 0); // Orange on connection


    //get client IP address
    if (client.available()) {
    
    String requestLine = "";
    while (client.available()) {
        char c = client.read();
        requestLine += c;
        if (c == '\n') break;
    }
    // Check for suspicious activity
   

    // Log the request line
    Serial.println("Request: " + requestLine);

    IPAddress clientIP = client.remoteIP(); // Get the client's IP address

    // Log the connection attempt
    String message = "Client connection attempt from : " + clientIP.toString();

    // Log the message to the console
    Serial.println(message);
    Particle.publish("HP alert", message, PRIVATE);

    //redirect to VoltFlow website
    
    const char redirectResponse[] =
      "HTTP/1.1 302 Found\r\n"
      "Location: https://whisper556.github.io/VoltFlow/\r\n"
      "Connection: close\r\n"
      "\r\n";

      if (requestLine.indexOf("/api/devices") != -1) {
        sendFakeAPIResponse(client);
      } else if (requestLine.indexOf("curl/") != -1 || requestLine.indexOf("POST") != -1) {

      Particle.publish("HP alert", "Suspicious activity detected", PRIVATE);
      client.println("HTTP/1.1 403 Forbidden\r\nConnection: close\r\n\r\nForbidden");

      } else {
        // Default response
        client.print(redirectResponse);
    } 

      
      delay(1000); // Simulate some processing delay
      client.stop(); // Close the client connection
      
      // after sending response
      RGB.color(0, 255, 0); // Green = served
      RGB.control(false); // return control to system

    }

    delay(200); // Small delay to prevent busy-waiting
  } 


}
