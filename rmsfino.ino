/* 
 *  
 *  Código do projeto
 *  Sensores: DHT11, MQ135
 *  Atuadores: Buzzer, Bomba de água
 *  Ultima atualização: 1/06/2019 
 *  Autores: 
 *  Leandro Almeida Nº 84112
 *  Ruben Gomes  Nº 84180
 */
#include <dht.h>
#include "Akeru.h"

//Timers defined in miliseconds
#define TDOWN 30000//timer tDown - de downlink
#define TUP 20000//timer tUp - de uplink 
#define TUPFIRE 30000 //timer tUpFire - de uplink em caso de incêndio
#define DANGERTIME 120000 //timer to control danger of fire - 2 minutos - time between new temperature check
#define BUZZERTIME 10000 //
#define PUMPTIME 10000 //
#define PRINTTIME 5000//print every 5 seconds


unsigned long lastTdown = 0; // the time of the last downlink
unsigned long lastTup = 0; // the time of the last uplink
unsigned long lastTupFire = 0; // the time of the last uplink in fire
unsigned long dangerController = 0; // every 2 minutes we replace lastTemperature and we compare actualTemperature with lastTemperature to check wether  actual - last > 10ºC
unsigned long pumpTime = 0;
unsigned long alarmTime = 0;
unsigned long printTime = 0;
float lastTemperature = -1;

int air_value;
int dht_pin = 7; //digital - D7 
boolean fire_flag = false; //true if fire is active

int buzzer_pin = 3; //digital - D3
int pump_pin = 2; //digital - D2

//flags used in the case that the user decide to activate alarm or water pump
String bombState = "0"; //bomb active?
String alarmState ="0"; //alarm active?
String alarmEnable = "1"; //alarm enabled?


dht dht11;      


//SIGFOX
#define TX 4
#define RX 5
Akeru akeru(RX, TX); 

String sendData;
String receiveData=""; 
String fire = "0";

//Sensor values
String carbonValue ;
String temperatureValue ;
String humidityValue ;

//sensor thresholds
String carbonTh;
String temperatureTh = "35";
String humidityTh;

//construct Data in the needed format to Uplink Stuff!
String constructData(String _data) {
  while(1){
    if(_data.length() == 3)
      return _data;
        
   _data = "0" + _data;
  }
    
}

void turnOnAlarm()
{
  Serial.println("Turning on alarm");
  tone(buzzer_pin, 1000);
}

void turnOffAlarm()
{
  Serial.println("Turning off alarm");
  noTone(buzzer_pin);
}



void turnOnBomb()
{
  Serial.println("Turning on bomb");
  digitalWrite(pump_pin, HIGH);

}

void turnOffBomb()
{
   Serial.println("Turning off bomb");
   digitalWrite(pump_pin, LOW);
    
}

//Uplink
void uplinkStuff(){
  Serial.print("fire é ");
  Serial.println(fire);
  sendData = temperatureValue + humidityValue + carbonValue + fire + "00";
  Serial.print("Message to send in Uplink: ");
  Serial.println(sendData);

  //UPLINK 
  if(akeru.sendPayload(sendData))
    Serial.println("Message sent!");
       
}


//decode the information received in downlink stuff
void decodeReceiveData(String _data){
  carbonTh = "";
  temperatureTh = "";
  humidityTh = "";
  
  for(int i=1; i < 3; i++){
    temperatureTh = temperatureTh + _data[i];
    humidityTh = humidityTh + _data[i+2];
    carbonTh = carbonTh + _data[i+4];
  }
  alarmEnable = _data[7];
  alarmState = _data[8];
  bombState = _data[9];
  
  //if the user send information to turnOn alarm
  if(alarmState.equals("1")){
    Serial.println("User send information to turnOn Alarm");
    turnOnAlarm();
    alarmTime = millis();
  }
  //if the user send information to turnOn waterPump  
  if(bombState.equals("1")){
    Serial.println("User send information to turnOn waterPump");
    turnOnBomb();
    pumpTime = millis();
  }

}



//DOWNLINK
void downlinkStuff(){
      Serial.println("A iniciar downlink...");   
      if (akeru.receive(&receiveData))
      {
        Serial.println("---------- RECEIVED SOMETHING FROM DOWNLINK ----------");
        Serial.print("Data Received: ");
        Serial.println(receiveData);
      } 
      
      Serial.println("After Decoding Downlink Data");
      decodeReceiveData(receiveData);
      Serial.print("Temperature Threshold: ");
      Serial.println(temperatureTh);   
      Serial.print("Humidity Threshold: ");
      Serial.println(humidityTh);
      Serial.print("carbon Threshold: ");
      Serial.println(carbonTh);
      Serial.println("--------------------------------------------------------");
  
}

void uplinkFormat(){
  //put the sensor values in the needed format to send in Uplink 
  temperatureValue = String(int(dht11.temperature));
  temperatureValue = constructData(temperatureValue); 
  humidityValue = String(int(dht11.humidity));
  humidityValue = constructData(humidityValue);
  carbonValue = String(air_value);
  carbonValue = constructData(carbonValue);
}


void setup()
{
  Serial.begin(2000000);
  
  //Timers initialization
  lastTdown = millis();
  lastTup = millis();
  lastTupFire = millis();
  dangerController = millis();
  printTime = millis();

  pinMode(buzzer_pin, OUTPUT);
  pinMode(pump_pin, OUTPUT);

  //Pump turned off initially
  digitalWrite(pump_pin, LOW); //desliga o pump

  if(!akeru.begin())
  {
    Serial.println("Problema no Akeru");
  }

}
void loop()
{
  
  //--------------Reading sensor values-------------------------------------
  // MQ35
  air_value = analogRead(A0)/10;
  //DHT11
  dht11.read11(dht_pin);
  //------------------------------------------------------------------------

  //----------------Print actual state--------------------------------------- 
  if(millis() - printTime > PRINTTIME)
  {
    Serial.println();
    Serial.println(" ***** - Sensor Values - *****");
    Serial.print("Gas = ");
    Serial.print(air_value);
    Serial.println("%");
    Serial.print("Temperature: ");
    Serial.print(dht11.temperature);
    Serial.println("ºC");
    Serial.print("Relative humidity: ");
    Serial.print(dht11.humidity);
    Serial.println("%");    
    Serial.println("******************************");
    Serial.println();
    printTime = millis();
  }
  //-----------------------------------------------------------------------------

  //put the variables in uplink Format
  uplinkFormat();

  //If temperatureValue > temperatureThreshold -> there is danger & fire = '1'
  //if the actual temperature is greater than threshold, we consider fire!
  if(dht11.temperature > temperatureTh.toInt())
  {
    Serial.println("Danger is true1 - There is fire because the temperature is greater than threshold! ATTENTION!\n");
    fire = "1";
  }
  else if(lastTemperature != -1) //If lastTemperature is already initialized
  {
    //if difference between  actual and temperatures is greater than 10 && temperature > temperatureThreshold - 5, we considerer that there is fire
    if((dht11.temperature- lastTemperature  > 10) && dht11.temperature > temperatureTh.toInt() - 5)
    {
      Serial.println("Danger is true! There is fire because the temperature increase very fast\n");
      fire = "1";
      
    }
    else //there is not danger & fire = '0'
    {
      fire = "0";
    }
  }
 
    

  //actualize lastTemperature value if every 2 minutes (DANGERTIME)
  if((millis() - dangerController)>= DANGERTIME)
  {
     lastTemperature = dht11.temperature;
     dangerController = millis();
  }

  
  //Assess the danger of fire
  if(fire.equals("1"))
  {
    if(fire_flag == true)//(Fire already detected) 
    {
      //if end of tUpFire -> Send uplink message and reinitialize tUp e tUpFire
      if((millis() - lastTupFire)>= TUPFIRE)
      {
        uplinkStuff();
        //restart uplink Fire timer
        lastTupFire = millis();
      }
      else if((millis() - lastTdown)>= TDOWN)
      {
        downlinkStuff();
        lastTdown = millis(); 
      }
      
    }
    else{
        //alarm enabled ? --> turn on alarm & pump
        //turn on pump
        turnOnBomb();
        
        if(alarmEnable.equals("1"))
        {
          //turn on alarm
          turnOnAlarm();
        }
        fire_flag = 1; 
        uplinkStuff();  
        //Reinitialize tUp e tUpFire
        lastTupFire = millis();
        lastTup = millis();
    }
  }
  
  else{
    
      if (fire_flag == true){
        Serial.println("Fire extinguished!");
        fire_flag = false;
        turnOffBomb(); 
        turnOffAlarm(); 
      }
  
      if((millis() - lastTup)>= TUP)
      {
        //UPLINK      
        uplinkStuff();
        lastTupFire = millis();
        lastTup = millis();
      }
      else if((millis() - lastTdown)>= TDOWN)
      {
        downlinkStuff();
        lastTdown = millis(); 
      }

      //desligar bomba e/ou alarme caso o utilizador tenha mandado informação para o ligar, e caso não haja incêndio!
      if((millis()- pumpTime) > PUMPTIME && bombState.equals("1")){
        turnOffBomb();
        bombState = "0";
      }
      if((millis()- alarmTime) > BUZZERTIME && alarmState.equals("1")){
        turnOffAlarm();
        alarmState = "0";
      }
    
  }

  delay(2000);
 
}
