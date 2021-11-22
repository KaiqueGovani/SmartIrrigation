/*******************************************************************************
O projeto tem como objetivo fundamental a criação de um protótipo para um sistema de 
irrigação inteligente, visando obter um menor gasto de água por meio de uma programação 
que controlará a dispersão de água de acordo com o tipo de planta a ser cultivada, 
de modo que o gasto de água seja o mínimo necessário para que o desenvolvimento da 
planta ainda ocorra da maneira esperada.
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
char ssid[] = "SUA_REDE_WIFI";
char pass[] = "SUA_SENHA_WIFI";

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

//Funcao que le o pino V1 a cada atualizacao de estado
BLYNK_WRITE(V1){
  
  unsigned long timeIn = millis();
  int pinValue = param.asInt(); //Le o valor do pino virtual
  digitalWrite(LED_BUILTIN, pinValue); //Aciona o LED da placa de acordo com o valor lido pelo pino virtual

  //Pega o dia da semana que foi acionado o botão
  setDay();
  Blynk.sendInternal("meta", "get", "Dia");
  delay(5000);
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
}

BLYNK_CONNECTED() {                  //When device is connected to server...
  Blynk.sendInternal("rtc", "sync"); //request current local time for device
  Blynk.sendInternal("meta", "get", "Dia");
}

BLYNK_WRITE(InternalPinRTC) {   //check the value of InternalPinRTC  
  tempo = param.asLong();      //store time in t variable
}

//Configuracao do codigo
void setup(){
  
  //Inicializacao do monitor serial
  Serial.begin(9600);

  //Configura o pino do LED interno da placa como saida
  pinMode(LED_BUILTIN, OUTPUT);

  //Inicializa a comunicacao serial do ESP8266
  EspSerial.begin(ESP8266_BAUD);
  delay(10);

  //Inicializacao da comunicacao e conexao do modulo ao aplicativo
  Blynk.begin(auth, wifi, ssid, pass);

}

//Repeticao do codigo
void loop(){
  Blynk.run(); //Mantem a conexao ativa com o aplicativo e processa comandos recebidos ou enviados
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
  
}
