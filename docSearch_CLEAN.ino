#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <time.h>
#include <TimeLib.h>
#include <HTTPClient.h>
#include <ESP32Time.h>
#include <stdint.h>
#include <stdio.h>
#include <SPIFFS.h>
#define GOOGLE_ADD      "https://www.googleapis.com/customsearch/v1?" //Base Custom Google Search Address
#define GOOGLE_KEY      "YOUR GOOGLE API KEY HERE"
#define GOOGLE_CX       "CUSTOM CHAT ID HERE"
#define WIFI_SSID       "YOUR SSID HERE"
#define WIFI_PASSWORD   "YOU PASSWORD HERE"
#define BOT_TOKEN       "YOUR TELEGRAM BOT API KEY HERE"
#define BOT_MTBS        1500  //bot check frequency

struct SpiRamAllocator {
  void* allocate(size_t size) {
    return heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
  }
  void deallocate(void* pointer) {
    heap_caps_free(pointer);
  }
  void* reallocate(void* ptr, size_t new_size) {
    return heap_caps_realloc(ptr, new_size, MALLOC_CAP_SPIRAM);
  }
};

using SpiRamJsonDocument = BasicJsonDocument<SpiRamAllocator>;
long REQ_ID;
long bot_lasttime = 0;
WiFiClientSecure secured_client;
DeserializationError error;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

void searchDocs(String SEARCH_QUERY);
String formatSearch(String TEXT_IN);
void sendResult(String rTITLE, String rSNIPPET, String rLINK);

void setup() {

  Serial.begin(115200);
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS initialisation failed!");
    while (1) yield(); // Stay here twiddling thumbs waiting
  }
  if (!psramInit()) {
    Serial.println("Error Initialising PSRam");
  }
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println(WiFi.localIP());
}

void loop() {

  if (millis() - bot_lasttime > BOT_MTBS)
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages)
    {
      Serial.println("got response(s): " + String(numNewMessages));
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    bot_lasttime = millis();
  }
}

void searchDocs(String SEARCH_QUERY, String USER_ID) {

  SpiRamJsonDocument searchRes(100000);
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String urlSender = String(GOOGLE_ADD) + "key=" + String(GOOGLE_KEY) + "&cx=" + String(GOOGLE_CX) + "&num=10&q=" + SEARCH_QUERY;
    http.begin(urlSender);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      Serial.println(payload);
      error = deserializeJson(searchRes, payload);

      if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
      }
      JsonObject queries_request_0 = searchRes["queries"]["request"][0];
      const char* queries_request_0_title = queries_request_0["title"]; // "Google Custom Search - Cell model"
      const char* queries_request_0_totalResults = queries_request_0["totalResults"]; // "209000"
      const char* queries_request_0_searchTerms = queries_request_0["searchTerms"]; // "Cell model"
      int queries_request_0_count = queries_request_0["count"]; // 10
      int queries_request_0_startIndex = queries_request_0["startIndex"]; // 1
      const char* queries_request_0_safe = queries_request_0["safe"]; // "off"
      const char* queries_request_0_cx = queries_request_0["cx"]; // "b402386af75ba4a26"

      JsonObject queries_nextPage_0 = searchRes["queries"]["nextPage"][0];
      const char* queries_nextPage_0_title = queries_nextPage_0["title"]; // "Google Custom Search - Cell ...
      const char* queries_nextPage_0_totalResults = queries_nextPage_0["totalResults"]; // "209000"
      const char* queries_nextPage_0_searchTerms = queries_nextPage_0["searchTerms"]; // "Cell model"
      int queries_nextPage_0_count = queries_nextPage_0["count"]; // 10
      int queries_nextPage_0_startIndex = queries_nextPage_0["startIndex"]; // 11
      const char* queries_nextPage_0_inputEncoding = queries_nextPage_0["inputEncoding"]; // "utf8"
      const char* queries_nextPage_0_outputEncoding = queries_nextPage_0["outputEncoding"]; // "utf8"
      const char* queries_nextPage_0_safe = queries_nextPage_0["safe"]; // "off"
      const char* queries_nextPage_0_cx = queries_nextPage_0["cx"]; // "b402386af75ba4a26"

      JsonArray items = searchRes["items"];

      String REQ_TITLE[queries_request_0_count];
      String REQ_SNIPPET[queries_request_0_count];
      String REQ_LINK[queries_request_0_count];
      
      for(int i=0;i<queries_request_0_count;i++){
        JsonObject cur_items = items[i];
        const char* rtitle = cur_items["title"];
        REQ_TITLE[i] = rtitle;
        const char* rsnippet = cur_items["snippet"];
        REQ_SNIPPET[i] = rsnippet;
        const char* rlink = cur_items["link"];
        REQ_LINK[i] = rlink;
      }
        
      for(int i=0;i<queries_request_0_count;i++){
        sendResult(REQ_TITLE[i], REQ_SNIPPET[i], REQ_LINK[i], USER_ID, i+1);
        //delay(2000);
      }
    }
  }

}

void handleNewMessages(int numNewMessages)
{
  for (int i = 0; i < numNewMessages; i++)
  {
    Serial.println(numNewMessages);
    Serial.println(bot.messages[i].text);
    searchDocs(formatSearch(bot.messages[i].text), bot.messages[i].from_id);
  }
}

String formatSearch(String TEXT_IN){
  String RETURN_TEXT = "";

  int LEN = TEXT_IN.length();

  for (int l = 0; l < LEN; l++) {
    if (TEXT_IN.charAt(l) == ' ') {
      RETURN_TEXT += "%20";
    } else {
      RETURN_TEXT += TEXT_IN.charAt(l);
    }
  }
  return RETURN_TEXT;

}

void sendResult(String rTITLE, String rSNIPPET, String rLINK, String USER_ID, int rNUMBER){
  String MSG_OUT = "Result " + String(rNUMBER) + "\n\n" + rTITLE + "\n\n" + rSNIPPET + "\n\n" + rLINK;
  bot.sendMessage(USER_ID, MSG_OUT, "");
}
