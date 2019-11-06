/*
    By: Joshua Kasirye
*/

/* Disclaimer
    Please the code below works for only esp8266 which I used not esp32. It was difficult to ship in the
    esp32 before the end of this project, therefore the next best option was esp8266 which
    works similarily like the esp32. Thank you*/


  //Including all the required libraries.
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h> //http library to enable http connection
#include <WiFiClient.h>
#define LED_BUILTIN 13


// WiFi network
const char* ssid     = "Josh"; //Hotspot name
const char* password = "DonJohnJosh"; //Hotspot password


//initializing and declaring variables
volatile float val; //variable to keep the analog input from the sensor
int measuringLED = D2; //GREEN LED
int powerLED = D5; // RED LED
const int analog_inputPin = A0; //Analog pin on esp8266

// Set web servers at port number to 80
ESP8266WebServer server ( 80 );//This server name is for enabling using the html client app
WiFiServer server1(80); //This server name is for enabling using the http protocol


void setup() {
  //defining the pins, D2, D5 and A0
  pinMode(measuringLED, OUTPUT); //Green LED for measuriing
  pinMode(powerLED, OUTPUT); //Red LED for power and no connection
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(analog_inputPin, INPUT); //Pin for reading from the sensors
  digitalWrite(measuringLED, LOW); //Turning the Green LED off
  digitalWrite(powerLED, HIGH); //Turning the red LED on
  Serial.begin(115200); //serial monitor with baund frequency = 115200

  // Connecting to a WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password); // Requesting for internect connectivity

  while (WiFi.status() != WL_CONNECTED) { //Checking connecctivity
    delay(500); //waits for half a second before next request
    Serial.print(".");
    pinMode(LED_BUILTIN, LOW); //Blue LED goes off if not connected
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.on ( "/", handleRoot ); //Getting user info from the html client input form
  server.on ("/save", handleSave); //Client appllication on ESP server and saving on server

  //Beginging both servers all at once for easy communication
  server.begin();
  server1.begin();
  
  pinMode(LED_BUILTIN, HIGH);//Blue LED comes on
  digitalWrite(powerLED, LOW); //Red LED goes off
}



void loop() {
  server.handleClient();// calling the main function that handles the client app.
}


char htmlResponse[3000]; //For storing the HTML response request

//Creating the HTML Vital taking page
void handleRoot() {
  snprintf ( htmlResponse, 3000,
             "<!DOCTYPE html>\
<html lang=\"en\">\
  <head>\
    <meta charset=\"utf-8\">\
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
    <style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\
    .button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;\
    text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}\
    .button2 {background-color: #555555;}\
    </style>\
  </head>\
  <body>\
          <h1>Welcome to the REHM System</h1>\
          <br></br>\
          <h2> Vitals Taking</h2>\
          <h4>Please Make sure that the Blue light on the device is on, which means that you are connected.</h4>\
          <h4>Click on take temperature when you are ready to take the measurements</h4>\
          <br>\
          <h3>Login here</h3>\
          <p>Username: <input type='text' name='user_name' id='user_name'required></p>\
          <p>Password: <input type='password' name='password' id='password' required></p>\
          <div>\
          <p><button class=\"button\" id=\"save_button\">Take Temperature</button></p>\
          </div>\
    <script src=\"https://ajax.googleapis.com/ajax/libs/jquery/1.11.3/jquery.min.js\"></script>\
    <script>\
      var UserName;\
      var Password;\
      $('#save_button').click(function(e){\
        e.preventDefault();\
        UserName = $('#user_name').val();\
        Password = $('#password').val();\
        $.get('/save?UserName=' + UserName + '&Password=' + Password , function(data){\
          console.log(data);\
        });\
      });\
    </script>\
  </body>\
</html>");
  
  server.send ( 200, "text/html", htmlResponse );// save input form data to server

}


void handleSave() {//function to retrieve the saved information from the server 
  if (server.arg("UserName") != "") {
    Serial.println("User Name: " + server.arg("UserName")); //User name
  }
  if (server.arg("Password") != "") {
    Serial.println("Password: " + server.arg("Password")); //Password
  }

  digitalWrite(measuringLED, HIGH);//Green LED comes on.
  volatile float average = 0;
  volatile int count = 0; //variable to keep truck of added values
  for (int i = 0; i <= 10 - 1; i++) {
    val = analogRead(analog_inputPin); //Reading sensor data

    float cel = ((val * (5.0 / 1024)) - 1.375) / 0.0225; //obtaining temperature in Celcius
    if (cel > 25 and cel < 55 ) { //verrifying data being read by sensor
      average = average + cel; 
      count = count + 1;
    }

    Serial.println(cel);
    delay(1000); //waiting after one second
  }
  average = average / count; // obtaining the average
  digitalWrite(measuringLED, LOW);

  Serial.println("Average: " + String(average));

  //initializing the http client as http
  HTTPClient http;
  //link to be used to send the measured average temperature to the remote data base.
  String url = "http://rehmsys1.000webhostapp.com/add_temp.php?temp=" + String(average) + "&username=" + server.arg("UserName") + "&password=" + server.arg("Password");

  String payload = ""; //variable to store the response from the http request.
  Serial.println(url);
  http.begin(url); //sending the http request.


  //GET method
  int httpCode = http.GET(); //getting the response code
  if (httpCode > 0)
  {
    Serial.printf("[HTTP] GET...code: %d\n", httpCode);
    if (httpCode == HTTP_CODE_OK)
    {
      payload = http.getString(); // obtaing the string of the response from the database
      payload.trim(); // removing unwanted spaces from the string
    }
  }
  else
  {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();//closing the client
  
  delay(500);
  int len_payload = payload.length();//length of payload
  Serial.println(payload);
  Serial.println(len_payload);
  if (payload == "YES" ) { //Transmission was successful
    for (int i = 0; i < 3; i++) {
      digitalWrite(measuringLED, HIGH); 
      delay(1000);
      digitalWrite(measuringLED, LOW);
      delay(1000);
    }

  }
  else { //Transmission not successful
    for (int i = 0; i < 3; i++) {
      digitalWrite(powerLED, HIGH);
      delay(1000);
      digitalWrite(powerLED, LOW);
      delay(1000);
    }

  }
}
