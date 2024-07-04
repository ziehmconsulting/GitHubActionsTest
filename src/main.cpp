/*H**********************************************************************
* FILENAME :        clean_Send.ino             DESIGN REF: FMCM00
* Version:          0.0.1
* DESCRIPTION :
*       Connection test for the Norvi AGENT1 CST-BT9-E-L based ESP32 with current messurment.
*
* PUBLIC FUNCTIONS :
*       void
*       int
*
* NOTES :
*       The naming of the prefixes of the variables and functions results from the variable type or
*       the variable type of the return value.
*       "c" stands for "char", an "s" for "short",
*       "v" for "void", a "u" for "unsigned" and
*       "x" for all non-standard types
*
*       Copyright Tjark Ziehm 2024
*
* AUTHOR :    Tjark Ziehm  & Bhanuka Gamachchige
* START DATE :    01 Juni 2024
*
* CHANGES :
*
* Convention:   <major>.<minor>.<patch>
* REF NO  VERSION DATE        WHO     DETAIL
* F21/33  A.03.04 20April2024 TZ      add Header
*
*H*/
/********************************************************************************************************************NETWORK PAGE VARIABLES****************************************/
#include "credentials.hpp"
#include "global_variables.hpp"
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include "features/ACS712_Features.hpp"



/***********************************************SCHEDULER PAGE VARIABLES**********************************************************/
// Define starting addresses for each text zone
#define ADDR_UI_NETWORK 0
#define ADDR_UI_SIGNAL 7
#define ADDR_UI_STATUS_MQTT 13
#define ADDR_NETWORK_APN 23
#define ADDR_MQTT_SERVER 33
#define ADDR_MQTT_USER 43
#define ADDR_MQTT_PW 50
#define ADDR_MQTT_DN 60
#define ADDR_MQTT_ID 72
#define ADDR_MQTT_SD 86
/**********************************************************************************************************************************/
// Modem Serial Pins
#define RX_PIN 4       // ESP32 RX pin
#define TX_PIN 2       // ESP32 TX pin
#define BAUD_RATE 9600 // Baud rate for ESP32 UART communication
#define GSM_RESET 19
#define GSM_ENABLE 18
/**********************************************************************************************************************************/
// Define MQTT topics
#define MQTT_TOPIC_1 "v1/devices/me/rpc/request/1"
#define MQTT_TOPIC_2 "v1/devices/me/rpc/request/2"
#define MQTT_TOPIC_5 "v1/devices/me/rpc/request/3"
#define MQTT_TOPIC_3 "v1/devices/me/attributes"
#define MQTT_TOPIC_4 "v1/devices/me/telemetry"
/**********************************************************************************************************************************/
// Define your firmware version and serial number here
String firmwareVersion = "1.23";
String serialNumber = "SN-12345";
String mqttMessage = "";
String payload = "";
bool Ignore = false;
/**********************************************************************************************************************************/
// Define your sensor variables here
int energy = 65451;
float current = 12.5;

/**********************************************************************************************************************************/
// Define a constant for the interval (in milliseconds)
#define MQTT_CHECK_INTERVAL 10000  // 10 seconds
#define SENSOR_SEND_INTERVAL 20000 // 20 seconds
/**********************************************************************************************************************************/
// Get the current time
unsigned long currentMillis = 0;
// Define a variable to store the last time the MQTT connection was checked
unsigned long lastMQTTCheckTime = 0;
unsigned long previousMillis = 0;
// Define a variable to store the last time Sensor data was sent
unsigned long lastSensorValSentTime = 0;
/**************************************************HANDLE SAVE FROM WEB FUNCTION**************************************************/
// monitor serial
void monitorSerial1()
{
  while (Serial1.available())
  {
    if (Ignore)
    {
      // reset to ignore the sent loop
      payload = String("No payload!");
      mqttMessage = String("No message!");
    }
    else
    {
      mqttMessage = Serial1.readStringUntil('\n');
    };
    Serial.println(mqttMessage);
  }
}

/***************************************************HANDLE SAVE FROM WEB FUNCTION************************************************/
// Reboot the Modem
void rebootModem()
{
  // Send AT command
  for (int i = 0; i < 3; i++)
  {
    Serial1.println("AT");
    unsigned long previousMillis = millis();
    while (millis() - previousMillis < 1300)
    {
      monitorSerial1();
      String command = Serial1.readStringUntil('\n');
      if (command.startsWith("OK"))
        break;
    }
  }

  // Send reboot command
  Serial1.println("AT+NRB");
  unsigned long previousMillis = millis();
  while (millis() - previousMillis < 10000)
  {
    monitorSerial1();
  }
}

/*******************************************************HANDLE SAVE FROM WEB FUNCTION*********************************************/
// function to check if the devce is still connected
bool isConnected()
{
  // Send the AT command to check MQTT connection status
  String command = "AT+QMTCONN?";
  Serial1.println(command);
  Serial.println(command);

  // Wait for response
  previousMillis = millis();
  while (millis() - previousMillis < 8000)
  {
    monitorSerial1();
    if (Serial1.find("+QMTCONN: 0,3"))
    {
      Serial.println("--------------------------------------------------------------------------------------------------------------");
      Serial.println("MQTT Status: Connected!");
      Serial.println("--------------------------------------------------------------------------------------------------------------");
      return true;
    }
    else if (Serial1.find("+QMTCONN: 0,2"))
    {
      Serial.println("--------------------------------------------------------------------------------------------------------------");
      Serial.println("MQTT Status: Being Connected");
      Serial.println("--------------------------------------------------------------------------------------------------------------");
      return true;
    }
    else if (Serial1.find("+QMTCONN: 0,1"))
    {
      Serial.println("-----------------------------------------------------------------------------------------------------------");
      Serial.println("MQTT Status: Initializing");
      Serial.println("-----------------------------------------------------------------------------------------------------------");
      return true;
    }
    else if (Serial1.find("+QMTCONN: 0,4"))
    {
      Serial.println("-----------------------------------------------------------------------------------------------------------");
      Serial.println("MQTT Status: Disconnected");
      Serial.println("-----------------------------------------------------------------------------------------------------------");
      return false;
    }
  }
  // If no response received within timeout, consider it disconnected
  Serial.println("--------------------------------------------------------------------------------------------------------------");
  Serial.println("MQTT Status: Disconnected (Timeout)");
  Serial.println("--------------------------------------------------------------------------------------------------------------");
  return false;
}

/*************************************************HANDLE SAVE FROM WEB FUNCTION***************************************************/
// Function to register on the network with APN
void registerOnNetwork(const String &apn)
{
  int failedAttempts = 0;
  // Send CFUN command
  Serial1.println("AT+CFUN=0");
  Serial.println("AT+CFUN=0");

  while (failedAttempts < 5)
  {
    // Send CFUN command
    Serial1.println("AT+CFUN=1");
    Serial.println("AT+CFUN=1");
    previousMillis = millis();
    while (millis() - previousMillis < 5000)
    {
      monitorSerial1();
      if (Serial1.find("OK"))
        break;
    }

    // Send CEREG command
    Serial1.println("AT+CEREG=1");
    Serial.println("AT+CEREG=1");
    previousMillis = millis();
    while (millis() - previousMillis < 5000)
    {
      monitorSerial1();
      if (Serial1.find("OK"))
        break;
    }

    // Send CGDCONT command

    String cgdcontCmd = "AT+CGDCONT=1,\"IP\",\"" + apn + "\"";
    Serial1.println(cgdcontCmd);
    Serial.println(cgdcontCmd);

    previousMillis = millis();
    while (millis() - previousMillis < 5000)
    {
      monitorSerial1();
      if (Serial1.find("OK"))
        break;
    }

    // Send CGATT command
    Serial1.println("AT+CGATT=1");
    Serial.println("AT+CGATT=1");
    previousMillis = millis();
    while (millis() - previousMillis < 5000)
    {
      monitorSerial1();
      if (Serial1.find("OK"))
        break;
    }

    // Wait for registration with the network
    previousMillis = millis();
    bool registrationSuccess = false;
    while (millis() - previousMillis < 20000)
    {
      monitorSerial1();
      if (Serial1.find("+CEREG:2"))
      {
        registrationSuccess = true;
        break;
      }
    }

    // Check if registration was successful
    if (registrationSuccess)
    {
      // Wait for final registration status
      previousMillis = millis();
      while (millis() - previousMillis < 20000)
      {
        monitorSerial1();
        //+CEREG:1,5
        if (Serial1.find("+CEREG:5"))
        {
          Serial.println("Modem configured and registered on the network.");
          return; // Successful registration, exit function
        }
      }
    }

    // If registration failed, increment the failedAttempts counter
    failedAttempts++;

    // If we've reached 5 failed attempts, reboot the modem and reset the counter
    if (failedAttempts == 5)
    {
      Serial.println("Failed to register on the network after 5 attempts. Rebooting modem...");
      rebootModem();
      failedAttempts = 0; // Reset the counter
    }

    Serial.println("Failed to register on the network. Retrying...");
  }
}

/**************************************************HANDLE SAVE FROM WEB FUNCTION**************************************************/
// Function to connect to MQTT Broker
void connectToMQTTBroker(const String &server, const String &user, int port, const String &password, const String &clientId)
{
// Send QMTOPEN command
OpenMQ:;
  String qmtopenCmd = "AT+QMTOPEN=0,\"" + server + "\"," + String(port);
  Serial1.println(qmtopenCmd);
  Serial.println(qmtopenCmd);
  previousMillis = millis();
  while (millis() - previousMillis < 10000)
  {
    monitorSerial1();
    if (Serial1.find("OK"))
      break;
    if (Serial1.find("ERROR"))
    {
      Serial.println("Retry MQTT OPEN");
      delay(3000);
      goto OpenMQ;
    };
  }
  previousMillis = millis();
  while (millis() - previousMillis < 15000)
  {
    monitorSerial1();
    if (Serial1.find("+QMTOPEN"))
      break;
  }
// Send QMTCONN command
ConnMQ:;
  String qmtconnCmd = "AT+QMTCONN=0,\"" + clientId + "\",\"" + user + "\",\"" + password + "\"";
  Serial1.println(qmtconnCmd);
  Serial.println(qmtconnCmd);
  previousMillis = millis();
  while (millis() - previousMillis < 10000)
  {
    monitorSerial1();
    if (Serial1.find("OK"))
      break;
    if (Serial1.find("ERROR"))
    {
      Serial.println("Retry MQTT CONN");
      delay(3000);
      goto ConnMQ;
    };
  }
  previousMillis = millis();
  while (millis() - previousMillis < 15000)
  {
    monitorSerial1();
    if (Serial1.find("+QMTCONN"))
      break;
  }
  Serial.println("Successfully connected to MQTT broker.");
}

/*************************************************HANDLE SAVE FROM WEB FUNCTION**************************************************/
// Function to subscribe to MQTT topics one at a time
void subscribeToMQTTTopic(const String &topic)
{
  // Send QMTSUB command
  String qmtsubCmd = "AT+QMTSUB=0,1,\"" + topic + "\",2";
  Serial1.println(qmtsubCmd);
  Serial.println(qmtsubCmd);
  previousMillis = millis();
  while (millis() - previousMillis < 10000)
  {
    monitorSerial1();
  }
  Serial.println("Subscribed to " + topic + " successfully.");
}

/************************************************HANDLE SAVE FROM WEB FUNCTION**************************************************/
// Function to publish MQTT message with ArduinoJson payload
void publishMQTTMessage(const JsonDocument &jsonDoc, String topic)
{
  // Serialize the JSON document to a string
  String payload;
  serializeJson(jsonDoc, payload);
  // Calculate payload length
  int payloadLength = payload.length();
  // Prepare the AT command to publish the message with payload length
  String command = "AT+QMTPUB=0,0,0,0,\"" + topic + "\"," + String(payloadLength);
  Serial1.println(command);
  Serial.print(command);
  // Wait for the modem to respond with '>'
  unsigned long previousMillis = millis();
  while (1)
  {
    String response = Serial1.readStringUntil('\n');
    if (response.startsWith(">"))
    {
      monitorSerial1();
      break;
    }
    if (millis() - previousMillis > 6000)
    {
      monitorSerial1();
      Serial.println("Failed to receive prompt for message input.");
      return;
    }
  }
  // Send the payload
  Serial1.println(payload);
  Serial.println(payload);
  previousMillis = millis();
  while (millis() - previousMillis < 10000)
  {
    monitorSerial1();
    if (Serial1.find("OK"))
      break;
  };
  previousMillis = millis();
  while (millis() - previousMillis < 10000)
  {
    monitorSerial1();
    if (Serial1.find("+QMTPUB: 0,0,0"))
      break;
  };
}

/*************************************************HANDLE SAVE FROM WEB FUNCTION***************************************************/
// Function to publish device information via MQTT with specified firmware version and serial number
void publishDeviceInfo(const String &firmwareVersion, const String &serialNumber)
{
  // Create a JSON document to hold device information
  const size_t capacity = JSON_OBJECT_SIZE(2);
  DynamicJsonDocument jsonDoc(capacity);
  // Add device information to the JSON document
  jsonDoc["firmwareVersion"] = firmwareVersion;
  jsonDoc["serialNumber"] = serialNumber;
  // Publish the JSON document to MQTT topic MQTT_TOPIC_4
  publishMQTTMessage(jsonDoc, MQTT_TOPIC_4);
}

/*************************************************HANDLE SAVE FROM WEB FUNCTION**************************************************/
// Function to publish sensor values via MQTT with specified energy and current values
void publishSensorValues(int energy, float current)
{
  // Create a JSON document to hold sensor values
  const size_t capacity = JSON_OBJECT_SIZE(2);
  DynamicJsonDocument jsonDoc(capacity);
  // Add sensor values to the JSON document
  jsonDoc["energy"] = energy;
  jsonDoc["current"] = current;
  // Publish the JSON document to MQTT topic MQTT_TOPIC_4
  publishMQTTMessage(jsonDoc, MQTT_TOPIC_4);
}

/**************************************************HANDLE SAVE FROM WEB FUNCTION*************************************************/
// Function to handle messages received on MQTT_TOPIC_1
void handleMQTTTopic1(const String &payload)
{
  // Parse the JSON payload
  DynamicJsonDocument doc(512); // Adjust the capacity as needed
  DeserializationError error = deserializeJson(doc, payload);
  // Check for parsing errors
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }
  // Check if the JSON object contains the "method" field
  if (doc.containsKey("method"))
  {
    // Get the value of the "method" field
    String method = doc["method"];
    // Check if the method is "reboot"
    if (method == "reboot")
    {
      // Perform reboot operation
      // Example: Code to reboot the device
      Serial.println("-----------------------------------------------------------------------------------------------------------");
      Serial.println("");
      Serial.println("");
      Serial.print("Rebooting device");
      // Publish the response payload to the MQTT server
      // Create a JSON document to hold the message
      DynamicJsonDocument jsonDoc(512); // Adjust the capacity as needed
      jsonDoc["message"] = "Rebooting device";
      // Publish the JSON document to MQTT topic MQTT_TOPIC_1
      publishMQTTMessage(jsonDoc, MQTT_TOPIC_1);
      Ignore = true;
      delay(1000);
      ESP.restart(); // Perform a device reboot
      // rebootModem();
    }
    else
    {
      // If the method is not recognized, you can log an error or perform other actions as needed
      Serial.println("Unknown method in MQTT_TOPIC_1 payload");
    }
  }
  else
  {
    Serial.println("No 'method' field found in MQTT_TOPIC_1 payload");
  }
}


/*******************************************************SETUP********************************************************************/
void setup()
{

  Serial.begin(9600);
  delay(400);
  Serial1.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);
  delay(400);
  pinMode(GSM_RESET, OUTPUT);
  digitalWrite(GSM_RESET, HIGH);
  pinMode(GSM_ENABLE, OUTPUT);
  digitalWrite(GSM_ENABLE, HIGH); /* Turn ON output*/

  Serial.println("*******************************activate curent sensor!***********************************************");
  //setCurrentSensor();

  Serial.println("*******************************sensor activation Done!***********************************************");
  delay(500);

  Serial.println("********************************Starting Setup!*****************************************************");
  // Reboot Modem
  rebootModem();
  delay(1000);

  Serial.println("-------------------------Starting the Registration to Network with APN------------------------------");
  // Register on the network with APN
  registerOnNetwork(data.NETWORK_APN);

  // Connect to MQTT Broker
  Serial.println("-------------------------Connecting to MQTT Broker-------------------------------------------------");
  connectToMQTTBroker(data.MQTT_SERVER, data.MQTT_USER, data.MQTT_PORT, data.MQTT_PW, data.MQTT_ID);

  // Subscribe to MQTT topics
  Serial.println("-------------------------Subscribing to defined Topics------------------------------------------");
  //subscribeToMQTTTopic(MQTT_TOPIC_1);
  //subscribeToMQTTTopic(MQTT_TOPIC_2);
  //subscribeToMQTTTopic(MQTT_TOPIC_3);

  // Call the function to publish device info at boot
  Serial.println("--------------------------Publishing Device Info-------------------------------------------------");
  publishDeviceInfo(firmwareVersion, serialNumber);

  // Call the function to publish sensor values every 60 seconds
  Serial.println("--------------------------Publishing Sensor Values-----------------------------------------------");
  publishSensorValues(energy, current);

  Serial.println("**************************Setup Successfully executed!********************************************");
}
/*******************************************************LOOP********************************************************************/
void loop()
{
  // Check if it's time to send sensor data again
  if (millis() - lastSensorValSentTime >= SENSOR_SEND_INTERVAL)
  {
    // Update the last sensor value sent time
    lastSensorValSentTime = millis();
    current = getCurrent();
    publishSensorValues(energy, current); // Assuming ENERGY_VALUE and CURRENT_VALUE are variables holding the sensor values
  }

  // Check if it's time to check MQTT connection status
  if (millis() - lastMQTTCheckTime >= MQTT_CHECK_INTERVAL)
  {
    lastMQTTCheckTime = millis();
    if (!isConnected())
    {
      // If not connected, register on the network and connect to MQTT broker
      registerOnNetwork(data.NETWORK_APN);
      connectToMQTTBroker(data.MQTT_SERVER, data.MQTT_USER, data.MQTT_PORT, data.MQTT_PW, data.MQTT_ID);
      subscribeToMQTTTopic(MQTT_TOPIC_1);
      subscribeToMQTTTopic(MQTT_TOPIC_2);
    }
  }
  // Monitor Serial1 transactions
  monitorSerial1();

  if (Ignore)
  {
    // reset to ignore the sent loop
    payload = "";
    mqttMessage = "";
  };
  // Check if the received message starts with "+QMTRECV"
  if (mqttMessage.startsWith("+QMTRECV"))
  {
    if (Ignore)
    {
      payload = String("No payload!");
      mqttMessage = String("No message!");
    };
    // Print the received message
    Serial.println("-----------------------------------------------------------------------------------------------------------");
    Serial.print("Received Message:");
    Serial.print(mqttMessage);
    Serial.println("");
    Serial.println("----------------------------------------------------------------------------------------------------------");

    // Extract the topic name
    int topicStart = 15; // Find the index of escaped double quotes and add 2 to skip them
    int topicEnd = mqttMessage.indexOf("{", topicStart) - 2;
    String topic = mqttMessage.substring(topicStart, topicEnd);
    Serial.println("-----------------------------------------------------------------------------------------------------------");
    Serial.print("Topic:");
    Serial.print(topic);
    Serial.println("");
    // Extract the payload
    int payloadStart = mqttMessage.indexOf("{", topicEnd);
    payload = mqttMessage.substring(payloadStart);
    Serial.print("Payload:");
    Serial.print(payload);
    Serial.println("");
    Serial.println("-----------------------------------------------------------------------------------------------------------");

    // Check if the received topic matches MQTT_TOPIC_1 or MQTT_TOPIC_2
    if (topic == MQTT_TOPIC_1 )
    {
      // Call the function to handle the message for the corresponding topic
      if (topic == MQTT_TOPIC_1)
      {
        Serial.println("MQTT_TOPIC_1");
        handleMQTTTopic1(payload);
        payload = String("No payload!");
        mqttMessage = String("No message!");
      }
    }
  };
  if (Ignore)
  {
    // reset to ignore the sent loop
    payload = "";
    mqttMessage = "";
    Ignore = false;
  };
}
