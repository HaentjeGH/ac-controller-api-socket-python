#include <WiFi.h>             // Library used for wifi connection
#include <ArduinoJson.h>      // Library used for JSON parsing
#include <WebSocketsClient.h> // Library used for websocket: https://github.com/Links2004/arduinoWebSockets

#include <Arduino.h>
#include <IRremoteESP8266.h> // Library used for IR transmission: https://github.com/crankyoldgit/IRremoteESP8266
#include <IRac.h>
#include <IRutils.h>
#include <IRrecv.h>

// WiFi credentials
char *WIFI_SSID = "FRITZ!Box 7560 BH";
char *WIFI_PASSWORD = "72117123858228781469";

// Commom AC class
const uint16_t kSendPin = 2;
// IRsend irsend(kSendPin);
IRac ac(kSendPin);
stdAc::state_t state;

// IR Receive class
const uint16_t kRecvPin = 22;
IRrecv irrecv(kRecvPin);

// LED pins
const int Red_LED_pin = 32;
const int Green_LED_pin = 33;

// Client indentifier
char *deviceID = "DEVICE IDENTIFIER";

// Web socket client
WebSocketsClient webSocket;
bool socketConnected = false;

// Function to convert bool to string 1 or 0
bool charToBool(const char *state)
{
  return state == "1";
}

char *stringToChar(String string)
{
  int len = string.length() + 1;
  char *buf = new char[len];
  string.toCharArray(buf, len);

  return buf;
}

// Function to contruct JSON state string for sending to the server
String stateToString(stdAc::state_t state)
{
  // Construct JSON string
  String stateString = "\{\"deviceID\":\"DEVICE IDENTIFIER";
  stateString = stateString + "\",\"protocol\":\"" + state.protocol;
  stateString = stateString + "\",\"model\":\""    + state.model;
  stateString = stateString + "\",\"power\":\""    + state.power;
  stateString = stateString + "\",\"mode\":\""     + ac.opmodeToString(state.mode);
  stateString = stateString + "\",\"degrees\":\""  + state.degrees;
  stateString = stateString + "\",\"celsius\":\""  + state.celsius;
  stateString = stateString + "\",\"fanspeed\":\"" + ac.fanspeedToString(state.fanspeed);
  stateString = stateString + "\",\"swingv\":\""   + ac.swingvToString(state.swingv);
  stateString = stateString + "\",\"swingh\":\""   + ac.swinghToString(state.swingh);
  stateString = stateString + "\",\"quiet\":\""    + state.celsius;
  stateString = stateString + "\",\"turbo\":\""    + state.turbo;
  stateString = stateString + "\",\"econo\":\""    + state.econo;
  stateString = stateString + "\",\"light\":\""    + state.light;
  stateString = stateString + "\",\"filter\":\""   + state.filter;
  stateString = stateString + "\",\"clean\":\""    + state.clean;
  stateString = stateString + "\",\"beep\":\""     + state.beep;
  stateString = stateString + "\",\"sleep\":\""    + state.sleep;
  stateString = stateString + "\",\"clock\":\""    + state.clock;
  stateString = stateString + "\"}";

  return stateString;
}

void setAcNextState(const char *optionConst, const char *stateValue)
{
  char *option = (char *)optionConst;

  if (strcmp(option, "model") == 0)
  {
    state.model = ac.strToModel(stateValue);
  }
  else if (strcmp(option, "power") == 0)
  {
    state.power = charToBool(stateValue);
  }
  else if (strcmp(option, "mode") == 0)
  {
    state.mode = ac.strToOpmode(stateValue);
  }
  else if (strcmp(option, "degrees") == 0)
  {
    state.degrees = atof(stateValue);
  }
  else if (strcmp(option, "celsius") == 0)
  {
    state.celsius = charToBool(stateValue);
  }
  else if (strcmp(option, "fanspeed") == 0)
  {
    state.fanspeed = ac.strToFanspeed(stateValue);
  }
  else if (strcmp(option, "quiet") == 0)
  {
    state.quiet = charToBool(stateValue);
  }
  else if (strcmp(option, "turbo") == 0)
  {
    state.turbo = charToBool(stateValue);
  }
  else if (strcmp(option, "econo") == 0)
  {
    state.econo = charToBool(stateValue);
  }
  else if (strcmp(option, "light") == 0)
  {
    state.light = charToBool(stateValue);
  }
  else if (strcmp(option, "filter") == 0)
  {
    state.filter = charToBool(stateValue);
  }
  else if (strcmp(option, "clean") == 0)
  {
    state.clean = charToBool(stateValue);
  }
  else if (strcmp(option, "beep") == 0)
  {
    state.beep = charToBool(stateValue);
  }
  else if (strcmp(option, "sleep") == 0)
  {
    state.sleep = atoi(stateValue);
  }
  else if (strcmp(option, "clock") == 0)
  {
    state.clock = atoi(stateValue);
  }
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_DISCONNECTED:
    // Set socket connection to false and print disconnected once
    if (socketConnected == true)
    {
      Serial.println("[WSc] Disconnected!");
    }
    socketConnected = false;

    break;
  case WStype_CONNECTED:
    Serial.println("[WSc] Connected to websocket");

    // Set socket connection to true
    socketConnected = true;
    break;
  case WStype_TEXT:
  {
    Serial.printf("[WSc] %s\n", payload);

    if (strcmp((char *)payload, "Server is working!") == 0)
    {
      // Send device ID to server
      webSocket.sendTXT(deviceID);

      // Break early
      break;
    }

    // Parse request data
    DynamicJsonDocument requestJson(1024);
    deserializeJson(requestJson, (char *)payload);

    if (requestJson["op"] == "get-settings")
    {
      // Get JSON state char
      const char *stateString = stringToChar(stateToString(state));

      // Send settings to server
      webSocket.sendTXT(stateString);
    }
    else if (requestJson["op"] == "update-settings")
    {
      // Iterate through settings to update
      for (JsonPair pair : requestJson["settings"].as<JsonObject>())
      {
        // Set new state of AC
        setAcNextState(pair.key().c_str(), pair.value().as<const char *>());

        // Send new state to AC
        ac.sendAc(state, &state);
      }
    }

    break;
  }
  default:
    break;
  }
}

void receiveIR()
{
  // IR receive results
  decode_results RecvResults;

  // Decode results
  if (irrecv.decode(&RecvResults))
  {
    // Check if protocol detected is supported by library
    if (ac.isProtocolSupported(RecvResults.decode_type))
    {
      // Get state from received char
      IRAcUtils::decodeToState(&RecvResults, &state);

      ac.sendAc(state, &state);
    }

    // Receive the next value
    irrecv.resume();
  }
}

void setup()
{
  Serial.begin(115200);

  // Connect to WiFi network
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Begin websocket
  webSocket.beginSSL("accontroller.tbrouwer.com", 443, "/ws");
  webSocket.onEvent(webSocketEvent);

  // Start the receiver
  irrecv.enableIRIn();
  
  // Temporarily set defaults
  state.protocol = decode_type_t::MITSUBISHI_HEAVY_152;
  state.model = 1;
  state.power = true;
  state.mode = stdAc::opmode_t::kAuto;
  state.degrees = 22;
  state.fanspeed = stdAc::fanspeed_t::kAuto;
  state.swingv = stdAc::swingv_t::kAuto;
  state.swingh = stdAc::swingh_t::kOff;
  state.quiet = true;
  state.turbo = false;
  state.econo = false;
  state.light = false;
  state.filter = false;
  state.clean = false;
  state.beep = false;
  state.sleep = -1;
  state.clock = -1;
  ac.sendAc(state, &state);
}

void loop()
{
  // Check for new websocket messages
  webSocket.loop();

  // If new IR message received
  receiveIR();
}
