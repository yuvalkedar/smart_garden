/*
	AUTHOR: YUVAL 'KD' KEDAR
	DATE: MAY 19

	SMART GARDEN BASED ESP8266. THE LIFE BAR WILL CHANGE ITS COLOUR BASED ON THE MOISTURE SENSOR READINGS.
	GREEN = WET
	YELLOW = IN BETWEEN
	RED = DRY

	THE DATA WILL BE DISPLAYED IN THE SERIAL MONITOR AND ON A WEB SERVER.
	GO TO THIS IP ADDRESS: http://192.168.1.193
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>

#define MOISTURE_SENSOR_PIN	(A0)
#define ELECTRIC_FAUCET_PIN (D8)
#define RED_LED_PIN 		(D0)
#define YELLOW_LED_PIN 		(D2)
#define GREEN_LED_PIN 		(D4)
#define SERIAL_BAUDRATE		(9600)
#define DATA_RATE			(1000)
#define MID_MOISTURE_VAL	(67)
#define LOW_MOISTURE_VAL	(33)
#define WIFI_SSID	("change me to your wifi ssid")
#define WIFI_PASS	("change me to your wifi pass")

uint8_t life_bar[] = {RED_LED_PIN, YELLOW_LED_PIN, GREEN_LED_PIN};
uint8_t old_led = 0;
uint8_t moisture_sensor_val = 0;
float percent = 0;

int WiFiStrength = 0;
WiFiServer server(80);

void wifi_init() {
	Serial.println("\nWifi connecting to ");
	Serial.println(WIFI_SSID);
	WiFi.begin(WIFI_SSID, WIFI_PASS);

	/*
	// Set the ip address of the webserver
	// WiFi.config(WebServerIP, Gateway, Subnet)
	// or comment out the line below and DHCP will be used to obtain an IP address
	// which will be displayed via the serial console

	WiFi.config(IPAddress(192, 168, 1, 221), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
	*/

	Serial.print("\nConnecting");

	while(WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");        
	}

	// Start the server
	server.begin();
	Serial.println("Server started");

	Serial.println("Wifi Connected Successfully!");
	Serial.print("Use this URL to see the data: ");
	Serial.print("http://");
	Serial.println(WiFi.localIP() );
	Serial.println("/");
}

void setup() {
	Serial.begin(SERIAL_BAUDRATE);
	wifi_init();

	Serial.println("\nHello World\nI'm GROOT!");

	pinMode(MOISTURE_SENSOR_PIN, INPUT);
	pinMode(ELECTRIC_FAUCET_PIN, OUTPUT);
	for(uint8_t i = 0; i < 3; i++) {
		pinMode(life_bar[i], OUTPUT);
		digitalWrite(life_bar[i], LOW);
	}
}

void loop() {
	WiFiStrength = WiFi.RSSI(); // get dBm from the ESP8266

	moisture_sensor_val = analogRead(MOISTURE_SENSOR_PIN);
	moisture_sensor_val = constrain(moisture_sensor_val, 5, 205);
	percent = map(moisture_sensor_val, 5, 205, 0, 100);

	Serial.print("\n\nAnalog Value: ");
	Serial.print(moisture_sensor_val);
	Serial.print("\nPercent: ");
	Serial.print(percent);
	Serial.print("%");

	if(percent > MID_MOISTURE_VAL) {
		digitalWrite(GREEN_LED_PIN, HIGH);
		digitalWrite(YELLOW_LED_PIN, LOW);
		digitalWrite(RED_LED_PIN, LOW);
		digitalWrite(ELECTRIC_FAUCET_PIN, LOW);
	}

	else if(percent > LOW_MOISTURE_VAL && percent <= MID_MOISTURE_VAL) {
		digitalWrite(GREEN_LED_PIN, LOW);
		digitalWrite(YELLOW_LED_PIN, HIGH);
		digitalWrite(RED_LED_PIN, LOW);
		digitalWrite(ELECTRIC_FAUCET_PIN, LOW);
	}

	else if(percent <= LOW_MOISTURE_VAL) {
		digitalWrite(GREEN_LED_PIN, LOW);
		digitalWrite(YELLOW_LED_PIN, LOW);
		digitalWrite(RED_LED_PIN, HIGH);
		digitalWrite(ELECTRIC_FAUCET_PIN, HIGH);
	}

	delay(DATA_RATE); // in milliseconds

	// check to for any web server requests. ie - browser requesting a page from the webserver
	WiFiClient client = server.available();
	if(!client) return;
	
	Serial.println("new client");	// Wait until the client sends some data

	// Read the first line of the request
	String request = client.readStringUntil('\r');
	Serial.println(request);
	client.flush();

	// Return the response
	client.println("HTTP/1.1 200 OK");
	client.println("Content-Type: text/html");
	client.println(""); //  do not forget this one
	client.println("<!DOCTYPE HTML>");

	client.println("<html>");
	client.println(" <head>");
	client.println("<meta http-equiv=\"refresh\" content=\"60\">");
	client.println(" <script type=\"text/javascript\" src=\"https://www.gstatic.com/charts/loader.js\"></script>");
	client.println("  <script type=\"text/javascript\">");
	client.println("    google.charts.load('current', {'packages':['gauge']});");
	client.println("    google.charts.setOnLoadCallback(drawChart);");
	client.println("   function drawChart() {");

	client.println("      var data = google.visualization.arrayToDataTable([ ");
	client.println("        ['Label', 'Value'], ");
	client.print("        ['Moisture',  ");
	client.print(percent);
	client.println(" ], ");
	client.println("       ]); ");
	// setup the google chart options here
	client.println("    var options = {");
	client.println("      width: 400, height: 120,");
	client.println("      redFrom: 0, redTo: 25,");
	client.println("      yellowFrom: 25, yellowTo: 75,");
	client.println("      greenFrom: 75, greenTo: 100,");
	client.println("       minorTicks: 5");
	client.println("    };");

	client.println("   var chart = new google.visualization.Gauge(document.getElementById('chart_div'));");

	client.println("  chart.draw(data, options);");

	client.println("  setInterval(function() {");
	client.print("  data.setValue(0, 1, ");
	client.print(percent);
	client.println("    );");
	client.println("    chart.draw(data, options);");
	client.println("    }, 13000);");


	client.println("  }");
	client.println(" </script>");

	client.println("  </head>");
	client.println("  <body>");

	client.print("<h1 style=\"size:12px;\">ESP8266 Soil Moisture</h1>");

	// show some data on the webpage and the guage
	client.println("<table><tr><td>");

	client.print("WiFi Signal Strength: ");
	client.println(WiFiStrength);
	client.println("dBm<br>");
	client.print("Analog Raw: ");
	client.println(moisture_sensor_val);
	client.println("<br><a href=\"/REFRESH\"\"><button>Refresh</button></a>");

	client.println("</td><td>");
	// below is the google chart html
	client.println("<div id=\"chart_div\" style=\"width: 300px; height: 120px;\"></div>");
	client.println("</td></tr></table>");

	client.println("<body>");
	client.println("</html>");
	
	delay(1);
	Serial.println("Client disonnected");
	Serial.println("");
}
