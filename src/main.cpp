/*
 * Homie for SonOff relay.
 */

#include <Homie.h>
#include <EEPROM.h>
#include <Bounce2.h>
#ifdef SONOFF
// sonoff
#define PIN_RELAY1 12
#define PIN_RELAY2 13
#define PIN_LED 16
#define PIN_BUTTON 0
#define RELAY_ON_STATE HIGH
#define RELAY_OFF_STATE LOW
#endif

#ifdef SONOFFS20
// sonoff
#define PIN_RELAY1 12
#define PIN_RELAY2 13
#define PIN_LED 16
#define PIN_BUTTON 0
#define RELAY_ON_STATE HIGH
#define RELAY_OFF_STATE LOW
#endif


#ifdef GENERICRELAY
// esp01 generic relay with different pin configuration
#define PIN_RELAY 2
#define PIN_BUTTON 0
//#define PIN_LED 1
#define RELAY_ON_STATE LOW
#define RELAY_OFF_STATE HIGH
#endif

unsigned long downCounterStart1;
unsigned long downCounterLimit1=0;
unsigned long downCounterStart2;
unsigned long downCounterLimit2=0;
unsigned long watchDogCounterStart=0;

unsigned long keepAliveReceived=0;
int lastButtonValue = 1;

// EEPROM structure
struct EEpromDataStruct {
  int keepAliveTimeOut; // 0 - disabled, keepalive time - seconds
  bool initialState1;  // Initial state (just after boot - homie independet)
  bool initialState2;  // Initial state (just after boot - homie independet)
  int watchDogTimeOut; // 0 - disabled, watchdog time limit - seconds
};

EEpromDataStruct EEpromData;

Bounce debouncerButton = Bounce();

HomieNode relay1Node("relay01", "relay");
HomieNode relay2Node("relay02", "relay");

HomieNode keepAliveNode("keepalive", "keepalive");
HomieNode watchDogNode("watchdog", "Watchdog mode");
#ifdef SONOFFS20
HomieNode ledNode("led","led");
#endif
HomieSetting<bool> reverseMode("reverse mode", "Relay reverse mode");

/*
 * Recevied tick message for watchdog
 */
bool watchdogTickHandler(const HomieRange& range, const String& value)
{
  if (value == "0")
  {
    watchDogCounterStart = 0;
  } else {
    watchDogCounterStart = millis();
  }
  return true;
}
/*
 * Received watchdog timeout value
 */
bool watchdogTimeOutHandler(const HomieRange& range, const String& value)
{
  int oldValue = EEpromData.watchDogTimeOut;
  if (value.toInt() > 15)
  {
    EEpromData.watchDogTimeOut = value.toInt();
  }
  if (value=="0")
  {
    EEpromData.watchDogTimeOut = 0;
  }

  if (oldValue!=EEpromData.watchDogTimeOut)
  {
    String outMsg = String(EEpromData.watchDogTimeOut);
    watchDogNode.setProperty("timeOut").send(outMsg);
    EEPROM.put(0, EEpromData);
    EEPROM.commit();
  }
}
/*
 *
 */
bool relay2StateHandler(const HomieRange& range, const String& value)
{
  if (value == "ON") {
    if (reverseMode.get())
    {
      digitalWrite(PIN_RELAY2, RELAY_OFF_STATE);
    } else {
      digitalWrite(PIN_RELAY2, RELAY_ON_STATE);
    }
    relay2Node.setProperty("relayState").send("ON");
  } else if (value == "OFF") {
    if (reverseMode.get())
    {
      digitalWrite(PIN_RELAY2, RELAY_ON_STATE);
    } else {
      digitalWrite(PIN_RELAY2, RELAY_OFF_STATE);
    }
    relay2Node.setProperty("relayState").send("OFF");
  } else {
    return false;
  }
  return true;
}


bool relay1StateHandler(const HomieRange& range, const String& value)
{
  if (value == "ON") {
    if (reverseMode.get())
    {
      digitalWrite(PIN_RELAY1, RELAY_OFF_STATE);
    } else {
      digitalWrite(PIN_RELAY1, RELAY_ON_STATE);
    }
    relay1Node.setProperty("relayState").send("ON");
  } else if (value == "OFF") {
    if (reverseMode.get())
    {
      digitalWrite(PIN_RELAY1, RELAY_ON_STATE);
    } else {
      digitalWrite(PIN_RELAY1, RELAY_OFF_STATE);
    }
    relay1Node.setProperty("relayState").send("OFF");
  } else {
    return false;
  }
  return true;
}

/*
 * Keepliave tick handler
 */
bool keepAliveTickHandler(HomieRange range, String value)
{
  keepAliveReceived=millis();
  return true;
}

/*
 * Keepalive message handler
 */
bool keepAliveTimeOutHandler(HomieRange range, String value)
{
  int oldValue = EEpromData.keepAliveTimeOut;
  if (value.toInt() > 0)
  {
    EEpromData.keepAliveTimeOut = value.toInt();
  }
  if (value=="0")
  {
    EEpromData.keepAliveTimeOut = 0;
  }
  if (oldValue!=EEpromData.keepAliveTimeOut)
  {
    String outMsg = String(EEpromData.keepAliveTimeOut);
    keepAliveNode.setProperty("timeOut").send(outMsg);
    EEPROM.put(0, EEpromData);
    EEPROM.commit();
  }
}

/*
 * Timer handler
 */
bool relay1TimerHandler(HomieRange range, String value)
{
  if (value.toInt() > 0)
  {
    if (reverseMode.get())
    {
      digitalWrite(PIN_RELAY1, RELAY_OFF_STATE);
    } else {
      digitalWrite(PIN_RELAY1, RELAY_ON_STATE);
    }
    #ifdef PIN_LED
    digitalWrite(PIN_LED, LOW);
    #endif
    downCounterStart1 = millis();
    downCounterLimit1 = value.toInt()*1000;
    relay1Node.setProperty("relayState").send("ON");
    relay1Node.setProperty("relayTimer").send(value);
    return true;
  } else {
    return false;
  }
}
bool relay2TimerHandler(HomieRange range, String value)
{
  if (value.toInt() > 0)
  {
    if (reverseMode.get())
    {
      digitalWrite(PIN_RELAY2, RELAY_OFF_STATE);
    } else {
      digitalWrite(PIN_RELAY2, RELAY_ON_STATE);
    }
    #ifdef PIN_LED
    digitalWrite(PIN_LED, LOW);
    #endif
    downCounterStart2 = millis();
    downCounterLimit2 = value.toInt()*1000;
    relay2Node.setProperty("relayState").send("ON");
    relay2Node.setProperty("relayTimer").send(value);
    return true;
  } else {
    return false;
  }
}

/*
 * Led node handler
 */
#ifdef SONOFFS20
bool ledNodeHandler(HomieRange range, String message)
{
  if (message=="on" || message=="ON" || message=="1")
  {
    digitalWrite(PIN_LED, HIGH);
  } else if (message=="off" || message=="OFF" || message=="0")
  {
    digitalWrite(PIN_LED, LOW);
  }
  return true;
}
#endif

/*
 * Initial mode handler
 */
bool relay1InitModeHandler(HomieRange range, String value)
{
  int oldValue = EEpromData.initialState1;
  if (value.toInt() == 1 or value=="ON")
  {
    relay1Node.setProperty("relay1InitMode").send("1");
    EEpromData.initialState1=1;
  } else {
    relay1Node.setProperty("relay1InitMode").send( "0");
    EEpromData.initialState1=0;
  }
  if (oldValue!=EEpromData.initialState1)
  {
    EEPROM.put(0, EEpromData);
    EEPROM.commit();
  }
  return true;
}
bool relay2InitModeHandler(HomieRange range, String value)
{
  int oldValue = EEpromData.initialState2;
  if (value.toInt() == 1 or value=="ON")
  {
    relay2Node.setProperty("relay2InitMode").send("1");
    EEpromData.initialState2=1;
  } else {
    relay2Node.setProperty("relay2InitMode").send( "0");
    EEpromData.initialState2=0;
  }
  if (oldValue!=EEpromData.initialState2)
  {
    EEPROM.put(0, EEpromData);
    EEPROM.commit();
  }
  return true;
}

/*
 * Homie setup handler
 */
void setupHandler()
{
  HomieRange emptyRange;
  if (EEpromData.initialState1)
  {
    if (reverseMode.get()){
      relay1StateHandler(emptyRange, "OFF");
    }
    else{
      relay1StateHandler(emptyRange, "ON");
    }
    relay1Node.setProperty("relayInitMode").send("1");
  } else {
    if (reverseMode.get()){
      relay1StateHandler(emptyRange, "ON");
    }
    else{
      relay1StateHandler(emptyRange, "OFF");
    }
    relay1Node.setProperty("relayInitMode").send("0");
  }
  if (EEpromData.initialState2)
  {
    if (reverseMode.get()){
      relay2StateHandler(emptyRange, "OFF");
    }
    else{
      relay2StateHandler(emptyRange, "ON");
    }
    relay2Node.setProperty("relayInitMode").send("1");
  } else {
    if (reverseMode.get()){
      relay2StateHandler(emptyRange, "ON");
    }
    else{
      relay2StateHandler(emptyRange, "OFF");
    }
    relay2Node.setProperty("relayInitMode").send("0");
  }
  String outMsg = String(EEpromData.keepAliveTimeOut);
  keepAliveNode.setProperty("timeOut").send(outMsg);
  outMsg = EEpromData.watchDogTimeOut;
  watchDogNode.setProperty("timeOut").send(outMsg);
  keepAliveReceived=millis();
  #ifdef SONOFFS20
  digitalWrite(PIN_LED, LOW);
  ledNode.setProperty("state").send("off");
  #endif
}


/*
 * Homie loop handler
 */
void loopHandler()
{
  if (downCounterLimit1>0)
  {
    if ((millis() - downCounterStart1 ) > downCounterLimit1)
    {
      // Turn off relay
      if (reverseMode.get())
      {
        digitalWrite(PIN_RELAY1, RELAY_ON_STATE);
      } else {
        digitalWrite(PIN_RELAY1, RELAY_OFF_STATE);
      }
      #ifdef PIN_LED
      digitalWrite(PIN_LED, HIGH);
      #endif
      relay1Node.setProperty("relayState").send("OFF");
      relay1Node.setProperty("relayTimer").send("0");
      downCounterLimit1=0;
    }
  }
  if (downCounterLimit2>0)
  {
    if ((millis() - downCounterStart2 ) > downCounterLimit2)
    {
      // Turn off relay
      if (reverseMode.get())
      {
        digitalWrite(PIN_RELAY2, RELAY_ON_STATE);
      } else {
        digitalWrite(PIN_RELAY2, RELAY_OFF_STATE);
      }
      #ifdef PIN_LED
      digitalWrite(PIN_LED, HIGH);
      #endif
      relay2Node.setProperty("relayState").send("OFF");
      relay2Node.setProperty("relayTimer").send("0");
      downCounterLimit2=0;
    }
  }
  int buttonValue = debouncerButton.read();

  if (buttonValue != lastButtonValue)
  {
    lastButtonValue = buttonValue;
    int relayValue = digitalRead(PIN_RELAY1);
    if (buttonValue == HIGH)
    {
      HomieRange emptyRange;
      if (
          ((!reverseMode.get()) && relayValue == RELAY_ON_STATE) ||
          (( reverseMode.get()) && relayValue == RELAY_OFF_STATE) )
      {
        relay1StateHandler(emptyRange, "OFF");
      } else {
        relay1StateHandler(emptyRange, "ON");
      }
    }
  }
  debouncerButton.update();

  // Check if keepalive is supported and expired
  if (EEpromData.keepAliveTimeOut != 0 && (millis() - keepAliveReceived) > EEpromData.keepAliveTimeOut*1000 )
  {
    ESP.restart();
  }
  if (watchDogCounterStart!=0 && EEpromData.watchDogTimeOut!=0 && (millis() - watchDogCounterStart) > EEpromData.watchDogTimeOut * 1000 )
  {
    HomieRange emptyRange;
    relay1TimerHandler(emptyRange, "10"); // Disable relay for 10 sec
    relay2TimerHandler(emptyRange, "10"); // Disable relay for 10 sec
    watchDogCounterStart = millis();
  }
}

/*
 * Homie event handler
 */
void onHomieEvent(const HomieEvent& event) {
  switch(event.type) {
    case HomieEventType::CONFIGURATION_MODE: // Default eeprom data in configuration mode
      digitalWrite(PIN_RELAY1, RELAY_OFF_STATE);
      digitalWrite(PIN_RELAY2, RELAY_OFF_STATE);
      EEpromData.keepAliveTimeOut = 0;
      EEpromData.initialState1 = false;
      EEpromData.initialState2 = false;
      EEpromData.watchDogTimeOut = 0;
      EEPROM.put(0, EEpromData);
      EEPROM.commit();
      break;
    case HomieEventType::NORMAL_MODE:
      // Do whatever you want when normal mode is started
      break;
    case HomieEventType::OTA_STARTED:
      // Do whatever you want when OTA mode is started
      digitalWrite(PIN_RELAY1, RELAY_OFF_STATE);
      digitalWrite(PIN_RELAY2, RELAY_OFF_STATE);
      break;
    case HomieEventType::ABOUT_TO_RESET:
      // Do whatever you want when the device is about to reset
      break;
    case HomieEventType::WIFI_CONNECTED:
      // Do whatever you want when Wi-Fi is connected in normal mode
      break;
    case HomieEventType::WIFI_DISCONNECTED:
      // Do whatever you want when Wi-Fi is disconnected in normal mode
      break;
    case HomieEventType::MQTT_DISCONNECTED:
      // Do whatever you want when MQTT is disconnected in normal mode
      break;
  }
}

/*
 * Main setup
 */
void setup()
{
  #ifdef SONOFF
  Serial.begin(115200);
  Serial.println("\n\n");
  #endif
  EEPROM.begin(sizeof(EEpromData));
  EEPROM.get(0,EEpromData);

  pinMode(PIN_RELAY1, OUTPUT);
  pinMode(PIN_RELAY2, OUTPUT);
  #ifdef PIN_LED
  pinMode(PIN_LED, OUTPUT);
  #endif
  pinMode(PIN_BUTTON, INPUT);
	digitalWrite(PIN_BUTTON, HIGH);
	debouncerButton.attach(PIN_BUTTON);
	debouncerButton.interval(50);

  if (EEpromData.initialState1)
  {
    #ifdef SONOFF
    Serial.println( "Sonoff ON - initial state\n");
    #endif
    digitalWrite(PIN_RELAY1,RELAY_ON_STATE);
  } else {
    #ifdef SONOFF
    Serial.println( "Sonoff OFF - initial state\n");
    #endif
    digitalWrite(PIN_RELAY1,RELAY_OFF_STATE);
    EEpromData.initialState1=0;
  }
  if (EEpromData.initialState2)
  {
    #ifdef SONOFF
    Serial.println( "Sonoff ON - initial state\n");
    #endif
    digitalWrite(PIN_RELAY2,RELAY_ON_STATE);
  } else {
    #ifdef SONOFF
    Serial.println( "Sonoff OFF - initial state\n");
    #endif
    digitalWrite(PIN_RELAY2,RELAY_OFF_STATE);
    EEpromData.initialState2=0;
  }

  Homie_setFirmware(FIRMWARE_NAME, FIRMWARE_VER);
  Homie.setSetupFunction(setupHandler);
  Homie.setLoopFunction(loopHandler);
  #ifdef PIN_LED
  Homie.setLedPin(PIN_LED, LOW);
  #else
  Homie.disableLedFeedback();
  #endif
  #ifdef GENERICRELAY
  Homie.disableLogging();
  #endif
  Homie.setResetTrigger(PIN_BUTTON, LOW, 10000);
  relay1Node.advertise("relayState").settable(relay1StateHandler);
  relay2Node.advertise("relayState").settable(relay2StateHandler);
  relay1Node.advertise("relayTimer").settable(relay1TimerHandler);
  relay2Node.advertise("relayTimer").settable(relay2TimerHandler);
  relay1Node.advertise("relayInitMode").settable(relay1InitModeHandler);
  relay2Node.advertise("relayInitMode").settable(relay2InitModeHandler);
  watchDogNode.advertise("tick").settable(watchdogTickHandler);
  watchDogNode.advertise("timeOut").settable(watchdogTimeOutHandler);
  keepAliveNode.advertise("tick").settable(keepAliveTickHandler);
  keepAliveNode.advertise("timeOut").settable(keepAliveTimeOutHandler);
  #ifdef SONOFFS20
  ledNode.advertise("state").settable(ledNodeHandler);
  #endif
  Homie.onEvent(onHomieEvent);
  reverseMode.setDefaultValue(false).setValidator([] (bool candidate) {
    return (candidate >= 0) && (candidate <= 1);
  });
  Homie.setup();
}

/*
 * Main loop
 */
void loop()
{
  Homie.loop();
}
