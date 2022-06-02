#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <ESP_Mail_Client.h>
#ifdef ESP32
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#else
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>S
#endif
#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is connected to GPIO 4
#define ONE_WIRE_BUS 4
// Default Threshold Temperature Value
String inputMessage3 = "40.0";
const char *PARAM_INPUT_1 = "input1"; // for fetching threshold temperature
const char *PARAM_INPUT_2 = "input2"; // for fetching recipient email
// bool sendMailCheck = false;
bool RodCheck=true;
int EmailMsgCount = 0;
const int HEATING_ROD = 23;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);

// Variables to store temperature values
String temperatureF = "";
String temperatureC = "";

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 30000;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
// The values get updated automatically
String readDSTemperatureC()
{
  // Call sensors.requestTemperatures() to issue a global temperature and Requests to all devices on the bus
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);

  if (tempC == -127.00)
  {
    Serial.println("Failed to read from DS18B20 sensor");
    return "--";
  }
  else
  {
    Serial.print("Temperature Celsius: ");
    Serial.println(tempC);
  }
  return String(tempC);
}

String readDSTemperatureF()
{
  // Call sensors.requestTemperatures() to issue a global temperature and Requests to all devices on the bus
  sensors.requestTemperatures();
  float tempF = sensors.getTempFByIndex(0);

  if (int(tempF) == -196)
  {
    Serial.println("Failed to read from DS18B20 sensor");
    return "--";
  }
  else
  {
    Serial.print("Temperature Fahrenheit: ");
    Serial.println(tempF);
  }
  return String(tempF);
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .ds-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>Heating and Temperature tracking System</h2>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="ds-labels">Temperature Celsius</span> 
    <span id="temperaturec">%TEMPERATUREC%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="ds-labels">Temperature Fahrenheit</span>
    <span id="temperaturef">%TEMPERATUREF%</span>
    <sup class="units">&deg;F</sup>
  </p>
  <br>
  <form action="/get">
    Temperature Threshold C<input type="number" step="0.1" name="input1" value="%THRESHOLD%" required><br>
    Recipient Email address<input type="text" step="0.1" name="input2" value="%THRESHOLD%" required><br>
    <input type="submit" value="Submit">
  </form>
</body>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperaturec").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperaturec", true);
  xhttp.send();
}, 10000) ;
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperaturef").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperaturef", true);
  xhttp.send();
}, 10000) ;
</script>
</html>)rawliteral";

// Replaces placeholder with DS18B20 values
String processor(const String &var)
{
  // Serial.println(var);
  if (var == "TEMPERATUREC")
  {
    return temperatureC;
  }
  else if (var == "TEMPERATUREF")
  {
    return temperatureF;
  }
  return String();
}

// old code below
#define WIFI_SSID "TECNO"//ZONG4G-56FF
#define WIFI_PASSWORD "siddiqui"//01598416

#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465

/* The sign in credentials */
#define AUTHOR_EMAIL "amadsid60@gmail.com"
#define AUTHOR_PASSWORD "myjjhsncjibtyioy"

/* Recipient's email*/
// #define RECIPIENT_EMAIL "munimzafar100@gmail.com"
String receive_email = "munimzafar100@gmail.com";

/* The SMTP Session object used for Email sending */
SMTPSession smtp;

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);

void setup()
{
  Serial.begin(115200);
  Serial.println();
  pinMode(HEATING_ROD, OUTPUT);
  sensors.begin(); // Starting DS18B20 library
  temperatureC = readDSTemperatureC();
  temperatureF = readDSTemperatureF();
  // Serial.println(sendMailCheck);
  // Serial.println(inputMessage3);

  Serial.print("Connecting to AP");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(200);
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", index_html, processor); });
  server.on("/temperaturec", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/plain", temperatureC.c_str()); });
  server.on("/temperaturef", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/plain", temperatureF.c_str()); });
  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              
    //To get temperature from user
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage3 = request->getParam(PARAM_INPUT_1)->value();
    }
    //To get the email from user
    if (request->hasParam(PARAM_INPUT_2)){
      receive_email = request->getParam(PARAM_INPUT_2)->value();
    } 
    digitalWrite(HEATING_ROD, HIGH);//heating starts on form submission
    RodCheck=false;
    EmailMsgCount=0;
     request->send(200, "text/html", "HTTP GET request sent to your ESP.<br><a href=\"/\">Return to Home Page</a>"); });
  // Start server
  server.begin();
}

void loop()
{
  if (RodCheck == false)
  {
    digitalWrite(HEATING_ROD, HIGH);
  }

  if ((millis() - lastTime) > timerDelay)
  {
    temperatureC = readDSTemperatureC();
    temperatureF = readDSTemperatureF();
    lastTime = millis();
    Serial.println(EmailMsgCount);
    

    if ((temperatureC.toFloat() >= inputMessage3.toFloat())) // check for mail sending temperature threshold
    {
      if (EmailMsgCount == 0)
      {
        smtp.debug(1);

        /* Set the callback function to get the sending results */
        smtp.callback(smtpCallback);

        /* Declare the session config data */
        ESP_Mail_Session session;

        /* Set the session config */
        session.server.host_name = SMTP_HOST;
        session.server.port = SMTP_PORT;
        session.login.email = AUTHOR_EMAIL;
        session.login.password = AUTHOR_PASSWORD;
        session.login.user_domain = "";

        /* Declare the message class */
        SMTP_Message message;

        /* Set the message headers */
        message.sender.name = "ESP";
        message.sender.email = AUTHOR_EMAIL;
        message.subject = "Temperature Sensor Notification";
        message.addRecipient("Amad", receive_email);//Sara

        /*Send HTML message*/
        String htmlMsg = "<div style=\"color:#2f4468;\"><h1>Hello Customer!</h1><p>- Temperature has been achieved. Rod has been powered off</p></div>";
        message.html.content = htmlMsg.c_str();
        message.html.content = htmlMsg.c_str();
        message.text.charSet = "us-ascii";
        message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

        /* Connect to server with the session config */
        if (!smtp.connect(&session))
          return;

        /* Start sending Email and close the session */
        if (!MailClient.sendMail(&smtp, &message))
          Serial.println("Error sending Email, " + smtp.errorReason());
      }
      // sendMailCheck = false;
      RodCheck = true;
      EmailMsgCount++;//One email notification to indicate the result
      digitalWrite(HEATING_ROD, LOW);//Heating rod powered off
    }
  }
}

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status)
{
  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success())
  {
    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failled: %d\n", status.failedCount());
    Serial.println("----------------\n");
    struct tm dt;

    for (size_t i = 0; i < smtp.sendingResult.size(); i++)
    {
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);
      time_t ts = (time_t)result.timestamp;
      localtime_r(&ts, &dt);

      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients);
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject);
    }
    Serial.println("----------------\n");
  }
}