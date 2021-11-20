/*
Copyright 2021 Maurice Dupont

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <time.h>
#include <Arduino_JSON.h>
#include "Credentials.h"   

String btn[5];

unsigned short int kw, lastkw;

String names[5];
bool done[5];

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

ESP8266WebServer server(80);

WiFiClient client;
HTTPClient http;

void setup() {
  Serial.begin(9600);
  delay(100);  

  Serial.println("Connecting to ");
  Serial.println(ssid);

  //connect to your local wi-fi network
  WiFi.begin(ssid, pass);

  //check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED) {
  delay(1000);
  Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected..!");
  Serial.print("Got IP: ");  Serial.println(WiFi.localIP());

  server.on("/", handle_root);
  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("HTTP server started");

  lastkw = getWeekOfYear();
}

void loop() {
  if(WiFi.status()== WL_CONNECTED) {
    server.handleClient();
  } else {
    Serial.println("WiFi Disconnected");
  }
}

void run_update(String name, int id) {
    String serverPath = httpserver + "/rotate";
    http.begin(client, serverPath);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String data = "name="+name+"&id="+String(id);
    int code = http.POST(data);
    String response = http.getString();
    Serial.println(response);
}

void run_select() {
    String serverPath = httpserver + "/putzplan";
    http.begin(client, serverPath.c_str());
    int code = http.GET();

    if(code>0) {
      String payload = http.getString();
      JSONVar json = JSON.parse(payload);
      if(JSON.typeof(json) == "undefined") {
        Serial.println("JSON parsing failed");
        return;
      }
      for(int i=0;i<5;i++) {
        JSONVar row = json[i];
        String name = JSON.stringify(row["Name"]);
        names[i] = name.substring(1, name.length()-1);
        String d = JSON.stringify(row["erledigt"]);
        done[i] = atoi(d.substring(1, d.length()-1).c_str());
      }
    } else {
      Serial.println("HTTP GET error");
    }
    http.end();
}

void run_done(int id) {
    String serverPath = httpserver + "/done";
    http.begin(client, serverPath);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String data = "id="+String(id);
    int code = http.POST(data);
    String response = http.getString();
    Serial.println(response);
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

unsigned short int getWeekOfYear()
  {
    timeClient.begin();
    timeClient.setTimeOffset(3600);
    unsigned short int ret = 0;
    char weekOfYear[2];
    time_t sin1970;
    struct tm *ts;

    timeClient.update();
    unsigned long epoch = timeClient.getEpochTime();

    sin1970 = (time_t) epoch;
    ts = localtime(&sin1970);

    if(strftime(weekOfYear,3,"%W",ts)<0) {
      Serial.println("Ausnahmefehler: Ungueltige Kalenderwoche.");
      exit(-1);
    }
    ret=atoi(weekOfYear);
    timeClient.end();
    
   return ret;
  }

void rotate() {   
    String last;     
    last = names[4];    
        
    for(int j = 4; j > 0; j--){       
      names[j] = names[j-1];
      run_update(names[j], j);
    }       
    names[0] = last;
    run_update(names[0], 0);
    for(int i=0;i<5;i++) {
      done[i] = false;
    }
}

void handle_root() {
    run_select();

    kw = getWeekOfYear();
    
    if ( server.hasArg("btn0") ) {
      if(btn[0] == "btn-danger") { 
        btn[0] = "btn-success";
        done[0] = true;
        run_done(0);
      }
    } else if ( server.hasArg("btn1") ) {
      if(btn[1] == "btn-danger") { 
        btn[1] = "btn-success";
        done[1] = true;
        run_done(1);
      }
    } else if ( server.hasArg("btn2") ) {
      if(btn[2] == "btn-danger") { 
        btn[2] = "btn-success";
        done[2] = true;
        run_done(2);
      }
    } else if ( server.hasArg("btn3") ) {
      if(btn[3] == "btn-danger") { 
        btn[3] = "btn-success";
        done[3] = true;
        run_done(3);
      }
    } else if ( server.hasArg("btn4") ) {
      if(btn[4] == "btn-danger") { 
        btn[4] = "btn-success";
        done[4] = true;
        run_done(4);
      }
    } else if ( server.hasArg("week") ) {
      kw++;
    }

  if (kw != lastkw) {
    rotate();
    for(int i=0;i<5;i++) {
      Serial.println(names[i]);
    }
    Serial.println("-----------");
    lastkw = kw;
  }

  for(int i=0;i<5;i++) {
      if(done[i]) btn[i] = "btn-success";
      else btn[i] = "btn-danger";
    }
    
  server.send(200, "text/html", SendHTML());
}

String SendHTML(){
  String page = "<!DOCTYPE html> <html>\n";
  page +="<head>\n";
  page +="<title>Putzplan</title>\n";
  page +="<link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css' />";
  page +="<script src='https://cdnjs.cloudflare.com/ajax/libs/jquery/3.6.0/jquery.js' integrity='sha512-n/4gHW3atM3QqRcbCn6ewmpxcLAHGaDjpEBu4xZd47N0W2oQ+6q7oc3PXstrJYXcbNU1OHdQ1T7pAP+gi5Yu8g==' crossorigin='anonymous' referrerpolicy='no-referrer'></script>";
  page +="<script src='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js'></script>";
  page +="<meta name='viewport' content='width=device-width, initial-scale=1'>";
  page +="</head>\n";
  page +="<body>\n";
  page +="<div class='container-fluid'>\n";
  page +="  <div class='row'>\n";
  page +="   <div class='col-md-12'>\n";
  page +="    <h1>Putzplan</h1>";
  page +="    <h3>Kalenderwoche ";
  page +=String(kw);
  page +="    </h3>";
  page +="     <table class='table table-bordered table-hover table-sm'>\n";
  page +="       <thead>\n";
  page +="         <tr>\n";
  page +="           <th>\n";
  page +="             K&uuml;che\n";
  page +="           </th>\n";
  page +="           <th>\n";
  page +="             Bad oben\n";
  page +="           </th>\n";
  page +="           <th>\n";
  page +="             G&auml;stebad, Flur oben\n";
  page +="           </th>\n";
  page +="           <th>\n";
  page +="             Bad unten\n";
  page +="           </th>\n";
  page +="           <th>\n";
  page +="             Treppe, Flur unten, W&auml;sche\n";
  page +="           </th>\n";
  page +="         </tr>\n";
  page +="       </thead>\n";
  page +="       <tbody>\n";
  page +="         <tr>\n";
  page +="           <td>\n";
  page +="           <form action='/' method='POST'>\n";
  page +="             <button type='button submit' value='1' class='btn ";
  page += btn[0];
  page +=                                                         " btn-lg' name='btn0'>";
  page += names[0];
  page +="</button>\n";
  page +="           </form>\n";
  page +="           </td>\n";
  page +="           <td>\n";
  page +="           <form action='/' method='POST'>\n";
  page +="             <button type='button submit' value='1' class='btn ";
  page += btn[1];
  page +=                                                         " btn-lg' name='btn1'>";
  page += names[1];
  page +="</button>\n";
  page +="           </form>\n";
  page +="           </td>\n";
  page +="           <td>\n";
  page +="           <form action='/' method='POST'>\n";
  page +="             <button type='button submit' value='1' class='btn ";
  page += btn[2];
  page +=                                                         " btn-lg' name='btn2'>";
  page += names[2];
  page +="</button>\n";
  page +="           </form>\n";
  page +="           </td>\n";
  page +="           <td>\n";
  page +="           <form action='/' method='POST'>\n";
  page +="             <button type='button submit' value='1' class='btn ";
  page += btn[3];
  page +=                                                         " btn-lg' name='btn3'>";
  page += names[3];
  page +="</button>\n";
  page +="           </form>\n";
  page +="           </td>\n";
  page +="           <td>\n";
  page +="           <form action='/' method='POST'>\n";
  page +="             <button type='button submit' value='1' class='btn ";
  page += btn[4];
  page +=                                                         " btn-lg' name='btn4'>";
  page += names[4];
  page +="</button>\n";
  page +="           </form>\n";
  page +="           </td>\n";
  page +="         </tr>\n";
  page +="       </tbody>\n";
  page +="     </table>\n";
  //page +="<form action='/' method='POST'>\n<button type='button submit' value='1' class='btn btn-lg' name='week'>Woche+1</button></form>\n";
  page +="   </div>\n";
  page +=" </div>\n";
  page +="</div>\n";
  page +="</body>\n";
  page +="</html>\n";
  return page;
}
