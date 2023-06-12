#include "base.h"
#include "config.h"

// Timer info
#define DEVICE_Publish_interval 10000
unsigned long temp_last_time;
unsigned long current_time;

// Device init info and status
JSONVar device_info;
void initInfo()
{
  // Init a JSON Object
  device_info["device_type"] = DEVICE_TYPE;
  device_info["device_name"] = DEVICE_NAME;
  device_info["device_position"] = DEVICE_POSITION;
  device_info["device_status"] = true;
  device_info["data"]["fountain_status"] = false;
  device_info["data"]["mode"] = "manual";
}

const int radarPin = 8;
int ismotion;

int val = 0;
#define CONTROL_PIN 10
#define BUTTON_PIN 6

// Device self function
void pin_change(void);

void setup()
{
  initInfo();
  setupBase();
  temp_last_time = millis();

  pinMode(CONTROL_PIN, OUTPUT);
  attachInterrupt(BUTTON_PIN, pin_change, RISING);

  pinMode(radarPin, INPUT);
}

void loop()
{
  // Check MQTT broker connection status
  // If it is disconnected, reconnect to the broker
  if (!client.connected())
  {
    reconnect();
  }

  // Publish them ...
  current_time = millis();
  if (DEVICE_Publish_interval < (current_time - temp_last_time))
  {
    String jsonString = JSON.stringify(device_info);
    const char *jsonCharArray = jsonString.c_str();
    client.publish(TOPIC_INFO, jsonCharArray, false);
    Serial.print("Device Mode: ");
    Serial.println(device_info["data"]["mode"]);
    // Update last time value
    temp_last_time = current_time;
  }

  if (!device_info["device_status"])
  {
    digitalWrite(CONTROL_PIN, LOW);
  }

  if (device_info["device_status"] && !strcmp(device_info["data"]["mode"], "auto"))
  {
    // Radar sensor read
    ismotion = digitalRead(radarPin);
    if (ismotion)
    {
      digitalWrite(CONTROL_PIN, HIGH);
    }
    else
    {
      digitalWrite(CONTROL_PIN, LOW);
    }
  }

  if (device_info["device_status"] && !strcmp(device_info["data"]["mode"], "manual"))
  {
    if (device_info["data"]["fountain_status"])
    {
      digitalWrite(CONTROL_PIN, HIGH);
    }
    else
    {
      digitalWrite(CONTROL_PIN, LOW);
    }
  }

  client.loop();
  deviceStatus();
}

void topicSubPub()
{
  // Once connected, publish an announcement...
  // ... and resubscribe
  client.subscribe(TOPIC_CONTROL);
}

void handleCallback(char *topic, JSONVar payloadJSON)
{
  // If LED Control command is incoming, change LED status
  if (!strcmp(topic, TOPIC_CONTROL) && !strcmp(payloadJSON["device_name"], DEVICE_NAME))
  {
    if ((bool)payloadJSON["device_status"])
    {
      device_info["device_status"] = true;
    }
    else
    {
      device_info["device_status"] = false;
    }

    if ((bool)payloadJSON["device_mode"])
    {
      device_info["data"]["mode"] = "manual";
    }
    else
    {
      device_info["data"]["mode"] = "auto";
    }

    if ((bool)payloadJSON["fountain_status"])
    {
      device_info["data"]["fountain_status"] = true;
    }
    else
    {
      device_info["data"]["fountain_status"] = false;
    }
  }
}

void pin_change(void)
{
  device_info["data"]["fountain_status"] = !device_info["data"]["fountain_status"];
}