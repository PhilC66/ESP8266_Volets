/* Commande Volets Roulants V3 PhC 21/03/2019

  On lit les commandes des BP telecommandes par interrupt
  On recoit les commandes de la page Web et actionne en parallele sur les BP
  on envoie les actions sur base BDD a intervalle regulier,
  en dehors des moments d'action
  
  to do
  passer en mqtt à la place http ?

  02/11/2023
  Wifimanager timeout et retry
  ICACHE_RAM_ATTR necessaire avec version ESP8266 2.5.2
  ARDUINO IDE 1.8.19, ESP8266 2.5.2
  Le croquis utilise 409360 octets (39%)
  Les variables globales utilisent 36708 octets (44%)

  31/10/2023
  Ajouté Wifimanager
  ICACHE_RAM_ATTR necessaire avec version ESP8266 2.5.2
  ARDUINO IDE 1.8.19, ESP8266 2.5.2
  Le croquis utilise 376104 octets (36%)
  Les variables globales utilisent 39592 octets (48%)

	06/06/2020 ESP8266 2.5.0
  Arduino IDE 1.8.10
	Compilation ESP8266 (> 2.5.0 ne fonctionne pas pb ISR ou OTA),80MHz 4MHz (3M spiffs)
  356428 34%, 35784 43% 09/06/2020
  356412 34%, 37144 45% 08/08/2019
	
*/
#include <Arduino.h>
#include <credentials_home.h>     // informations de connexion Wifi
#include <ESP8266WiFi.h>          // Biblio Wifi
#include <ArduinoOTA.h>           // Upload Wifi
#include <RemoteDebug.h>          // Telnet debug
#include <ESP8266WebServer.h>     // Serveur Httpp
#include <WiFiManager.h>          // Helps with connecting to Wifi
#include <TimeAlarms.h>
#include "index.h"

ESP8266WebServer server(3200);    // On instancie un serveur qui ecoute sur le port 3200
WiFiClient client;


RemoteDebug debug;

#define LedRouge 0
#define LedBleue 2

#define OpMontEtg 13 //13 ne marche pas en interrupt sur un esp de test?
#define OpDescEtg 12
#define OpMontRdc 5
#define OpDescRdc 4

int data[100][2] = {0, 0}; // data pour la bdd
int nbrligne = 0;          // nbr de lignes data

volatile int Metg = 0;
volatile int Detg = 0;
volatile int Mrdc = 0;
volatile int Drdc = 0;

volatile unsigned long irq1 = 0;
volatile unsigned long irq2 = 0;
volatile unsigned long irq3 = 0;
volatile unsigned long irq4 = 0;

int rebond = 20;
AlarmId Svie;
// ICACHE_RAM_ATTR necessaire avec version ESP8266 2.5.2
ICACHE_RAM_ATTR void handleIRQ1() { // interrupt monte etage.
  if (millis() - irq1 > rebond) {
    Metg ++;
    irq1 = millis();
  }
}
ICACHE_RAM_ATTR void handleIRQ2() { // interrupt desc etage
  if (millis() - irq2 > rebond) {
    Detg ++;
    irq2 = millis();
  }
}
ICACHE_RAM_ATTR void handleIRQ3() { // interrupt monte rdc
  if (millis() - irq3 > rebond) {
    Mrdc ++;
    irq3 = millis();
  }
}
ICACHE_RAM_ATTR void handleIRQ4() { // interrupt desc rdc
  if (millis() - irq4 > rebond) {
    Drdc ++;
    irq4 = millis();
  }
}
void configModeCallback (WiFiManager *myWiFiManager);
//---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  // /* WiFiManager */
  WiFiManager wifiManager;
  /* Uncomment for testing wifi manager */
  // wifiManager.resetSettings();
  wifiManager.setAPCallback(configModeCallback);
	wifiManager.setConfigPortalTimeout(120); // sets timeout before AP,webserver loop ends and exits
  wifiManager.setConnectRetries(10);       // sets number of retries for autoconnect

  /* or use this for auto generated name ESP + ChipID */
  if (!wifiManager.autoConnect()) {
    Serial.println(F("failed to connect and hit timeout"));
    delay(1000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  /* Manual Wifi */
  // WiFi.begin(mySSID, myPASSWORD);
  // Serial.println("");
  // // on attend d'etre connecte au WiFi avant de continuer
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }
  // on affiche l'adresse IP qui nous a ete attribuee
  Serial.println("");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  ArduinoOTA.setHostname("ESP_Volets"); // on donne une petit nom a notre module
  ArduinoOTA.begin();                   // initialisation de l'OTA
  debug.begin("ESP_Volets");
  debug.setResetCmdEnabled(true);       // autorise reset par telnet

  // on definit les points d'entree (les URL a saisir dans le navigateur web) et on affiche un simple texte
  server.on ("/", handleClient);


  server.begin(); // on demarre le serveur web

  pinMode			(LedRouge, OUTPUT);
  pinMode			(LedBleue, OUTPUT);
  digitalWrite(LedRouge, HIGH);
  digitalWrite(LedBleue, HIGH);

  noInterrupts();
  setInterruptions(); // installation des interruptions entrÃ¯Â¿Â½e des BP

  Svie = Alarm.timerRepeat(5, SignalVie); // chaque 5s
  Alarm.enable(Svie);

}
//---------------------------------------------------------------------------
void loop() {
  
  if (Metg > 0) { // detection d'un appui sur bouton
    Serial.print("interrupt Metg :"), Serial.println(Metg);
    Metg = 0;
    AccumulData(9, 1);
  }
  if (Detg > 0) {
    Serial.print("interrupt Detg :"), Serial.println(Detg);
    Detg = 0;
    AccumulData(9, 0);
  }
  if (Mrdc > 0) {
    Serial.print("interrupt Mrdc :"), Serial.println(Mrdc);
    Mrdc = 0;
    AccumulData(1, 9);
  }
  if (Drdc > 0) {
    Serial.print("interrupt Drdc :"), Serial.println(Drdc);
    Drdc = 0;
    AccumulData(0, 9);
  }

  ArduinoOTA.handle();	// Telechargement en Wifi
  debug.handle();				// debug par Telnet
  server.handleClient();// pour que les requetes soient traitees en Http
  Alarm.delay(1);

}
//---------------------------------------------------------------------------
void setInterruptions() {

  pinMode			(OpMontEtg, INPUT_PULLUP);
  pinMode			(OpDescEtg, INPUT_PULLUP);
  pinMode			(OpMontRdc, INPUT_PULLUP);
  pinMode			(OpDescRdc, INPUT_PULLUP);

  noInterrupts();

// #define ISR(serviceRoutine) void serviceroutine() __attribute__((ICACHE_RAM_ATTR)); void serviceroutine() 

  attachInterrupt(digitalPinToInterrupt(OpMontEtg), handleIRQ1, FALLING);
  attachInterrupt(digitalPinToInterrupt(OpDescEtg), handleIRQ2, FALLING);
  attachInterrupt(digitalPinToInterrupt(OpMontRdc), handleIRQ3, FALLING);
  attachInterrupt(digitalPinToInterrupt(OpDescRdc), handleIRQ4, FALLING);
  attachInterrupt(digitalPinToInterrupt(OpDescRdc), handleIRQ4, FALLING);

  interrupts();
}

//---------------------------------------------------------------------------
void DrivePin(int PinOutput) {
  // commande en parallele de BP
  Serial.print("Pin :"), Serial.println(PinOutput);
  noInterrupts();
  detachInterrupt(digitalPinToInterrupt(PinOutput));

  pinMode     (PinOutput, OUTPUT);

  digitalWrite(LedRouge, LOW);
  digitalWrite(PinOutput, LOW);
  Alarm.delay(250);
  digitalWrite(PinOutput, HIGH);
  digitalWrite(LedRouge, HIGH);

  setInterruptions();
}
//---------------------------------------------------------------------------
void AccumulData(byte f, byte s) {
  // f 0/1/9, s 0/1/9

  data[nbrligne][0] = f;
  data[nbrligne][1] = s;
	debug.print("commande :"),debug.print(f),debug.print(","),debug.println(s);

  nbrligne ++;
Alarm.enable(Svie);
}
//---------------------------------------------------------------------------
void handleClient() {
  // debug.print("args = "),debug.println(server.args());
  // debug.print("argname = "),debug.println(server.argName(0));
  // debug.print("arg = "),debug.println(server.arg(0));

  if (server.hasArg("D0")) { // ouverture etage
    if (server.arg(0) == "M") {
      DrivePin(OpMontEtg);
      server.send ( 200, "text/html", getPage() );
      AccumulData(9, 1);
    }
  }
  else if (server.hasArg("D1")) { // fermeture etage
    if (server.arg(0) == "D") {
      DrivePin(OpDescEtg);
      server.send ( 200, "text/html", getPage() );
      AccumulData(9, 0);
    }
  }
  else if (server.hasArg("D2")) { // ouverture rdc
    if (server.arg(0) == "M") {
      DrivePin(OpMontRdc);
      server.send ( 200, "text/html", getPage() );
      AccumulData(1, 9);
    }
  }
  else if (server.hasArg("D3")) { // fermeture rdc
    if (server.arg(0) == "D") {
      DrivePin(OpDescRdc);
      server.send ( 200, "text/html", getPage() );
      AccumulData(0, 9);
    }
  }
  else {
    server.send ( 200, "text/html", getPage() );
  }
}
//---------------------------------------------------------------------------
boolean W_data() {
  // envoie data sur base de données serveur
  bool flagreturn = false;
  int 	 pos = 0;
  String Sdata = "";
  String tempo = "Host:";
  tempo += monSitelocal;

  for (int n = 0; n < nbrligne; n ++ ) {
    if (client.connect(monSitelocal, 80)) { // REPLACE WITH YOUR SERVER ADDRESS
      Sdata  = "rdc=";
      Sdata += String(data[n][0]);
      Sdata += "&etage=";
      Sdata += String(data[n][1]);
      client.println("POST /volets/volets.php HTTP/1.1");
      client.println (tempo);          // SERVER ADDRESS HERE TOO
      client.println("Content-Type: application/x-www-form-urlencoded");
      client.print("Content-Length: ");
      client.println(Sdata.length());
      client.println();
      client.print(Sdata);
      delay(10);
      // debug.print(F("length : ")), debug.println(Sdata.length());
      // debug.print(F("Envoye au serveur : ")), debug.println(Sdata);

      String req = client.readStringUntil('X');//'\r' ne renvoie pas tous les caracteres?
      // debug.print("reponse serveur volets : "),debug.println(req);
      pos = req.indexOf("WOK");
      if (pos > 1) {
        debug.print("Ecriture bdd OK : "), debug.println(Sdata);
        flagreturn = true;
      }
      else {
        debug.println("Ecriture bdd KO");

        flagreturn = false;
      }
    }
  }
  if (client.connected()) {
    client.stop();  // DISCONNECT FROM THE SERVER
  }
  return flagreturn;
}
//---------------------------------------------------------------------------
void SignalVie() {

  char time_str[15];
  const uint32_t millis_in_day = 1000 * 60 * 60 * 24;
  const uint32_t millis_in_hour = 1000 * 60 * 60;
  const uint32_t millis_in_minute = 1000 * 60;
  uint8_t days = millis() / (millis_in_day);
  uint8_t hours = (millis() - (days * millis_in_day)) / millis_in_hour;
  uint8_t minutes = (millis() - (days * millis_in_day) - (hours * millis_in_hour)) / millis_in_minute;
  uint8_t secondes = (millis() - (days * millis_in_day) - (hours * millis_in_hour) - (minutes * millis_in_minute)) / 1000;
  sprintf(time_str, "%02dj%02dh%02dm%02ds", days, hours, minutes, secondes);

  debug.print("Uptime :"), debug.println(time_str);
  Serial.print("Uptime :"), Serial.println(time_str);

  digitalWrite(LedBleue, 0);
  Alarm.delay(15);
  digitalWrite(LedBleue, 1);

  if (nbrligne > 0)	{
    // ecrire bdd
    W_data();
    for (int i = 0; i < nbrligne; i++) {
      Serial.printf("numligne : %d,%d,%d\n", i, data[i][0], data[i][1]);
    }
    nbrligne = 0;
  }

}
//---------------------------------------------------------------------------
String getPage() { //content='60'
  // String page = "<html lang='fr'><head><meta http-equiv='refresh'  name='viewport' content='width=device-width, initial-scale=1 user-scalable=no'/>";
  // page += "<link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css'><script src='https://ajax.googleapis.com/ajax/libs/jquery/3.1.1/jquery.min.js'></script><script src='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js'></script>";
  // page += "<title>Volets</title></head><body>";
  // page += "<div class='container-fluid'>";
  // page += "<div class='row'>";
  // page += "<div class='col-md-12'>";
  // page += "<h1 class ='text-center'>Volets Roulants</h1>";
  // page += "<div class='row'>";
  // page += "<div class='col-xs-2 '><h4 class ='text-left'>Etage</h4>";
  // page += "</div>";
  // page += "<div class='col-xs-5 '><form action='/' method='POST'><button type='button submit' name='D0' value='M' class='btn btn-success btn-lg'>Ouvrir</button></form></div>";
  // page += "<div class='col-xs-5 '><form action='/' method='POST'><button type='button submit' name='D1' value='D' class='btn btn-primary btn-lg'>Fermer</button></form></div>";
  // page += "<div class='col-xs-2 '><h4 class ='text-left'>Rdc</h4>";
  // page += "</div>";
  // page += "<div class='col-xs-5 '><form action='/' method='POST'><button type='button submit' name='D2' value='M' class='btn btn-success btn-lg'>Ouvrir</button></form></div>";
  // page += "<div class='col-xs-5 '><form action='/' method='POST'><button type='button submit' name='D3' value='D' class='btn btn-primary btn-lg'>Fermer</button></form></div>";
  // page += "<br><p></p>";
  // page += "</div></div></div></div>";
  // page += "</body></html>";
  String page = MAIN_page;
  return page;
}
//--------------------------------------------------------------------------------//
void configModeCallback (WiFiManager *myWiFiManager) {
  /* Called if WiFi has not been configured yet */
  Serial.println("Gestion Wifi"); // "Wifi Manager"
  Serial.println("Se connecter a "); // "Please connect to AP"
  Serial.println(myWiFiManager->getConfigPortalSSID());
  Serial.println("Acceder a la page de configuration"); // "To setup Wifi Configuration"
  Serial.println("192.168.4.1");
}
