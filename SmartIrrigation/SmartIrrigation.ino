/*******************************************************************************
* Kit Avançado para Arduino v4 - Hello Blynk
* Primeiros Passos com o aplicativo Blynk IoT.
* (Baseado no exemplo padrao do Blynk)
*******************************************************************************/

//Declaracao dos parametros de conexao do aplicativo
//Alterar os codigos abaixo de acordo com o que foi gerado na plataforma
#define BLYNK_TEMPLATE_ID "SEUS_DADOS"
#define BLYNK_DEVICE_NAME "SEUS_DADOS"
#define BLYNK_AUTH_TOKEN "SEUS_DADOS"

//Definicao do monitoramento de conexao da placa pela serial
#define BLYNK_PRINT Serial

//Adicao das bibliotecas
#include <ESP8266_Lib.h>
#include <BlynkSimpleShieldEsp8266.h>
#include <SoftwareSerial.h>
#include <Time.h>

//Declaracao da variavel que armazena o codigo de autenticacao para conexao
char auth[] = BLYNK_AUTH_TOKEN;

//Declaracao do nome e senha da rede Wi-Fi
//Altere as variaveis abaixo com o nome e senha da sua rede Wi-Fi
char ssid[] = "SUA_INTERNET"
char pass[] = "SUA_SENHA";

//Criacao do objeto serial para comunicacao com o ESP8266
SoftwareSerial EspSerial(10, 11); // RX, TX

//Declaracao da variavel que armazena a velocidade de comunicacao do modulo
const int ESP8266_BAUD = 9600;

//Confiracao do objeto 'wifi' para usar a serial do ESP8266 para conexao
ESP8266 wifi(&EspSerial);

//Variavel Dia
int dia = 0;
long tempo;
long tz_offset = 2; //offset da TimeZone em São Paulo

//Configurações da bomba d'água
const float VAZAO = 3.92;

static unsigned long waterTimer = 0;
int waterFlag = 0;

#define wifiLED 12   //D12
static unsigned long wifiTimer = 0;
int wifiFlag = 0;

#define MOISTUREPIN A0 
static unsigned long moistureTimer = 0;
float sensorValue = 0; 

bool AUTO = false;



//Apenas para teste de funcionamento:
//Funcao que le o pino V1 a cada atualizacao de estado
BLYNK_WRITE(V1){
  int pinValue = param.asInt(); //Le o valor do pino virtual
  AUTO = pinValue;
  Serial.println("AUTO: " + String(AUTO));
  digitalWrite(LED_BUILTIN, pinValue); //Aciona o LED da placa de acordo com o valor lido pelo pino virtual
}

unsigned long timeIn;
unsigned long timeOut;
BLYNK_WRITE(V2){
  int pinValue = param.asInt(); //Le o valor do pino virtual
  if (pinValue == 1){
    waterOn();
  }
  if (pinValue == 0){//Se o pino for desligado
    waterOff();
  }
}

BLYNK_WRITE(InternalPinUTC){
  String cmd = param[0].asStr();
  if (cmd == "time"){
    tempo = param[1].asLongLong();
  }
  else if (cmd == "tz"){
    tz_offset = param[1].asInt()/60;
  }
}

BLYNK_WRITE(InternalPinMETA){
  String cmd = param[0].asStr();
  if (cmd == "Dia"){
    String value = param[1].asStr();
    Serial.println(value);
    Blynk.virtualWrite(99, value);
  }
  if (cmd == "TimeSec"){
    int value = param[1].asInt();
    //Serial.println(int(value));
    Serial.println(String(int(timeOut - timeIn)) + " segundos");
    Blynk.sendInternal("meta", "set", "TimeSec", value + (timeOut - timeIn));
    Blynk.virtualWrite(50, (int(value) + int(timeOut - timeIn)) * VAZAO);
  }
}

BLYNK_CONNECTED() {                  //When device is connected to server...
  Blynk.sendInternal("rtc", "sync"); //request current local time for device
  //Blynk.sendInternal("meta", "get", "Dia");
  Blynk.syncVirtual(V1);
}

BLYNK_WRITE(InternalPinRTC) {   //check the value of InternalPinRTC  
  tempo = param.asLong();      //store time in t variable
}

String dayofweek(time_t now, int tz_offset) {
  // Calculate number of seconds since midnight 1 Jan 1970 local time
  time_t localtime = now + (tz_offset * 60 * 60);
  // Convert to number of days since 1 Jan 1970
  int days_since_epoch = localtime / 86400;
  // 1 Jan 1970 was a Thursday, so add 4 so Sunday is day 0, and mod 7
  int day_of_week = (days_since_epoch + 4) % 7; 

  int dia = day_of_week;
  String d;
  if (dia == 0)
    d = "Domingo";
  if (dia == 1)
    d = "Segunda";
  if (dia == 2)
    d = "Terça";
  if (dia == 3)
    d = "Quarta";
  if (dia == 4)
    d = "Quinta";
  if (dia == 5)
    d = "Sexta";
  if (dia == 6)
    d = "Sabado";

  return d;
}

void setDay(){
  Blynk.sendInternal("rtc", "sync");
  delay(50);
  Blynk.sendInternal("meta", "set", "Dia", dayofweek(tempo, 0));
  delay(20);
}

void waterOn(){
  if (waterFlag == 0){
    Blynk.virtualWrite(V2, 1);
    waterFlag = 1;
    digitalWrite(2, 1);
    waterTimer = millis();
    Blynk.logEvent("molhando");
    timeIn = millis()/1000;
    Serial.println(timeIn);
    //Pega o dia da semana que foi acionado o botão
    setDay();
    Blynk.sendInternal("meta", "get", "Dia");
    
  }
}

void waterOff(){
  if (waterFlag == 1){
    Blynk.virtualWrite(V2, 0);
    waterFlag = 0;
    digitalWrite(2, waterFlag);
    Blynk.virtualWrite(2, waterFlag);
    Blynk.logEvent("parou");
    timeOut = millis()/1000;
    delay(20);
    //Serial.println(timeOut);
    Blynk.sendInternal("meta", "get", "TimeSec");
    delay(20);
 
  }
  
}

void with_internet(){
}
void without_internet(){
}


void checkBlynkStatus() { // called every 6 seconds by SimpleTimer

  bool isconnected = Blynk.connected();
  if (isconnected == false) {
    wifiFlag = 1;
    digitalWrite(wifiLED, LOW);
  }
  if (isconnected == true) {
    wifiFlag = 0;
    digitalWrite(wifiLED, HIGH);
  }
}

void checkMoisture(){
  sensorValue = analogRead(MOISTUREPIN);
  sensorValue = map(sensorValue, 0, 1024, 100, 0);
  Blynk.virtualWrite(3, sensorValue);
  Serial.println(String(sensorValue) + "%");
  if (AUTO && sensorValue < 34){
    waterOn();
    
  } if (AUTO && sensorValue > 34 && waterFlag){
    waterOff();
    
  }
}

//Configuracao do codigo
void setup(){
  
  //Inicializacao do monitor serial
  Serial.begin(9600);

  //Configura o pino do LED interno da placa como saida
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(wifiLED, OUTPUT);
  pinMode(2, OUTPUT);

  //Inicializa a comunicacao serial do ESP8266
  EspSerial.begin(ESP8266_BAUD);
  delay(10);

  //Inicializacao da comunicacao e conexao do modulo ao aplicativo
  Blynk.begin(auth, wifi, ssid, pass);

  

}


//Repeticao do codigo
void loop(){
  Blynk.run(); //Mantem a conexao ativa com o aplicativo e processa comandos recebidos ou enviados
  if (waterFlag && millis()-waterTimer > 15000 && AUTO == 0){
    waterTimer = millis();
    waterOff();
  }
  if (millis()-wifiTimer > 10000){
    wifiTimer = millis();
    checkBlynkStatus();
  }
  if (millis()-moistureTimer > 10000){
    moistureTimer = millis();
    checkMoisture();
  }
  if (wifiFlag == 0)
    with_internet();
  else
    without_internet();
}
