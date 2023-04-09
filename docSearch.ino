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
#define GOOGLE_ADD      "https://www.googleapis.com/customsearch/v1?"
#define GOOGLE_KEY      "YOUR_GOOGLE_API_KEY"
#define GOOGLE_CX       "YOUR_CUSTOM_SEARCH"
#define WIFI_SSID       "YOUR_SSID"
#define WIFI_PASSWORD   "YOUR_PASSWORD"
#define BOT_TOKEN       "YOU_TELEGRAM_BOT_KEY"
#define BOT_MTBS        1500
#define GITHUB_TOKEN    "YOUR_GITHUB_API_KEY"
#define GITHUB_ADD      "https://api.github.com/"

long REQ_ID;

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
long bot_lasttime = 0;
WiFiClientSecure secured_client;
DeserializationError error;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

void searchDocs(String SEARCH_QUERY);
String formatSearch(String TEXT_IN);
void sendResult(String rTITLE, String rSNIPPET, String rLINK);
void listRFCS(String USER_ID);
void showRFCS(String USER_ID, long RFCS_NUMBER);
void listSystemScripts(String USER_ID);
void showSystemScripts(String USER_ID, long SCRIPT_NUMBER);
void listReferences(String USER_ID);
void showReference(String USER_ID, long REF_NUMBER);
void listProductionScripts(String USER_ID);
void showProductionScript(String USER_ID, long SCRIPT_NUMBER);
void listMiscScripts(String USER_ID);
void showMiscScript(String USER_ID, long SCRIPT_NUMBER);

void setup() {

  Serial.begin(115200);
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS initialisation failed!");
    while (1) yield();
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
      int queries_request_0_count = queries_request_0["count"]; 
      
      /*      
      const char* queries_request_0_title = queries_request_0["title"]; 
      const char* queries_request_0_totalResults = queries_request_0["totalResults"];
      const char* queries_request_0_searchTerms = queries_request_0["searchTerms"]; 
      int queries_request_0_startIndex = queries_request_0["startIndex"]; 
      const char* queries_request_0_safe = queries_request_0["safe"]; 
      const char* queries_request_0_cx = queries_request_0["cx"]; 
      
      JsonObject queries_nextPage_0 = searchRes["queries"]["nextPage"][0];
      const char* queries_nextPage_0_title = queries_nextPage_0["title"];
      const char* queries_nextPage_0_totalResults = queries_nextPage_0["totalResults"];
      const char* queries_nextPage_0_searchTerms = queries_nextPage_0["searchTerms"]; 
      int queries_nextPage_0_count = queries_nextPage_0["count"]; 
      int queries_nextPage_0_startIndex = queries_nextPage_0["startIndex"]; 
      const char* queries_nextPage_0_inputEncoding = queries_nextPage_0["inputEncoding"]; 
      const char* queries_nextPage_0_outputEncoding = queries_nextPage_0["outputEncoding"]; 
      const char* queries_nextPage_0_safe = queries_nextPage_0["safe"]; 
      const char* queries_nextPage_0_cx = queries_nextPage_0["cx"]; 
      */
      
      JsonArray items = searchRes["items"];

      String REQ_TITLE[queries_request_0_count];
      String REQ_SNIPPET[queries_request_0_count];
      String REQ_LINK[queries_request_0_count];

      for (int i = 0; i < queries_request_0_count; i++) {
        JsonObject cur_items = items[i];
        const char* rtitle = cur_items["title"];
        REQ_TITLE[i] = rtitle;
        const char* rsnippet = cur_items["snippet"];
        REQ_SNIPPET[i] = rsnippet;
        const char* rlink = cur_items["link"];
        REQ_LINK[i] = rlink;
      }

      for (int i = 0; i < queries_request_0_count; i++) {
        sendResult(REQ_TITLE[i], REQ_SNIPPET[i], REQ_LINK[i], USER_ID, i + 1);
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
    String lTEXT = bot.messages[i].text;
    lTEXT.toLowerCase();
    if (bot.messages[i].text.charAt(0) != '/') {
      searchDocs(formatSearch(bot.messages[i].text), bot.messages[i].from_id);
    } else {
      if (lTEXT == "/listrfcs" || lTEXT == "/listrfcs@nervosdocbot") {
        listRFCS(bot.messages[i].from_id);
      } else if (lTEXT.substring(0, 9) == "/showrfcs" && lTEXT.length() > 10 && lTEXT.length() < 15) {
        String R_NUM = lTEXT.substring(10);
        bool allDigit = true;
        char RFCS_LONG[4];
        for (int c = 0; c < R_NUM.length(); c++) {

          if (!isDigit(R_NUM.charAt(c))) {
            allDigit = false;
          } else {
            RFCS_LONG[c] = R_NUM.charAt(c);
          }
        }
        if (allDigit) {
          char *dPTR;
          long OUT_NUMBER = strtol(RFCS_LONG, &dPTR, 10);
          showRFCS(bot.messages[i].from_id, OUT_NUMBER);
        } else {
          bot.sendMessage(bot.messages[i].from_id, "You must submit your request in the format\n\n/showrfcs-XXXX\n\nWhere \"XXXX\" is the RFCS number you want a link to\n\nCall the \"/listrfcs\" command to see the list", "");
        }
      } else if (lTEXT.substring(0, 9) == "/showrfcs" && lTEXT.length() > 14) {

        bot.sendMessage(bot.messages[i].from_id, "Request too long", "");

      } else if (lTEXT.substring(0, 9) == "/showrfcs" && lTEXT.length() > 9 && lTEXT.charAt(9) != '-') {

        bot.sendMessage(bot.messages[i].from_id, "You must submit your request in the format\n\n/showrfcs-XXXX\n\nWhere \"XXXX\" is the RFCS number you want a link to\n\nCall the \"/listrfcs\" command to see the list", "");

      } else if (lTEXT == "/showrfcs" || lTEXT == "/showrfcs@nervosdocbot") {
        bot.sendMessage(bot.messages[i].from_id, "You must submit your request in the format\n\n/showrfcs-XXXX\n\nWhere \"XXXX\" is the RFCS number you want a link to\n\nCall the \"/listrfcs\" command to see the list", "");
      } else if (lTEXT == "/listsystemscripts" || lTEXT == "/listsystemscripts@nervosdocbot") {
        listSystemScripts(bot.messages[i].from_id);
      } else if (lTEXT == "/showsystemscripts" || lTEXT == "/showsystemscripts@nervosdocbot") {
        bot.sendMessage(bot.messages[i].from_id, "You must submit your request in the format\n\n/showsystemscripts-XXXX\n\nWhere \"XXXX\" is the Script number you want a link to\n\nCall the \"/listsystemscripts\" command to see the list", "");
      } else if (lTEXT.substring(0, 18) == "/showsystemscripts" && lTEXT.length() > 19 && lTEXT.length() < 24) {
        String R_NUM = lTEXT.substring(19);
        bool allDigit = true;
        char script_LONG[4];
        for (int c = 0; c < R_NUM.length(); c++) {

          if (!isDigit(R_NUM.charAt(c))) {
            allDigit = false;
          } else {
            script_LONG[c] = R_NUM.charAt(c);
          }
        }
        if (allDigit) {
          char *dPTR;
          long OUT_NUMBER = strtol(script_LONG, &dPTR, 10);
          showSystemScripts(bot.messages[i].from_id, OUT_NUMBER);
        } else {
          bot.sendMessage(bot.messages[i].from_id, "You must submit your request in the format\n\n/showsystemscripts-XXXX\n\nWhere \"XXXX\" is the Script number you want a link to\n\nCall the \"/listsystemscripts\" command to see the list", "");
        }
      } else if (lTEXT.substring(0, 18) == "/showsystemscripts" && lTEXT.length() > 23) {

        bot.sendMessage(bot.messages[i].from_id, "Request too long", "");

      } else if (lTEXT.substring(0, 18) == "/showsystemscripts" && lTEXT.length() > 18 && lTEXT.charAt(19) != '-') {

        bot.sendMessage(bot.messages[i].from_id, "You must submit your request in the format\n\n/showsystemscripts-XXXX\n\nWhere \"XXXX\" is the Script number you want a link to\n\nCall the \"/listsystemscripts\" command to see the list", "");

      } else if (lTEXT == "/listreferences" || lTEXT == "/listreferences@nervosdocbot") {
        listReferences(bot.messages[i].from_id);
      } else if (lTEXT == "/showreference" || lTEXT == "/showreference@nervosdocbot") {
        bot.sendMessage(bot.messages[i].from_id, "You must submit your request in the format\n\n/showreference-XXXX\n\nWhere \"XXXX\" is the Reference number you want a link to\n\nCall the \"/listreferences\" command to see the list", "");
      } else if (lTEXT.substring(0, 14) == "/showreference" && lTEXT.length() > 15 && lTEXT.length() < 20) {
        String R_NUM = lTEXT.substring(15);
        bool allDigit = true;
        char script_LONG[4];
        for (int c = 0; c < R_NUM.length(); c++) {

          if (!isDigit(R_NUM.charAt(c))) {
            allDigit = false;
          } else {
            script_LONG[c] = R_NUM.charAt(c);
          }
        }
        if (allDigit) {
          char *dPTR;
          long OUT_NUMBER = strtol(script_LONG, &dPTR, 10);
          showReference(bot.messages[i].from_id, OUT_NUMBER);
        } else {
          bot.sendMessage(bot.messages[i].from_id, "You must submit your request in the format\n\n/showreference-XXXX\n\nWhere \"XXXX\" is the Reference number you want a link to\n\nCall the \"/listreferences\" command to see the list", "");
        }
      } else if (lTEXT.substring(0, 14) == "/showreference" && lTEXT.length() > 19) {

        bot.sendMessage(bot.messages[i].from_id, "Request too long", "");

      } else if (lTEXT.substring(0, 14) == "/showreference" && lTEXT.length() > 14 && lTEXT.charAt(15) != '-') {

        bot.sendMessage(bot.messages[i].from_id, "You must submit your request in the format\n\n/showreference-XXXX\n\nWhere \"XXXX\" is the Reference number you want a link to\n\nCall the \"/listreferences\" command to see the list", "");

      } else if (lTEXT == "/listproductionscripts" || lTEXT == "/listproductionscripts@nervosdocbot") {
        listProductionScripts(bot.messages[i].from_id);
      } else if (lTEXT == "/showprodscript" || lTEXT == "/showprodscript@nervosdocbot") {
        bot.sendMessage(bot.messages[i].from_id, "You must submit your request in the format\n\n/showprodscript-XXXX\n\nWhere \"XXXX\" is the Script number you want a link to\n\nCall the \"/listproductionscripts\" command to see the list", "");
      } else if (lTEXT.substring(0, 15) == "/showprodscript" && lTEXT.length() > 16 && lTEXT.length() < 21) {
        String R_NUM = lTEXT.substring(16);
        bool allDigit = true;
        char script_LONG[4];
        for (int c = 0; c < R_NUM.length(); c++) {

          if (!isDigit(R_NUM.charAt(c))) {
            allDigit = false;
          } else {
            script_LONG[c] = R_NUM.charAt(c);
          }
        }
        if (allDigit) {
          char *dPTR;
          long OUT_NUMBER = strtol(script_LONG, &dPTR, 10);
          showProductionScript(bot.messages[i].from_id, OUT_NUMBER);
        } else {
          bot.sendMessage(bot.messages[i].from_id, "You must submit your request in the format\n\n/showprodscript-XXXX\n\nWhere \"XXXX\" is the Script number you want a link to\n\nCall the \"/listproductionscripts\" command to see the list", "");
        }
      } else if (lTEXT.substring(0, 15) == "/showprodscript" && lTEXT.length() > 20) {

        bot.sendMessage(bot.messages[i].from_id, "Request too long", "");

      } else if (lTEXT.substring(0, 15) == "/showprodscript" && lTEXT.length() > 15 && lTEXT.charAt(16) != '-') {

        bot.sendMessage(bot.messages[i].from_id, "You must submit your request in the format\n\n/showprodscript-XXXX\n\nWhere \"XXXX\" is the Script number you want a link to\n\nCall the \"/listproductionscripts\" command to see the list", "");

      } else if (lTEXT == "/listmiscscripts" || lTEXT == "/listmiscscripts@nervosdocbot") {
        listMiscScripts(bot.messages[i].from_id);
      } else if (lTEXT == "/showmiscscript" || lTEXT == "/showmiscscript@nervosdocbot") {
        bot.sendMessage(bot.messages[i].from_id, "You must submit your request in the format\n\n/showmiscscript-XXXX\n\nWhere \"XXXX\" is the Script number you want a link to\n\nCall the \"/listmiscscripts\" command to see the list", "");
      } else if (lTEXT.substring(0, 15) == "/showmiscscript" && lTEXT.length() > 16 && lTEXT.length() < 21) {
        String R_NUM = lTEXT.substring(16);
        bool allDigit = true;
        char script_LONG[4];
        for (int c = 0; c < R_NUM.length(); c++) {

          if (!isDigit(R_NUM.charAt(c))) {
            allDigit = false;
          } else {
            script_LONG[c] = R_NUM.charAt(c);
          }
        }
        if (allDigit) {
          char *dPTR;
          long OUT_NUMBER = strtol(script_LONG, &dPTR, 10);
          showMiscScript(bot.messages[i].from_id, OUT_NUMBER);
        } else {
          bot.sendMessage(bot.messages[i].from_id, "You must submit your request in the format\n\n/showmiscscript-XXXX\n\nWhere \"XXXX\" is the Script number you want a link to\n\nCall the \"/listmiscscripts\" command to see the list", "");
        }
      } else if (lTEXT.substring(0, 15) == "/showmiscscript" && lTEXT.length() > 20) {

        bot.sendMessage(bot.messages[i].from_id, "Request too long", "");

      } else if (lTEXT.substring(0, 15) == "/showmiscscript" && lTEXT.length() > 15 && lTEXT.charAt(16) != '-') {

        bot.sendMessage(bot.messages[i].from_id, "You must submit your request in the format\n\n/showmiscscript-XXXX\n\nWhere \"XXXX\" is the Script number you want a link to\n\nCall the \"/listmiscscripts\" command to see the list", "");

      }
      
    }
  }
}

String formatSearch(String TEXT_IN) {
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
void sendResult(String rTITLE, String rSNIPPET, String rLINK, String USER_ID, int rNUMBER) {
  String MSG_OUT = "Result " + String(rNUMBER) + "\n\n" + rTITLE + "\n\n" + rSNIPPET + "\n\n" + rLINK;
  bot.sendMessage(USER_ID, MSG_OUT, "");
}

void listRFCS(String USER_ID) {
  SpiRamJsonDocument searchRes(100000);
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String urlSender = String(GITHUB_ADD) + "repos/nervosnetwork/rfcs/contents/rfcs";
    http.begin(urlSender);
    http.addHeader("Content-Type", "application/vnd.github+json");
    http.addHeader("Accept", "application/vnd.github+json");
    http.addHeader("Authorization" , ("Bearer " + String(GITHUB_TOKEN)));
    http.addHeader("X-GitHub-Api-Version", "2022-11-28");

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      //Serial.println(payload);
      error = deserializeJson(searchRes, payload);

      if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
      }
      String MSG_OUT = "";
      for (JsonObject item : searchRes.as<JsonArray>()) {

        const char* repo_name = item["name"]; // "0001-positioning", "0002-ckb", "0003-ckb-vm", ...
        //const char* path = item["path"]; // "rfcs/0001-positioning", "rfcs/0002-ckb", "rfcs/0003-ckb-vm", ...
        //const char* sha = item["sha"]; // "b0acd5db5b368201027018cc9bf45355976facce", ...
        //int repo_size = item["size"]; // 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ...
        //const char* url = item["url"];
        const char* html_url = item["html_url"];
        //const char* git_url = item["git_url"];
        // item["download_url"] is null
        //const char* repo_type = item["type"]; // "dir", "dir", "dir", "dir", "dir", "dir", "dir", "dir", "dir", ...

        //JsonObject links = item["_links"];
        //const char* links_self = links["self"];
        //const char* links_git = links["git"];
        //const char* links_html = links["html"];
        MSG_OUT += String(repo_name) + "\n";
      }

      bot.sendMessage(USER_ID, MSG_OUT, "");
    }
  }
}

void showRFCS(String USER_ID, long RFCS_NUMBER) {
  SpiRamJsonDocument searchRes(100000);
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String urlSender = String(GITHUB_ADD) + "repos/nervosnetwork/rfcs/contents/rfcs";
    http.begin(urlSender);
    http.addHeader("Content-Type", "application/vnd.github+json");
    http.addHeader("Accept", "application/vnd.github+json");
    http.addHeader("Authorization" , ("Bearer " + String(GITHUB_TOKEN)));
    http.addHeader("X-GitHub-Api-Version", "2022-11-28");

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      //Serial.println(payload);
      error = deserializeJson(searchRes, payload);

      if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
      }
      String MSG_OUT = "";
      for (JsonObject item : searchRes.as<JsonArray>()) {

        const char* repo_name = item["name"]; // "0001-positioning", "0002-ckb", "0003-ckb-vm", ...
        //const char* path = item["path"]; // "rfcs/0001-positioning", "rfcs/0002-ckb", "rfcs/0003-ckb-vm", ...
        //const char* sha = item["sha"]; // "b0acd5db5b368201027018cc9bf45355976facce", ...
        //int repo_size = item["size"]; // 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ...
        //const char* url = item["url"];
        const char* html_url = item["html_url"];
        //const char* git_url = item["git_url"];
        // item["download_url"] is null
        //const char* repo_type = item["type"]; // "dir", "dir", "dir", "dir", "dir", "dir", "dir", "dir", "dir", ...

        //JsonObject links = item["_links"];
        //const char* links_self = links["self"];
        //const char* links_git = links["git"];
        //const char* links_html = links["html"];

        char REPO_CHECK[4];
        char *rPTR;
        for (int c = 0; c < 4; c++) {
          REPO_CHECK[c] = repo_name[c];
        }
        long RNUM = strtol(REPO_CHECK, &rPTR, 10);

        if (RNUM == RFCS_NUMBER) {
          MSG_OUT = String(html_url);
          bot.sendMessage(USER_ID, MSG_OUT, "");
          return;
        }
      }


    }
  }
}

void listSystemScripts(String USER_ID) {
  SpiRamJsonDocument searchRes(100000);
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String urlSender = String(GITHUB_ADD) + "repos/nervosnetwork/ckb-system-scripts/contents/c";
    http.begin(urlSender);
    http.addHeader("Content-Type", "application/vnd.github+json");
    http.addHeader("Accept", "application/vnd.github+json");
    http.addHeader("Authorization" , ("Bearer " + String(GITHUB_TOKEN)));
    http.addHeader("X-GitHub-Api-Version", "2022-11-28");

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      //Serial.println(payload);
      error = deserializeJson(searchRes, payload);

      if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
      }
      String MSG_OUT = "";
      int SCRIPT_ID = 1;
      for (JsonObject item : searchRes.as<JsonArray>()) {

        const char* script_name = item["name"]; // "blake2b.h", "blockchain.mol", "ckb_consts.h", "ckb_syscalls.h", ...
        //const char* path = item["path"]; // "c/blake2b.h", "c/blockchain.mol", "c/ckb_consts.h", ...
        //const char* sha = item["sha"]; // "a0f5810b7edbdf828ff5eaa26440b264e2f1db8d", ...
        //long script_size = item["size"]; // 14236, 2357, 1305, 5345, 3547, 23442, 1792, 93132, 14616, 11743, 2724, 1801
        //const char* url = item["url"];
        const char* html_url = item["html_url"];
        //const char* git_url = item["git_url"];
        //const char* download_url = item["download_url"];
        //const char* script_type = item["type"]; // "file", "file", "file", "file", "file", "file", "file", "file", ...

        //JsonObject links = item["_links"];
        //const char* links_self = links["self"];
        //const char* links_git = links["git"];
        //const char* links_html = links["html"];

        MSG_OUT += String(SCRIPT_ID) + ") " + String(script_name) + "\n";
        SCRIPT_ID++;
      }

      bot.sendMessage(USER_ID, MSG_OUT, "");
    }
  }

}

void showSystemScripts(String USER_ID, long SCRIPT_NUMBER) {
  SpiRamJsonDocument searchRes(100000);
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String urlSender = String(GITHUB_ADD) + "repos/nervosnetwork/ckb-system-scripts/contents/c";
    http.begin(urlSender);
    http.addHeader("Content-Type", "application/vnd.github+json");
    http.addHeader("Accept", "application/vnd.github+json");
    http.addHeader("Authorization" , ("Bearer " + String(GITHUB_TOKEN)));
    http.addHeader("X-GitHub-Api-Version", "2022-11-28");

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      //Serial.println(payload);
      error = deserializeJson(searchRes, payload);

      if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
      }
      String MSG_OUT = "";
      int SCRIPT_ID = 1;
      long TOTAL_BYTES = 0;
      for (JsonObject item : searchRes.as<JsonArray>()) {

        const char* script_name = item["name"]; // "blake2b.h", "blockchain.mol", "ckb_consts.h", "ckb_syscalls.h", ...
        //const char* path = item["path"]; // "c/blake2b.h", "c/blockchain.mol", "c/ckb_consts.h", ...
        //const char* sha = item["sha"]; // "a0f5810b7edbdf828ff5eaa26440b264e2f1db8d", ...
        long script_size = item["size"]; // 14236, 2357, 1305, 5345, 3547, 23442, 1792, 93132, 14616, 11743, 2724, 1801
        //const char* url = item["url"];
        const char* html_url = item["html_url"];
        //const char* git_url = item["git_url"];
        const char* download_url = item["download_url"];
        //const char* script_type = item["type"]; // "file", "file", "file", "file", "file", "file", "file", "file", ...

        //JsonObject links = item["_links"];
        //const char* links_self = links["self"];
        //const char* links_git = links["git"];
        //const char* links_html = links["html"];
        

        if (SCRIPT_ID == SCRIPT_NUMBER) {
          MSG_OUT = String(html_url);
          bot.sendMessage(USER_ID, MSG_OUT, "");
          return;
        }
        TOTAL_BYTES += script_size;
        SCRIPT_ID++;
      }
      Serial.println("Bytes in Repo: " + String(TOTAL_BYTES));
    }
  }
}

void listReferences(String USER_ID){
  String REF_LIST_OUT = "";

  REF_LIST_OUT += "01) CKB light client reference implementation\n";
  REF_LIST_OUT += "02) Merkle mountain range\n";
  REF_LIST_OUT += "03) RISC-V V testcases\n";
  REF_LIST_OUT += "04) Ed25519\n";
  REF_LIST_OUT += "05) EVM One\n";
  REF_LIST_OUT += "06) Sparse merkle tree\n";
  REF_LIST_OUT += "07) Parity JSON-RPC\n";
  REF_LIST_OUT += "08) rust-libp2p\n";
  REF_LIST_OUT += "09) CKB Explorer API Reference\n";
  REF_LIST_OUT += "10) ckb-std\n";
  REF_LIST_OUT += "11) CKB Standalone Debugger\n";
  REF_LIST_OUT += "12) Molecule\n";
  REF_LIST_OUT += "13) Capsule\n";
  REF_LIST_OUT += "14) CKB SDK Ruby\n";
  REF_LIST_OUT += "15) CKB SDK Rust\n";
  REF_LIST_OUT += "16) CKB SDK Java\n";
  REF_LIST_OUT += "17) CKB C stdlib\n";
  REF_LIST_OUT += "18) Mercury\n";
  REF_LIST_OUT += "19) CKB Lua\n";
  REF_LIST_OUT += "20) CKB SDK Golang\n";

  bot.sendMessage(USER_ID, REF_LIST_OUT, "");
}

void showReference(String USER_ID, long REF_ID){
  String REPO_LOC = "";
  
  switch(REF_ID){
    case 1:
      REPO_LOC = "https://github.com/nervosnetwork/ckb-light-client";
      break;
    case 2:
      REPO_LOC = "https://github.com/nervosnetwork/merkle-mountain-range";
      break;
    case 3:
      REPO_LOC = "https://github.com/nervosnetwork/rvv-testcases";
      break;
    case 4:
      REPO_LOC = "https://github.com/nervosnetwork/ed25519";
      break;
    case 5:
      REPO_LOC = "https://github.com/nervosnetwork/evmone";
      break;
    case 6:
      REPO_LOC = "https://github.com/nervosnetwork/sparse-merkle-tree";
      break;
    case 7:
      REPO_LOC = "https://github.com/nervosnetwork/jsonrpc";
      break;
    case 8:
      REPO_LOC = "https://github.com/nervosnetwork/rust-libp2p";
      break;
    case 9:
      REPO_LOC = "https://onnhbgvkmy.apifox.cn/";
      break;
    case 10:
      REPO_LOC = "https://github.com/nervosnetwork/ckb-std";
      break;
    case 11:
      REPO_LOC = "https://github.com/nervosnetwork/ckb-standalone-debugger";
      break;
    case 12:
      REPO_LOC = "https://github.com/nervosnetwork/molecule";
      break;
    case 13:
      REPO_LOC = "https://github.com/nervosnetwork/capsule";
      break;
    case 14:
      REPO_LOC = "https://github.com/nervosnetwork/ckb-sdk-ruby";
      break;
    case 15:
      REPO_LOC = "https://github.com/nervosnetwork/ckb-sdk-rust";
      break;
    case 16:
      REPO_LOC = "https://github.com/nervosnetwork/ckb-sdk-java";
      break;
    case 17:
      REPO_LOC = "https://github.com/nervosnetwork/ckb-c-stdlib";
      break;
    case 18:
      REPO_LOC = "https://github.com/nervosnetwork/mercury";
      break;
    case 19:
      REPO_LOC = "https://github.com/nervosnetwork/ckb-lua";
      break;
    case 20:
      REPO_LOC = "https://github.com/nervosnetwork/ckb-sdk-go";
      break;
  }
  bot.sendMessage(USER_ID, REPO_LOC, "");
}

void listProductionScripts(String USER_ID) {
  SpiRamJsonDocument searchRes(100000);
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String urlSender = String(GITHUB_ADD) + "repos/nervosnetwork/ckb-production-scripts/contents/c";
    http.begin(urlSender);
    http.addHeader("Content-Type", "application/vnd.github+json");
    http.addHeader("Accept", "application/vnd.github+json");
    http.addHeader("Authorization" , ("Bearer " + String(GITHUB_TOKEN)));
    http.addHeader("X-GitHub-Api-Version", "2022-11-28");

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      //Serial.println(payload);
      error = deserializeJson(searchRes, payload);

      if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
      }
      String MSG_OUT = "";
      int SCRIPT_ID = 1;
      for (JsonObject item : searchRes.as<JsonArray>()) {

        const char* script_name = item["name"]; 
        //const char* path = item["path"]; 
        //const char* sha = item["sha"]; 
        //long script_size = item["size"]; 
        //const char* url = item["url"];
        const char* html_url = item["html_url"];
        //const char* git_url = item["git_url"];
        //const char* download_url = item["download_url"];
        //const char* script_type = item["type"]; 

        //JsonObject links = item["_links"];
        //const char* links_self = links["self"];
        //const char* links_git = links["git"];
        //const char* links_html = links["html"];

        MSG_OUT += String(SCRIPT_ID) + ") " + String(script_name) + "\n";
        SCRIPT_ID++;
      }

      bot.sendMessage(USER_ID, MSG_OUT, "");
    }
  }

}

void showProductionScript(String USER_ID, long SCRIPT_NUMBER) {
  SpiRamJsonDocument searchRes(100000);
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String urlSender = String(GITHUB_ADD) + "repos/nervosnetwork/ckb-production-scripts/contents/c";
    http.begin(urlSender);
    http.addHeader("Content-Type", "application/vnd.github+json");
    http.addHeader("Accept", "application/vnd.github+json");
    http.addHeader("Authorization" , ("Bearer " + String(GITHUB_TOKEN)));
    http.addHeader("X-GitHub-Api-Version", "2022-11-28");

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      //Serial.println(payload);
      error = deserializeJson(searchRes, payload);

      if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
      }
      String MSG_OUT = "";
      int SCRIPT_ID = 1;
      long TOTAL_BYTES = 0;
      for (JsonObject item : searchRes.as<JsonArray>()) {

        const char* script_name = item["name"];
        //const char* path = item["path"]; 
        //const char* sha = item["sha"]; 
        long script_size = item["size"]; 
        //const char* url = item["url"];
        const char* html_url = item["html_url"];
        //const char* git_url = item["git_url"];
        const char* download_url = item["download_url"];
        //const char* script_type = item["type"]; 

        //JsonObject links = item["_links"];
        //const char* links_self = links["self"];
        //const char* links_git = links["git"];
        //const char* links_html = links["html"];
        

        if (SCRIPT_ID == SCRIPT_NUMBER) {
          MSG_OUT = String(html_url);
          bot.sendMessage(USER_ID, MSG_OUT, "");
          return;
        }
        TOTAL_BYTES += script_size;
        SCRIPT_ID++;
      }
      Serial.println("Bytes in Repo: " + String(TOTAL_BYTES));
    }
  }
}

void listMiscScripts(String USER_ID) {
  SpiRamJsonDocument searchRes(100000);
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String urlSender = String(GITHUB_ADD) + "repos/nervosnetwork/ckb-miscellaneous-scripts/contents/c";
    http.begin(urlSender);
    http.addHeader("Content-Type", "application/vnd.github+json");
    http.addHeader("Accept", "application/vnd.github+json");
    http.addHeader("Authorization" , ("Bearer " + String(GITHUB_TOKEN)));
    http.addHeader("X-GitHub-Api-Version", "2022-11-28");

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      //Serial.println(payload);
      error = deserializeJson(searchRes, payload);

      if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
      }
      String MSG_OUT = "";
      int SCRIPT_ID = 1;
      for (JsonObject item : searchRes.as<JsonArray>()) {

        const char* script_name = item["name"];
        //const char* path = item["path"]; 
        //const char* sha = item["sha"]; 
        //long script_size = item["size"];
        //const char* url = item["url"];
        const char* html_url = item["html_url"];
        //const char* git_url = item["git_url"];
        //const char* download_url = item["download_url"];
        //const char* script_type = item["type"]; 

        //JsonObject links = item["_links"];
        //const char* links_self = links["self"];
        //const char* links_git = links["git"];
        //const char* links_html = links["html"];

        MSG_OUT += String(SCRIPT_ID) + ") " + String(script_name) + "\n";
        SCRIPT_ID++;
      }

      bot.sendMessage(USER_ID, MSG_OUT, "");
    }
  }

}

void showMiscScript(String USER_ID, long SCRIPT_NUMBER) {
  SpiRamJsonDocument searchRes(100000);
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String urlSender = String(GITHUB_ADD) + "repos/nervosnetwork/ckb-miscellaneous-scripts/contents/c";
    http.begin(urlSender);
    http.addHeader("Content-Type", "application/vnd.github+json");
    http.addHeader("Accept", "application/vnd.github+json");
    http.addHeader("Authorization" , ("Bearer " + String(GITHUB_TOKEN)));
    http.addHeader("X-GitHub-Api-Version", "2022-11-28");

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      //Serial.println(payload);
      error = deserializeJson(searchRes, payload);

      if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
      }
      String MSG_OUT = "";
      int SCRIPT_ID = 1;
      long TOTAL_BYTES = 0;
      for (JsonObject item : searchRes.as<JsonArray>()) {

        const char* script_name = item["name"];
        //const char* path = item["path"]; 
        //const char* sha = item["sha"]; 
        long script_size = item["size"]; 
        //const char* url = item["url"];
        const char* html_url = item["html_url"];
        //const char* git_url = item["git_url"];
        const char* download_url = item["download_url"];
        //const char* script_type = item["type"]; 

        //JsonObject links = item["_links"];
        //const char* links_self = links["self"];
        //const char* links_git = links["git"];
        //const char* links_html = links["html"];
        

        if (SCRIPT_ID == SCRIPT_NUMBER) {
          MSG_OUT = String(html_url);
          bot.sendMessage(USER_ID, MSG_OUT, "");
          return;
        }
        TOTAL_BYTES += script_size;
        SCRIPT_ID++;
      }
      Serial.println("Bytes in Repo: " + String(TOTAL_BYTES));
    }
  }
}
