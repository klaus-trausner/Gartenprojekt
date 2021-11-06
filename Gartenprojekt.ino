#define _DEBUG_
#define _DISABLE_TLS_
#define THINGER_SERIAL_DEBUG

#include <ESP32Ping.h>

#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);


#include <Preferences.h>
Preferences prefs;

#include <time.h>
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;
struct tm timeinfo;


#include <ThingerESP32.h>
#define USERNAME "klaustrausner"
#define DEVICE_ID "Gartenprojekt"
#define DEVICE_CREDENTIAL "YkL7S&Z&Lql$sq"

#define SSID "3HuiTube_2.4Ghz_7259_Ext"
#define SSID_PASSWORD "6433828754"

ThingerESP32 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

#define trigPin 21
#define echoPin 22

long duration;
int z = 0;
int distance;
int volumen;

bool daten_sichern = false;
int reStarts = 0;

bool manuell = false;
bool messung = false;
bool pumpe = false;
long pumpeStart =0;
int autoPumpeDauer = 10;
bool restart = false;

int tuerZu =0;
long myTaster = 0;
#define tasterPin 35

int RelIn[] = {25,26,32,33};
#define relayAb 25
#define relayAuf 26
#define relayPumpe 32
#define relayNull 33

int bodenfeuchte = 0;
int bodenfeuchteMessInterval =120;
long bodenfeuchteStart=0;
int bodenPin = 34;
int intBodenfeuchte = 4100;

float temperatur =0;
int luftfeuchtigkeit = 0;

int intOeffnung = 3000;
int anzOeffnung = 0;
int messInterval = 5;

long myTimer = 0;
int stunden = 0;
int minuten = 0;
int timer_h_1, timer_h_2, timer_m_1, timer_m_2, timer_int_1,timer_int_2;
bool timer_on = false;

int step_1 =18;
int step_2 =22;
int step_3 =25;

bool auf = false;
bool ab = false;


void printLocalTime();


void setup() {
  Serial.begin(115200);
  sensors.begin();
  prefs.begin("garten", false);
  back();
  
  if (reStarts>5){
    reStarts=0;
  }else{
    reStarts++;
  }
  prefs.putInt("reStarts", reStarts);

  thing.add_wifi(SSID, SSID_PASSWORD);  
  
  pinMode(23, OUTPUT);
  pinMode(tasterPin,INPUT);

  unsigned char i;
  for(i=0; i<4;i++)
  {
    pinMode(RelIn[i], OUTPUT);
    digitalWrite(RelIn[i], HIGH);
  }

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);


  manuelleMessung();
  bodenfeuchteMessung();
  Serial.println("manuelle Messung durchgeführt im Setup");

  things();
  thing.handle();
  delay(1000);

  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();

  // tür schließen beim 5. reStart
  if (reStarts == 5){
    manuell = true;
    for (i=0; i<4;i++) {
      auf_ab(relayAb,0);
    }
    manuell=false;
  }
}


void loop() {
  
  bool success = Ping.ping("www.google.com", 1);
  Serial.print("Ping: ");
  Serial.println(success);
  
  if (success) {
    thing.handle();
    Serial.println("thing.handle normal");
    z=0;
    printLocalTime();
  } else {
    z++;
    if (z%100 == 0){
      thing.handle();
      printLocalTime();
      Serial.println("thing.handle nach 100 loops");
      if (z==4000){
        ESP.restart();
      }
    }
  }
  
  
  if (millis() > messInterval*60*1000 + myTimer) {
    manuelleMessung();
   
    myTimer = millis();
  }

  if (millis() > bodenfeuchteMessInterval*60*1000 + bodenfeuchteStart) {
    bodenfeuchteMessung();
    bodenfeuchteStart = millis();
  }

  if (messung){
    manuelleMessung();
    bodenfeuchteMessung();
  }
  
  if (millis() > 10000 + myTaster){
    tuerZu=analogRead(tasterPin);
  }
  
  if (tuerZu ==4095){
    //anzOeffnung=0;
    Serial.println("Tuer zu gedrückt");
    myTaster=millis();
    
    tuerZu=0;
  }
   
  if (!manuell){
    oeffnung();
  }else{
    manuelleOeffnung();
  }
  if (restart) {
    ESP.restart();
  }
  if (daten_sichern){
    sicherung();
    daten_sichern=false;
  }


  if (distance < 125){
    schaltuhr(); 
    }
}

void things(){
  thing["led"] <<digitalPin(23);
  thing["Tuer_zu"] >> [](pson& out){
      out = tuerZu;
};

  thing["restart"] << inputValue(restart);
  thing["Messung"] << [](pson& in){
    if(in.is_empty()){
        in = messung;
    }
    else{
        messung = in;
    }
  };

  thing["auf"] << inputValue(auf);
  thing["ab"] << inputValue(ab);
  thing["Wasserpumpe"] << digitalPin(32);
  thing["Relay-4"] << digitalPin(33);

  thing["Sensoren"] >> [](pson& out){
      out["Bodenfeuchte"] = bodenfeuchte;
      out["Luftfeuchtigkeit"] = luftfeuchtigkeit;
      out["Temperatur"] = temperatur;
};
  thing["Zisterne"] >> [](pson& out){ 
    out["Distanz"] = distance; 
    out["Volumen"] = volumen;
    };
  
  thing["Anzahl_Öffnungen"] >> [](pson& out){ out = anzOeffnung; };
  thing["Pumpe"] >> [](pson& out){ out = pumpe; };
  thing["Hebezeit"] << [](pson& in){
    if(in.is_empty()){
        in = intOeffnung;
    }
    else{
        intOeffnung = in;
    }
  };
  thing["Messinterval"] << [](pson& in){
    if(in.is_empty()){
        in = messInterval;
    }
    else{
        messInterval = in;
    }
  };
  thing["Step - 1"] << [](pson& in){
    if(in.is_empty()){
        in = step_1;
    }
    else{
        step_1 = in;
    }
  };
  thing["Step - 2"] << [](pson& in){
    if(in.is_empty()){
        in = step_2;
    }
    else{
        step_2 = in;
    }
  };
  thing["Step - 3"] << [](pson& in){
    if(in.is_empty()){
        in = step_3;
    }
    else{
        step_3 = in;
    }
  };
  
  thing["Bodenfeuchte-Schwelle"] << [](pson& in){
    if(in.is_empty()){
        in = intBodenfeuchte;
    }
    else{
        intBodenfeuchte = in;
    }
  };
  thing["Manuelle Steuerung"] << inputValue(manuell);

  thing["Timer-1 Stunde"] << inputValue(timer_h_1);
  thing["Timer-2 Stunde"] << inputValue(timer_h_2);
  thing["Timer-1 Minute"] << inputValue(timer_m_1);
  thing["Timer-2 Minute"] << inputValue(timer_m_2);
  thing["Timer-1 Intervall"] << inputValue(timer_int_1);
  thing["Timer-2 Intervall"] << inputValue(timer_int_2);

  thing["Sicherung"] << [](pson& in){
    if(in.is_empty()){
        in = daten_sichern;
    }
    else{
        daten_sichern = in;
    }
  };
  Serial.println("Things im Setup hinzugefügt, Setup beendet!");
}

  
  //Öffnet und schließt die Gartenbeetabdeckung
void auf_ab(int port, int auf){
  Serial.println("auf_ab");
  digitalWrite(port, LOW);
  digitalWrite(relayNull,LOW);

  if (auf == 1){
    anzOeffnung =anzOeffnung+1;
    delay(intOeffnung);
  }else{
    anzOeffnung =anzOeffnung-1;
    delay(intOeffnung);
  } 
  
  digitalWrite(port,HIGH);
  digitalWrite(relayNull,HIGH);

  if (anzOeffnung < 0){
    anzOeffnung =0;
  }
  if (anzOeffnung>4){
    anzOeffnung=4;
  }
  
  prefs.putInt("anzOeffnung", anzOeffnung);
}

//Steuert die Öffnung der Gartenbeetabdeckung über den Temperatursensor
void oeffnung(){  
    if (temperatur < step_1){
      while (anzOeffnung>0){
        auf_ab(relayAb,0);
      }  
    } else if (temperatur < step_2){
      if (anzOeffnung <1){
        auf_ab(relayAuf,1);
      }else if (anzOeffnung>1){
          auf_ab(relayAb,0);      
      } 
    }else if (temperatur < step_3){
        if (anzOeffnung<2){
          auf_ab(relayAuf,1);
        } else if (anzOeffnung>2){
          auf_ab(relayAb,0);
        }
      }else{
        if (anzOeffnung < 4){
          auf_ab(relayAuf,1);
        }
      }
  }

//Führt die manuelle Öffnung über Thinger.io aus
void manuelleOeffnung(){
    if (auf){  
      auf_ab(relayAuf,1);
      auf = false;
    }
    if (ab){
      auf_ab(relayAb,0);
      ab=false;
    }
  }

//Steuert die Zeituhr für die Beregnung und auch die Steuerung der Beregnung über einen Bodenfeuchtesensor
void schaltuhr(){    
    if (timer_int_1 > 0){
      if (stunden == timer_h_1 && minuten == timer_m_1 && !pumpe){
        digitalWrite(relayPumpe,LOW);
        pumpe=true;
        timer_on=true;
        thing.stream(thing["Pumpe"]);
      }
      if (stunden == timer_h_1+(timer_m_1+timer_int_1)/60 && (timer_m_1+timer_int_1)%60 <= minuten && pumpe){
        digitalWrite(relayPumpe,HIGH);
        pumpe = false;
        timer_on=false;
        thing.stream(thing["Pumpe"]);
      }   
    }

    if (timer_int_2 > 0){
      if (stunden == timer_h_2 && minuten == timer_m_2 && !pumpe){
        digitalWrite(relayPumpe,LOW);
        pumpe = true;
        timer_on=true;
        thing.stream(thing["Pumpe"]);
      }
      if (stunden == timer_h_2+(timer_m_2+timer_int_2)/60 && (timer_m_2+timer_int_2)%60 <= minuten && pumpe){
        digitalWrite(relayPumpe,HIGH);
        pumpe = false;
        timer_on=false;
        thing.stream(thing["Pumpe"]);
      }   
    }

    if (bodenfeuchte > intBodenfeuchte && !pumpe){
      digitalWrite(relayPumpe,LOW);
      pumpe=true;
      pumpeStart=millis();
      thing.stream(thing["Pumpe"]);
    }
    if (bodenfeuchte<=intBodenfeuchte && pumpe && !timer_on){
      digitalWrite(relayPumpe,HIGH);
      pumpe=false;
      thing.stream(thing["Pumpe"]);
    }

    if (millis() > (pumpeStart + autoPumpeDauer*60*1000)&&pumpe&&!timer_on){
      digitalWrite(relayPumpe,HIGH);
      pumpe=false;
      bodenfeuchte=0;
      thing.stream(thing["Pumpe"]);
    }
  }

void manuelleMessung(){
  
  Serial.println("Manuele Messung aufgerufen");
    digitalWrite(23, HIGH);
    Serial.println("LED leuchtet");
    digitalWrite(trigPin, LOW);
    Serial.println("TrigPin LOW");
    
    sensors.requestTemperatures();
    if (abs(sensors.getTempCByIndex(0)-temperatur)>1){
      temperatur = sensors.getTempCByIndex(0);
    }
    
    Serial.println("Temperatur gemessen: ");
    Serial.println(temperatur);

    delay(5);
    digitalWrite(trigPin, HIGH);
    delay(10);
    digitalWrite(trigPin, LOW);
    duration = pulseIn(echoPin, HIGH);
    distance = duration*0.034/2;
    //if (distance<50){
    //  distance =50;
    //  }
    
    volumen = 100*3.14*(14-distance/10);
    Serial.println("Distanz = ");
    Serial.println(distance);

    messung = false;
    delay(200);
    digitalWrite(23,LOW);
    Serial.println("LED ausgeschaltet");
}

void bodenfeuchteMessung(){
  bodenfeuchte = analogRead(34);
}

//Sichert die Daten auf dem ESP32
void sicherung(){
  Serial.println("sicherung");
  prefs.putInt("intBodenfeuchte", intBodenfeuchte);
  prefs.putInt("timer_h_1", timer_h_1);
  prefs.putInt("timer_h_2", timer_h_2);
  prefs.putInt("timer_m_1", timer_m_1);
  prefs.putInt("timer_m_2", timer_m_2);
  prefs.putInt("timer_int_1", timer_int_1);
  prefs.putInt("timer_hint_2", timer_int_2);
  prefs.putInt("step_1", step_1);
  prefs.putInt("step_2", step_2);
  prefs.putInt("step_3", step_3);
  prefs.putInt("anzOeffnung", anzOeffnung);
  prefs.putInt("intOeffnung", intOeffnung);
  prefs.putInt("Messinterval", messInterval);
}

//Holt die gesicherten Daten vom ESP32
void back(){
  Serial.println("back");
  intBodenfeuchte = prefs.getInt("intBodenfeuchte", 0);
  timer_h_1 = prefs.getInt("timer_h_1", 0);
  timer_h_2 = prefs.getInt("timer_h_2", 0);
  timer_m_1 = prefs.getInt("timer_m_1", 0);
  timer_m_2 = prefs.getInt("timer_m_2", 0);
  timer_int_1 = prefs.getInt("timer_int_1", 0);
  timer_int_2 = prefs.getInt("timer_hint_2", 0);
  step_1 = prefs.getInt("step_1", 0);
  step_2 = prefs.getInt("step_2", 0);
  step_3 = prefs.getInt("step_3", 0);
  anzOeffnung = prefs.getInt("anzOeffnung",0);
  intOeffnung = prefs.getInt("intOeffnung",0);
  messInterval = prefs.getInt("Messinterval",0);
  reStarts = prefs.getInt("reStarts",0);
}


void printLocalTime(){
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  minuten = timeinfo.tm_min;
  stunden = timeinfo.tm_hour;
}
