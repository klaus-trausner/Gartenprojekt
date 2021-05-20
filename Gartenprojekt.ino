#define _DISABLE_TLS_

#include <Preferences.h>
Preferences prefs;

#include <time.h>
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;
struct tm timeinfo;




#include <DHT.h> 
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#include <ThingerESP32.h>
#define USERNAME "klaustrausner"
#define DEVICE_ID "Gartenprojekt"
#define DEVICE_CREDENTIAL "YkL7S&Z&Lql$sq"

#define SSID "3HuiTube_2.4Ghz_7259"
#define SSID_PASSWORD "6433828754"

ThingerESP32 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

bool daten_sichern = false;

bool manuell = false;
bool pumpe = false;
bool restart = false;
int tuerZu =0;
#define tasterPin 18
int RelIn[] = {25,26,32,33};
int bodenfeuchte = 0;
int temperatur =0;
int luftfeuchtigkeit = 0;
int bodenPin = 34;
int intBodenfeuchte = 4100;
int intOeffnung = 3000;
int anzOeffnung = 0;

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

//const char* resource = "/trigger/Gartenprojekt_Eingaben/with/key/mI6bPCAB_YhdUddiVR5GYLaV3JSFh6NdccDjy3fuM4K";
//const char* server = "maker.ifttt.com";

void printLocalTime();
  

void auf_ab(int port, int auf){
  digitalWrite(port, LOW);
  delay(intOeffnung);
  digitalWrite(port,HIGH);
  if (auf == 1){
    anzOeffnung =anzOeffnung+1;
  }else{
    anzOeffnung =anzOeffnung-1;
  } 
  prefs.putInt("anzOeffnung", anzOeffnung);
}

void oeffnung(){
  
    if (temperatur < step_1){
      while (anzOeffnung>0){
        auf_ab(26,0);
      }  
    } else if (temperatur < step_2){
      if (anzOeffnung <1){
        auf_ab(25,1);
      }else if (anzOeffnung>1){
          auf_ab(26,0);      
      } 
    }else if (temperatur < step_3){
        if (anzOeffnung<2){
          auf_ab(25,1);
        } else if (anzOeffnung>2){
          auf_ab(26,0);
        }
      }else{
        if (anzOeffnung < 4){
          auf_ab(25,1);
        }
      }
  }

void manuelleOeffnung(){
    if (auf){  
      auf_ab(25,1);
      auf = false;
    }
    if (ab){
      auf_ab(26,0);
      ab=false;
    }
  }

void schaltuhr(){
    
    if (timer_int_1 > 0){
      if (stunden == timer_h_1 && minuten == timer_m_1 && !pumpe){
        digitalWrite(32,LOW);
        pumpe=true;
        timer_on=true;
      }
      if (stunden == timer_h_1+(timer_m_1+timer_int_1)/60 && (timer_m_1+timer_int_1)%60 <= minuten && pumpe){
        digitalWrite(32,HIGH);
        pumpe = false;
        timer_on=false;
      }   
    }

    if (timer_int_2 > 0){
      if (stunden == timer_h_2 && minuten == timer_m_2 && !pumpe){
        digitalWrite(32,LOW);
        pumpe = true;
        timer_on=true;
      }
      if (stunden == timer_h_2+(timer_m_2+timer_int_2)/60 && (timer_m_2+timer_int_2)%60 <= minuten && pumpe){
        digitalWrite(32,HIGH);
        pumpe = false;
        timer_on=false;
      }   
    }

    if (bodenfeuchte > intBodenfeuchte && !pumpe){
      digitalWrite(32,LOW);
      pumpe=true;
    }
    if (bodenfeuchte<=intBodenfeuchte && pumpe && !timer_on){
      digitalWrite(32,HIGH);
      pumpe=false;
    }
  }

void sicherung(){
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
}

void back(){
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
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  prefs.begin("garten", false);
  back();
  
  
  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();
  
  
  pinMode(23, OUTPUT);
  pinMode(tasterPin,INPUT);

  unsigned char i;
  for(i=0; i<4;i++)
  {
    pinMode(RelIn[i], OUTPUT);
    digitalWrite(RelIn[i], HIGH);
  }

  thing.add_wifi(SSID, SSID_PASSWORD);

  
  thing["led"] <<digitalPin(23);
  thing["Tuer_zu"] >> [](pson& out){
      out = tuerZu;
};
  thing["restart"] << inputValue(restart);
  thing["Bodenfeuchte-Messung"] << [](pson& in){
    in = analogRead(34);
    bodenfeuchte = in;
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
  
  thing["Anzahl_Öffnungen"] >> outputValue(anzOeffnung);
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
}



void loop() {
  thing.handle();

  if (millis() > 5*60*1000 + myTimer) {
    bodenfeuchte = analogRead(34);
    
    Serial.println(bodenfeuchte);
    printLocalTime();
    myTimer = millis();
  }
  luftfeuchtigkeit =dht.readHumidity();
  temperatur = dht.readTemperature();

  tuerZu=digitalRead(tasterPin);
  
  /*
  if (tuerZu == LOW){
    digitalWrite(23,HIGH);
    delay(500);
    digitalWrite(23,LOW);
  }
*/
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
    schaltuhr();

  
}

void printLocalTime()
{
  
  if(!getLocalTime(&timeinfo)){
    //Serial.println("Failed to obtain time");
    return;
  }
  //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  minuten = timeinfo.tm_min;
  stunden = timeinfo.tm_hour;
}
