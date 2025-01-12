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
const char *boolToChar(bool state)
{
  return state ? "1" : "0";
}

// Function to convert bool to string 1 or 0
bool charToBool(const char *state)
{
  return state == "1";
}

// Function to contruct JSON state string for sending to the server
char *stateToString(stdAc::state_t state)
{
  // Convert degrees float into char
  char *degrees;
  dtostrf(state.degrees, 0, 1, degrees);

  // Construct JSON string
  char *stateString = "{\"deviceID\":\"DEVICE IDENTIFIER\",";
  strcat(stateString, "\"protocol\":\"");
  strcat(stateString, (const char *)state.protocol);
  strcat(stateString, "\",\"model\":\"");
  strcat(stateString, (const char *)state.model);
  strcat(stateString, "\",\"power\":\"");
  strcat(stateString, boolToChar(state.power));
  strcat(stateString, "\",\"mode\":\"");
  strcat(stateString, (const char *)state.mode);
  strcat(stateString, "\",\"degrees\":\"");
  strcat(stateString, degrees);
  strcat(stateString, "\",\"celsius\":\"");
  strcat(stateString, boolToChar(state.celsius));
  strcat(stateString, "\",\"fanspeed\":\"");
  strcat(stateString, (const char *)state.fanspeed);
  // strcat(stateString, "\",\"swingv\":\"");
  // strcat(stateString, state.swingv);
  // strcat(stateString, "\",\"swingh\":\"");
  // strcat(stateString, state.swingh);
  strcat(stateString, "\",\"quiet\":\"");
  strcat(stateString, boolToChar(state.quiet));
  strcat(stateString, "\",\"turbo\":\"");
  strcat(stateString, boolToChar(state.turbo));
  strcat(stateString, "\",\"econo\":\"");
  strcat(stateString, boolToChar(state.econo));
  strcat(stateString, "\",\"light\":\"");
  strcat(stateString, boolToChar(state.light));
  strcat(stateString, "\",\"filter\":\"");
  strcat(stateString, boolToChar(state.filter));
  strcat(stateString, "\",\"clean\":\"");
  strcat(stateString, boolToChar(state.clean));
  strcat(stateString, "\",\"beep\":\"");
  strcat(stateString, boolToChar(state.beep));
  strcat(stateString, "\",\"sleep\":\"");
  strcat(stateString, (const char *)state.sleep);
  strcat(stateString, "\",\"clock\":\"");
  strcat(stateString, (const char *)state.clock);
  strcat(stateString, "\"}");

  return stateString;
}

void setAcNext(const char *optionConst, const char *state)
{
  char *option = (char *)optionConst;

  if (strcmp(option, "model") == 0)
  {
    ac.next.model = ac.strToModel(state);
  }
  else if (strcmp(option, "power") == 0)
  {
    ac.next.power = charToBool(state);
  }
  else if (strcmp(option, "mode") == 0)
  {
    ac.next.mode = ac.strToOpmode(state);
  }
  else if (strcmp(option, "degrees") == 0)
  {
    ac.next.degrees = atof(state);
  }
  else if (strcmp(option, "celsius") == 0)
  {
    ac.next.celsius = charToBool(state);
  }
  else if (strcmp(option, "fanspeed") == 0)
  {
    ac.next.fanspeed = ac.strToFanspeed(state);
  }
  else if (strcmp(option, "quiet") == 0)
  {
    ac.next.quiet = charToBool(state);
  }
  else if (strcmp(option, "turbo") == 0)
  {
    ac.next.turbo = charToBool(state);
  }
  else if (strcmp(option, "econo") == 0)
  {
    ac.next.econo = charToBool(state);
  }
  else if (strcmp(option, "light") == 0)
  {
    ac.next.light = charToBool(state);
  }
  else if (strcmp(option, "filter") == 0)
  {
    ac.next.filter = charToBool(state);
  }
  else if (strcmp(option, "clean") == 0)
  {
    ac.next.clean = charToBool(state);
  }
  else if (strcmp(option, "beep") == 0)
  {
    ac.next.beep = charToBool(state);
  }
  else if (strcmp(option, "sleep") == 0)
  {
    ac.next.sleep = atoi(state);
  }
  else if (strcmp(option, "clock") == 0)
  {
    ac.next.clock = atoi(state);
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
      const char *stateString = stateToString(ac.getState());

      // Send settings to server
      webSocket.sendTXT(stateString);
    }
    else if (requestJson["op"] == "update-settings")
    {
      // Iterate through settings to update
      for (JsonPair pair : requestJson["settings"].as<JsonObject>())
      {
        // Set new state of AC
        setAcNext(pair.key().c_str(), pair.value().as<const char *>());

        // Send new state to AC
        ac.sendAc();
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
    Serial.println(RecvResults.decode_type);

    // Check if protocol detected is supported by library
    if (ac.isProtocolSupported(RecvResults.decode_type))
    {
      // Get state from received char
      stdAc::state_t *state;
      IRAcUtils::decodeToState(&RecvResults, state);

      // Set AC state to received state
      ac.initState(state);
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
}

void loop()
{
  // Check for new websocket messages
  webSocket.loop();

  // If new IR message received
  receiveIR();
}
