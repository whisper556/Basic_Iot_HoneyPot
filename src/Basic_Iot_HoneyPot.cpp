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

TCPServer server(80);
TCPClient client;

// setup() runs once, when the device is first turned on
void setup() {
  Serial.begin(9600);

  WiFi.connect(); // Connect to Wi-Fi if not already connected
  waitUntil(WiFi.ready); // Wait until Wi-Fi is ready
  Log.info("Connected to Wi-Fi");

  // Start the TCP server
  if (server.begin()) {
    Log.info("TCP server started on port 80");
  } else {
    Log.error("Failed to start TCP server");
  }

  Serial.println("Setup complete");
  Particle.publish("HP status", "Setup complete", PRIVATE);

  Log.info("Device IP: %s", WiFi.localIP().toString().c_str());
  Particle.publish("HP IP", WiFi.localIP().toString(), PRIVATE);
}

void sendFakeAPIResponse(TCPClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  client.println("[{\"id\":\"plug1\",\"status\":\"off\"}]");
  client.flush();
}

void sendRedirect(TCPClient &client, const char *url) {
  // Send HTTP redirect response
  client.println("HTTP/1.1 302 Found");
  client.println("Content-Type: text/html; charset=UTF-8");
  client.print("Location: ");
  client.println(url);
  client.println("Connection: close");
  client.println(); // Empty line to end headers
  
  // Optional HTML body for browsers that don't follow redirects automatically
  client.println("<!DOCTYPE html>");
  client.println("<html>");
  client.println("<head>");
  client.print("  <meta http-equiv='refresh' content='0;url=");
  client.print(url);
  client.println("'>");
  client.println("  <title>Redirecting...</title>");
  client.println("</head>");
  client.println("<body>");
  client.print("  Redirecting to <a href='");
  client.print(url);
  client.print("'>");
  client.print(url);
  client.println("</a>");
  client.println("</body>");
  client.println("</html>");
  
  client.flush(); // Ensure data is sent
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
  client = server.available(); // Check for incoming client connections

  if(client && client.connected()) {
    // Turn on the LED to indicate a connection
    RGB.control(true);
    RGB.color(255, 165, 0); // Orange on connection

    // Get client IP address
    if (client.available()) {
        String requestLine = "";
        String fullRequest = "";
  
  // Read the entire HTTP request until we hit the empty line
        while (client.available()) {
          char c = client.read();
          fullRequest += c;
    
        // Save the first line separately
        if (requestLine.indexOf('\n') == -1) {
           requestLine += c;
        }     
    
        // Stop when we hit the empty line (end of headers)
        if (fullRequest.endsWith("\r\n\r\n") || fullRequest.endsWith("\n\n")) break;

      }
  
        // Clear any remaining data (request body)
      while (client.available()) {
          client.read();
      }

      // Log the requests
      Serial.println("First line: " + requestLine);
      Serial.println("Full request: " + fullRequest);

      // Log the request line
      Serial.println("Request: " + requestLine);

      IPAddress clientIP = client.remoteIP(); // Get the client's IP address

      // Log the connection attempt
      String message = "Client connection attempt from : " + clientIP.toString();
      Serial.println(message);
      Particle.publish("HP alert", message, PRIVATE);

      // Handle different request types
      if(requestLine.length()) {
        if (requestLine.indexOf("/login") != -1) {
          Serial.println("Attempting to redirect to VoltFlow website");
          sendRedirect(client, "https://whisper556.github.io/VoltFlow/");
        } else if (requestLine.indexOf("/api") != -1) {
          sendFakeAPIResponse(client);
        } else if (fullRequest.indexOf("curl/") != -1 || fullRequest.indexOf("POST") != -1) {
            //if client request contains "curl/" or "POST", treat it as suspicious activity
            Particle.publish("HP alert", "Suspicious activity detected", PRIVATE);
            client.println("HTTP/1.1 403 Forbidden");
            client.println("Connection: close");
            client.println();
            client.println("Forbidden");
            client.flush();
        } else {
              // Default response - redirect to VoltFlow website
             Serial.println("Default response: Redirecting to VoltFlow website");
             sendRedirect(client, "https://whisper556.github.io/VoltFlow/");
        }
      }

      // Small delay to ensure data is transmitted
      delay(100);
    }

    // Close the client connection
    client.stop();
    RGB.color(0, 250, 0); // LED green on served response
    RGB.control(false); // Turn off LED control
  }

  delay(50); // Small delay to prevent busy-waiting
}