/*******************************************************************************
* Irrigação Inteligente
* Irrigação baseada em paineis solares com conectividade com a núvem
* Utiliza a plataforma Blynk para armazenamento de dados e controle sem fio
*******************************************************************************/

//Declaracao dos parametros de conexao do aplicativo
//Alterar os codigos abaixo de acordo com o que foi gerado na plataforma
#define BLYNK_TEMPLATE_ID "TMPLVmhKPll6"
#define BLYNK_TEMPLATE_NAME "RoboCore Projeto Arduino Avançado"
#define BLYNK_AUTH_TOKEN "ixhLmYtSltb6wsRe3WRpOdjrfY-SQI4J"

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
char ssid[] = "Eu amo muito tudo isso";
char pass[] = "kapolove124";

//Criacao do objeto serial para comunicacao com o ESP8266
SoftwareSerial EspSerial(10, 11); // RX, TX

//Declaracao da variavel que armazena a velocidade de comunicacao do modulo
const int ESP8266_BAUD = 9600;

//Configuracao do objeto 'wifi' para usar a serial do ESP8266 para conexao
ESP8266 wifi(&EspSerial);

//Variavel Dia
int dia = 0;
long tempo;
long tz_offset = 2; //offset da TimeZone em São Paulo

//Configurações da bomba d'água
const float VAZAO = 9.92; // Vazão da bomba, obtida via experimento

//Configurações dos Pinos Virtuais Blynk
#define LEDVPin V1
#define RelayVPin V2
#define UmidityVPin V3
#define waterUsageVPin 50
#define lastDayActiveVPin 99

static unsigned long waterTimer = 0;
int waterFlag = 0;

#define wifiLED 12   //D12 pino para led de conexão com Blynk
static unsigned long wifiTimer = 0;
int wifiFlag = 0;

#define RelayPin 2
#define MOISTUREPIN A0 //A0 pino sensor de úmidade
static unsigned long moistureTimer = 0;
float sensorValue = 0; 

bool AUTO = false;



//Apenas para teste de funcionamento:
//Funcao que le o pino LEDVPin a cada atualizacao de estado
BLYNK_WRITE(LEDVPin){
  int pinValue = param.asInt(); //Le o valor do pino virtual
  AUTO = pinValue;
  Serial.println("AUTO: " + String(AUTO));
  digitalWrite(LED_BUILTIN, pinValue); //Aciona o LED da placa de acordo com o valor lido pelo pino virtual
}

unsigned long timeIn;
unsigned long timeOut;
BLYNK_WRITE(RelayVPin){ //Funcao que le o pino RelayVPin a cada atualizacao de estado
  int pinValue = param.asInt(); //Le o valor do pino virtual
  if (pinValue == 1){
    waterOn();
  }
  if (pinValue == 0){//Se o pino for desligado
    waterOff();
  }
}

BLYNK_WRITE(InternalPinUTC){ //Função que recebe os comandos do aplicativo
  String cmd = param[0].asStr();
  if (cmd == "time"){
    tempo = param[1].asLongLong();
  }
  else if (cmd == "tz"){
    tz_offset = param[1].asInt()/60;
  }
}

BLYNK_WRITE(InternalPinMETA){ //Função que recebe os comandos do aplicativo
  String cmd = param[0].asStr();
  if (cmd == "Dia"){ //Se o comando for para alterar o dia, muda o valor do pino que armazena o ultimo dia de operação
    String value = param[1].asStr();
    Serial.println(value);
    Blynk.virtualWrite(lastDayActiveVPin, value); //Envia o dia da semana para o aplicativo
  }
  if (cmd == "TimeSec"){ //Se o comando for para alterar o tempo de uso, calcula o volume de água usado apartir da vazão
    int value = param[1].asInt();
    //Serial.println(int(value));
    Serial.println(String(int(timeOut - timeIn)) + " segundos");
    Blynk.sendInternal("meta", "set", "TimeSec", value + (timeOut - timeIn)); //Envia o tempo de uso para o aplicativo
    Blynk.virtualWrite(waterUsageVPin, (int(value) + int(timeOut - timeIn)) * VAZAO); //Envia o volume de água usado para o aplicativo
  }
}

BLYNK_CONNECTED() {                  //When device is connected to server...
  Blynk.sendInternal("rtc", "sync"); //request current local time for device
  //Blynk.sendInternal("meta", "get", "Dia");
  Blynk.syncVirtual(LEDVPin);
}

BLYNK_WRITE(InternalPinRTC) {   //check the value of InternalPinRTC  
  tempo = param.asLong();      //store time in t variable
}

String dayofweek(time_t now, int tz_offset) { //Função que retorna o dia da semana
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

void setDay(){ //Função que envia o dia da semana para o aplicativo
  Blynk.sendInternal("rtc", "sync");
  delay(50);
  Blynk.sendInternal("meta", "set", "Dia", dayofweek(tempo, 0));
  delay(20);
}

void waterOn(){ //Função que liga a bomba d'água
  if (waterFlag == 0){ //Se o pino for ligado
    Blynk.virtualWrite(RelayVPin, 1); //Liga o pino virtual
    waterFlag = 1; //Marca que a bomba está ligada
    digitalWrite(RelayPin, 1); //Liga o relé
    waterTimer = millis(); //Marca o tempo de inicio da irrigação
    Blynk.logEvent("molhando"); //Envia um log para o aplicativo
    timeIn = millis()/1000;
    Serial.println(timeIn); //Pega o tempo em segundos que foi acionado o botão   
    setDay(); //Pega o dia da semana que foi acionado o botão
    Blynk.sendInternal("meta", "get", "Dia"); //Envia o dia da semana para o aplicativo
  }
}

void waterOff(){ //Função que desliga a bomba d'água
  if (waterFlag == 1){ //Se o pino for desligado
    Blynk.virtualWrite(RelayPin, 0); //Desliga o pino virtual
    waterFlag = 0; //Marca que a bomba está desligada
    digitalWrite(RelayPin, waterFlag); // Desliga o relé
    Blynk.logEvent("parou"); //Envia um log para o aplicativo
    timeOut = millis()/1000;
    delay(20);
    //Serial.println(timeOut);
    Blynk.sendInternal("meta", "get", "TimeSec"); //Envia o tempo de uso para o aplicativo
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

void checkMoisture(){ //Função que verifica a umidade do solo
  sensorValue = analogRead(MOISTUREPIN); //Lê o valor do sensor
  sensorValue = map(sensorValue, 0, 1024, 100, 0); //Converte o valor do sensor para porcentagem
  Blynk.virtualWrite(UmidityVPin, sensorValue); //Envia o valor do sensor para o aplicativo
  Serial.println(String(sensorValue) + "%"); //Imprime o valor do sensor no monitor serial
  if (AUTO && sensorValue <= 40){ 
    //Se o modo automático estiver ligado e a umidade do solo for menor que 40%
    waterOn(); //Liga a bomba
  } if (AUTO && sensorValue > 40 && waterFlag){ 
    //Se o modo automático estiver ligado e a umidade do solo for maior que 40% e a bomba estiver ligada
    waterOff(); //Desliga a bomba  
  }
}

//Configuracao do codigo
void setup(){
  
  //Inicializacao do monitor serial
  Serial.begin(9600);

  //Configura o pino do LED interno da placa como saida
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(wifiLED, OUTPUT);
  pinMode(RelayPin, OUTPUT);

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
