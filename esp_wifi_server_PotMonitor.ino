
#include <Adafruit_Sensor.h>


#include <Adafruit_TSL2561_U.h>
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

//Displays some basic information on this sensor from the unified
//sensor API sensor_t type (see Adafruit_Sensor for more information)
void displaySensorDetails(void)
{
  sensor_t sensor;
  tsl.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" lux");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" lux");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" lux");
  Serial.println("------------------------------------");
  Serial.println("");
  delay(500);
}

//Configures the gain and integration time for the TSL2561
void configureSensor(void)
{
  /* You can also manually set the gain or enable auto-gain support */
  // tsl.setGain(TSL2561_GAIN_1X);      /* No gain ... use in bright light to avoid sensor saturation */
  // tsl.setGain(TSL2561_GAIN_16X);     /* 16x gain ... use in low light to boost sensitivity */
  tsl.enableAutoRange(true);            /* Auto-gain ... switches automatically between 1x and 16x */

  /* Changing the integration time gives you better sensor resolution (402ms = 16-bit data) */
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);      /* fast but low resolution */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);  /* medium resolution and speed   */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);  /* 16-bit data but slowest conversions */

  /* Update these values depending on what you've set above! */
  Serial.println("------------------------------------");
  Serial.print  ("Gain:         "); Serial.println("Auto");
  Serial.print  ("Timing:       "); Serial.println("13 ms");
  Serial.println("------------------------------------");
}

#include <Adafruit_BMP280.h>
Adafruit_BMP280 bmp; // I2C

#include "gravity_soil_moisture_sensor.h"
GravitySoilMoistureSensor gravity_sensor;
const int soilPin = 32;


#include <WiFi.h>
const char* ssid     = "SHAW-6582D3";
const char* password = "277383744";
WiFiServer server(80);


String header;



void setup()
{
  Serial.begin(115200);
  pinMode(2, OUTPUT);      // set the LED pin mode

  delay(10);

  // initialzing soil moisture sensor
  if (!gravity_sensor.Setup(soilPin)) {
    Serial.println("Gravity Soil Moisture Sensor was not detected. Check wiring!");
    while (1);
  } else {
    Serial.println("Gravity Soil Moisture Sensor init done.");
  }
  
  // initialzing light sensor
  if (!tsl.begin())
  {
    /* There was a problem detecting the TSL2561 ... check your connections */
    Serial.print("Ooops, no TSL2561 detected ... Check your wiring or I2C ADDR!");
    while (1);
  }
  /* Display some basic information on this sensor */
  displaySensorDetails();
  /* Setup the sensor gain and integration time */
  configureSensor();

  // initialzing air temperature, pressure,humidity sensor
  if (!bmp.begin()) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    while (1);
  }
  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */


  //connecting to WiFi

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.begin();

  digitalWrite(2, HIGH);  

  randomSeed(analogRead(0));

}


float lightVal;
uint16_t soil_mois;
float humidity;
float celsius; //C
float pressure; // hpa




void loop() {
  WiFiClient client = server.available();   // listen for incoming clients


  if (client) {                             // if you get a client,

    // reading from sensors
    //humidity = 0; // if using BMP180, then it does not have humidity reading
    //humidity = bmp.readHumidity(); // use this, if you have BMP280
    celsius = bmp.readTemperature(); //C
    pressure = bmp.readPressure() * 0.01; // hpa

    soil_mois = gravity_sensor.Read();

    sensors_event_t event;
    tsl.getEvent(&event);
    if (event.light) {
      lightVal = event.light;
    }

    // handle client
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor

        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            
              // Display the HTML web page
              client.println("<!DOCTYPE html><html>");
              client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
              client.println("<link rel=\"stylesheet\" href=\"https://use.fontawesome.com/releases/v5.7.2/css/all.css\" integrity=\"sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr\" crossorigin=\"anonymous\">");
              // Feel free to change the background-color and font-size attributes to fit your preferences
              client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
              client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
              client.println("p { font-size: 4.0rem; } .units { font-size: 1.2rem; }");
              client.println("measure-labels{font-size: 6.5rem;vertical-align:middle;padding-bottom: 35px;}");
              
              client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
              client.println(".button2 {background-color: #555555;}</style></head>");

              // Web Page Heading
              client.println("<body><h1>ESP32 Plant Monitor</h1>");

              client.println("<p><i class=\"fas fa-thermometer-quarter\" style=\"color:#00add6;\"></i>"); 
              client.println("<span class=\"measure-labels\">Temperature :</span>");
              client.println("<span class=\"measure-labels\">" + String(celsius) + " C</span></p>");
                            
              client.println("<p><i class=\"fas fa-tint\" style=\"color:#00add6;\"></i>"); 
              client.println("<span class=\"measure-labels\">Humidity: " +String(humidity) + "</span>");
              client.println("<span class=\"units\"> %</span></p>");              

              client.println("<p><i class=\"fas fa-caret-down\" style=\"color:#00add6;\"></i>"); 
              client.println("<span class=\"measure-labels\">Pressure: " +String(pressure) + " hPa</span></p>");

              client.println("<p><i class=\"fas fa-cloud-sun\" style=\"color:#00add6;\"></i>"); 
              client.println("<span class=\"measure-labels\">Lighting: " +String(lightVal) + " Lux</span></p>");

              client.println("<p><i class=\"fas fa-water\" style=\"color:#00add6;\"></i>"); 
              client.println("<span class=\"measure-labels\">Soil Moisture: " +String(soil_mois) + " </span></p>");
              
              client.println("<p><a href=\"/update\"><button class=\"button\">Update</button></a></p>");

              client.println("</body></html>");
            
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          }
          else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        }
        else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /H")) {
          digitalWrite(2, HIGH);               // GET /H turns the LED on
        }
        if (currentLine.endsWith("GET /L")) {
          digitalWrite(2, LOW);                // GET /L turns the LED off
        }
        if (currentLine.endsWith("GET /abc")) {
          Serial.print(" -- usr cliked NNNNNN  -- ");
        }
        if (currentLine.endsWith("GET /tt1")) {
          Serial.print(" -- usr cliked tt1  -- ");
        }
        if (currentLine.endsWith("GET /tt2")) {
          Serial.print(" -- usr cliked tt2  -- ");
          digitalWrite(2, HIGH);
        }
      }
    }
    header = "";
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }

}
