
/*==============================================================================================================
    Revox-A77-Fernbedienung-V1b-ota
    (c) Hans Borngräber
    18.Mai 2023
  ==============================================================================================================*/

#include <ESP8266WebServer.h>                                                             // Im ESP8266 Master enthalten
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LittleFS.h>
#include <ArduinoOTA.h>

#define Prg_Version "(c)H.Borngraeber V1.b-ota  18.Mai 2023"
#define H_Name "Revox-A77"
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);

const char WIFI_SSID[] = "???";                                                           // WLAN Name (SSID)      <-- muss angepasst werden auf eigene Werte
const char WIFI_PASSWORD[] = "$$$$$$$$$$$$";                                              // WLAN Passwort         <-- muss angepasst werden auf eigene Werte

int FRV_REL = 14;                                                                         // Port D5 Schneller Rücklauf
int FFW_REL = 16;                                                                         // Port D0 Schneller Vorlauf
int PLAY_REL = 12;                                                                        // Port D6 Play
int STOP_REL = 13;                                                                        // PORT D7 Stop
int REC_REL = 0;                                                                          // Port D3 Aufnahme
int adc = A0;                                                                             // Port A0 Analog Eingang
int pos;                                                                                  // String Analyse
int Webdaten_laenge;                                                                      // Länge des eingelesenen Datensatzes
int Bremszeit = 3000;

String Aktion;
String Bilddatei = "revox-a77-skizze-75.b64";
char Bild1 [34611];

boolean Fastrueckstop = false;
boolean Fastvorstop = false;
boolean Playstop = 0;
WiFiServer server(80);                                                                    // Webserver Portnummer auf 80 setzen

//==============================================================================================================
void setup() {
  Serial.begin(74880);                                                                   // Serielle Schnittstelle aktivieren

  //-------------------------------------------------------------------------
  pinMode (FRV_REL, OUTPUT);
  digitalWrite (FRV_REL, LOW);
  pinMode (FFW_REL, OUTPUT);
  digitalWrite (FFW_REL, LOW);
  pinMode (PLAY_REL, OUTPUT);
  digitalWrite (PLAY_REL, LOW);
  pinMode (STOP_REL, OUTPUT);
  digitalWrite (STOP_REL, LOW);
  pinMode (REC_REL, OUTPUT);
  digitalWrite (REC_REL, LOW);

  //-------------------------------------------------------------------------
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);                                              // Dispay initialisieren
  Display_loeschen0();

  //-------------------------------------------------------------------------

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);                                               // Anmelde Paramter Wifi
  WiFi.mode(WIFI_STA);
  while (WiFi.status() != WL_CONNECTED) {
    delay (300);
    display.print(".");
    display.display();
  }
  WiFi.hostname(H_Name);
  //-------------------------------------------------------------------------
  LittleFS.begin();                                                                       // Filesystem initialisieren
  if (!LittleFS.begin()) {
    Serial.println (F("Fehler beim starten des Filesystemes!!"));
  }
  else {
    Serial.println (F("Datei System gestartet."));
  }
  //-------------------------------------------------------------------------
  Display_loeschen0();                                                                    // Programmversion ausgeben
  display.print(Prg_Version);
  display.display();
  delay (1000);

  Display_loeschen0();                                                                   // Verbindungsdaten ausgeben
  display.setCursor(28, 0);
  display.print("Verbunden");
  display.setCursor(0, 8);
  display.print("IP  : ");
  display.print(WiFi.localIP());
  display.setCursor(0, 16);
  display.print("SSID: ");
  display.print(WiFi.SSID());
  display.setCursor(0, 24);
  display.print("Name: ");
  display.print(WiFi.hostname());
  display.display();

  delay (2000);
  Stop ();
  Bild_lesen();

  //---------------------------------------------------
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else
      type = "filesystem";
    Serial.println ("Start OTA-updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println (F("\nOTA-End"));
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println (F("OTA-Ready"));

  //-------------------------------------------------------------------------
  server.begin();                                                                         // HTTP Server starten
}

//==============================================================================================================
void Bild_lesen () {
  File Datei1 = LittleFS.open( Bilddatei, "r");
  if (!Datei1) {
    Serial.println (F("-- Fehler beim öffnen der Datei: "));
    Serial.println (Bilddatei);
  }
  else {
    while (Datei1.available()) {
      int max_len = Datei1.available();
      int read_len = Datei1.readBytesUntil('\n', Bild1, max_len);
    }
    Datei1.close();
  }
}

//==============================================================================================================
void Display_loeschen0() {
  display.clearDisplay();
  display.setTextSize(0);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.display();
}

//==============================================================================================================
void Display_loeschen4() {
  display.clearDisplay();
  display.setTextSize(4);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.display();
}

//==============================================================================================================
void Display_loeschen2() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.display();
}

//==============================================================================================================
void Ruecklauf () {
  Fastrueckstop = true;
  if (Fastvorstop) {
    Bremsen();
  }
  digitalWrite (REC_REL, LOW);
  digitalWrite (STOP_REL, LOW);
  digitalWrite (FFW_REL, LOW);
  digitalWrite (PLAY_REL, LOW);
  digitalWrite (FRV_REL, HIGH);

  Display_loeschen2();
  display.setCursor(0, 8);
  display.print("<< Rueck");
  display.display();
  display.startscrollleft   (0x0, 0x7);
}

//==============================================================================================================
void Vorlauf () {
  Fastvorstop = true;
  if (Fastrueckstop) {
    Bremsen();
  }
  digitalWrite (REC_REL, LOW);
  digitalWrite (STOP_REL, LOW);
  digitalWrite (FFW_REL, HIGH);
  digitalWrite (PLAY_REL, LOW);
  digitalWrite (FRV_REL, LOW);

  Display_loeschen2();
  display.setCursor(0, 8);
  display.print(">> Vor");
  display.display();
  display.startscrollright   (0x0, 0x7);
}

//==============================================================================================================
void Play () {
  Playstop = 1;
  if (Fastvorstop || Fastrueckstop) {
    Bremsen();
  }
  digitalWrite (REC_REL, LOW);
  digitalWrite (STOP_REL, LOW);
  digitalWrite (FFW_REL, LOW);
  digitalWrite (PLAY_REL, HIGH);
  digitalWrite (FRV_REL, LOW);
  Display_loeschen2();
  display.setCursor(16, 8);
  display.print("> Play");
  display.display();
  display.startscrollright   (0x0, 0x7);
}

//==============================================================================================================
void Stop () {
  digitalWrite (REC_REL, LOW);
  digitalWrite (STOP_REL, HIGH);
  digitalWrite (FFW_REL, LOW);
  digitalWrite (PLAY_REL, LOW);
  digitalWrite (FRV_REL, LOW);
  display.stopscroll();
  if (Playstop == 0) {
    Serial.println (Playstop);
    Bremsen();
  }
  Playstop = 0;
  Display_loeschen2();
  display.print ("A77 Bereit");
  display.setCursor(32, 16);
  display.print("Stop");
  display.display();
  digitalWrite (STOP_REL, LOW);
}

//==============================================================================================================
void Bremsen() {
  Display_loeschen2();
  display.setCursor(16, 0);
  display.println("Bremse");
  display.print("<");
  display.display();
  display.startscrollleft   (0x2, 0x3);
  delay (Bremszeit);
  Fastvorstop = false;
  Fastrueckstop = false;
  display.stopscroll();
}

//==============================================================================================================
void Record () {
  Playstop = 1;
  if (Fastvorstop || Fastrueckstop) {
    Bremsen();
  }
  Playstop = 0;
  digitalWrite (FRV_REL, LOW);
  digitalWrite (STOP_REL, LOW);
  digitalWrite (FFW_REL, LOW);
  digitalWrite (REC_REL, HIGH);
  delay (500);
  digitalWrite (PLAY_REL, HIGH);

  Display_loeschen2();
  display.setCursor(0, 8);
  display.print("> Record");
  display.display();
  display.startscrollright   (0x0, 0x7);
}
//==============================================================================================================
void loop() {
  ArduinoOTA.handle();
  WiFiClient client = server.available();
  
  //-------------------------------------------------------------------------
  if (client) {
    Aktion = client.readStringUntil('\n');                                  // Webseiteneingaben einlesen bis Zeilenende erreicht ist \n
    Serial.println (F ("Webseitendaten: "));
    Serial.println (Aktion);
    Webdaten_laenge = Aktion.length();
    if (Webdaten_laenge > 5) {
      pos = Aktion.indexOf("GET / HTTP/1.1");
      if (pos == -1 ) {
        pos = Aktion.indexOf("GET /favicon.ico HTTP/1.1");
        if (pos == -1 ) {
          pos = Aktion.indexOf("GET /A77/FRV HTTP/1.1");
          if (pos > -1) {
            Ruecklauf();
          }
          pos = Aktion.indexOf("GET /A77/STOP HTTP/1.1");
          if (pos > -1) {
            Stop();
          }
          pos = Aktion.indexOf("GET /A77/FFW HTTP/1.1");
          if (pos > -1) {
            Vorlauf();
          }
          pos = Aktion.indexOf("GET /A77/PLAY HTTP/1.1");
          if (pos > -1) {
            Play();
          }
          pos = Aktion.indexOf("GET /A77/RECORD HTTP/1.1");
          if (pos > -1) {
            Record();
          }
        }
      }
    }
    //------------------------------------------------------------------------- Antwort auf den GET-Befehl
    client.println (F  ("HTTP/1.1 200 OK"));
    client.println (F  ("Content-type:text/html;  charset=UTF-8;"));
    client.println (F  ("Connection: close"));
    client.println   ();
    //------------------------------------------------------------------------- Webseite initialisieren
    client.println (F  ("<!DOCTYPE html><html>"));
    client.println (F  ("<head><meta text/html; http-equiv='refresh';>"));
    client.println (F  ("<head><meta name=\'viewport\' content=\'width=device-width, initial-scale=1.0\'>"));
    client.println (F  ("</head>"));

    client.println (F  ("<body style='color: rgb(0,0,0); background-color: rgb(0,0,0)'>"));
    client.println (F  ("<div style='text-align: center; font-family: Calibri ; color: black; font-weight: bold; '>"));
    client.println (F ("<H1><span style='background-color: grey; border: 15px solid grey; border-radius: 10px;'> "));
    client.println  (WiFi.hostname());
    client.println (F  ("</style></span></div></H1>"));
    client.println (F  ("<br><br><br><br>"));


    client.println (F  ("<div style='text-align: center;'>"));
    client.print    (Bild1);
    client.println  ();
    client.println (F  ("</style></div></img>"));

    client.println (F  ("<br><br><br><br>"));
    client.println (F  ("<div style='text-align: center; font-family: Calibri; color: rgb(0, 255, 255); font-weight: bold; '><br>"));
    client.println (F  ("<style>.button_green { background-color: green; border: none; color: black; padding: 16px 20px;-webkit-border-radius: 28;  -moz-border-radius: 28;  border-radius: 28px;text-decoration: none; font-size: 20px; margin: 2px; cursor: pointer;}</style>"));
    client.println (F  ("<style>.button_red { background-color: red; border: none; color: white; padding: 16px 20px;-webkit-border-radius: 28;  -moz-border-radius: 28;  border-radius: 28px;text-decoration: none; font-size: 20px; margin: 2px; cursor: pointer;}</style>"));
    client.println (F  ("<style>.button_yellow { background-color: yellow; border: none; color: black; padding: 16px 20px;-webkit-border-radius: 28;  -moz-border-radius: 28;  border-radius: 28px;text-decoration: none; font-size: 20px; margin: 2px; cursor: pointer;}</style>"));
    client.println (F  ("<style>.button_gray { background-color: gray; border: none; color: black; padding: 16px 20px;-webkit-border-radius: 28;  -moz-border-radius: 28;  border-radius: 28px;text-decoration: none; font-size: 20px; margin: 2px; cursor: pointer;}</style>"));
    client.println (F  ("<table style='width: auto; text-align: center; height: 30px;margin-left: auto; margin-right: auto;'border='0' cellpadding='4' cellspacing='4'>"));

    //------------------------------------------------------------------------- Schalt Buttons definieren
    client.println (F  ("<tbody><h3>"));
    client.println (F  ("<tr><td>"));
    client.println (F  ("<td> <a href='/A77/FRV' class='button_gray'>Rücklauf</a>"));
    client.println (F  ("<td> <a href='/A77/FFW' class='button_gray'>Vorlauf</a>"));
    client.println (F  ("<td> <a href='/A77/PLAY' class='button_green'>Play</a>"));
    client.println (F  ("<td> <a href='/A77/STOP' class='button_red'>Stop</a>"));
    client.println (F  ("<td> <a href='/A77/RECORD' class='button_red'>Record</a>"));
    client.println (F  ("</h3></tbody></table></style>"));

    client.println (F  ("<br><br><br><br>"));

    client.println (F  ("<div style='text-align: center; font-family:Calibri; color: white; font-weight: 100; '><br>"));
    client.println (Prg_Version);
    client.println (F  ("</style></div></body></html>"));
    client.println  ();
  }
}
