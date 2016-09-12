// Written by Régis Martiny

#include "DHT.h"
#include "Adafruit_BMP085.h"
#include "Thread.h"
#include <ArduinoSTL.h>
#include <SoftwareSerial.h>


//sensor luminosidade (LDR)
#define LDRPIN 13
//sensor temp e hum
#define DHTPIN 2     // what digital pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
#define DEBUG true
#define USB false
const String WIFI_SSID = "wFeliz";
const String WIFI_PASS = "ifrsfeliz";
const String SERVER = "192.168.103.227";
const String URI = "/meteorologia-api/v1/index.php/submitreading";
const String AUTH = "12345678901234567890";

//modulo wifi
//TX pino 11, RX pino 11
SoftwareSerial esp(11, 12);
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP085 bmp180;
Thread printThread, sensor1Thread, sensor2Thread, sensor3Thread;

//sensor class//////////////////////////////////////////////////////////////////////
class Sensor{
    public:
        String name;
        int nProbes;
        int* readerId;
        String* probe;
        String* value;
        String* magnitude;
        Sensor(int probes){
          nProbes = probes;
          readerId = new int[probes];
          probe = new String[probes]; 
          value = new String[probes];
          magnitude = new String[probes];
        };
};
//////////////////////////////////////////////////////////////////////////////////
std::vector<Sensor> sensors;

String data;


void resetWifi(){
  Serial.println("Resetting WIFI Module...");
  esp.println("AT+RST");
  delay(1000);
  if(esp.find("OK")) {
    Serial.println("Module Reset");
  }
}

void connectWifi() {
  Serial.println("Connecting...");
  //String cmd = "AT+CWJAP=\"" + WIFI_SSID + "\",\"" + WIFI_PASS + "\"";
  //esp.println(cmd);
  sendData("AT+CWJAP=\"" + WIFI_SSID + "\",\"" + WIFI_PASS + "\"\r\n", 10000, DEBUG);
  
  delay(4000);
  if(esp.find("OK")) {
    Serial.println("Connected!");
    // Mostra o endereco IP
    sendData("AT+CIFSR\r\n", 1000, DEBUG);
    // Configura para multiplas conexoes
    //sendData("AT+CIPMUX=1\r\n", 1000, DEBUG);
  }
  else {
    connectWifi();
  }
}


void postData() {
  data = getJSON();
  if (USB) Serial.println(data);
  else{
    httpPost();
    delay(1000);
  }
}


// This method makes a HTTP connection to the server and POSTs data
void httpPost() {
  Serial.println("Posting data..."); 
  esp.println("AT+CIPSTART=\"TCP\",\"" + SERVER + "\",80");//start a TCP connection.
  if( esp.find("OK")) {
    Serial.println("TCP connection ready");
  }
  delay(1000);
  String postRequest =
    "POST " + URI + " HTTP/1.1\r\n" +
    "Authorization: " + AUTH + "\r\n" +
    "Content-Length: " + (data.length() + 6) + "\r\n" +
    "Connection: Close\r\n" +
    "Content-Type: application/x-www-form-urlencoded\r\n" +
    "Host: " + SERVER + "\r\n" + "\r\n" + "data=" + data;

  Serial.println(postRequest);

  String sendCmd = "AT+CIPSEND=";//determine the number of caracters to be sent.
  //esp.print(sendCmd);
  sendData(sendCmd + postRequest.length() + "\r\n", 500, DEBUG);
  //esp.println(postRequest.length());
  delay(500);

  //if(esp.find(">")) { 
    Serial.println("Sending.."); 
    //esp.print(postRequest);
    sendData(postRequest, 500, DEBUG);
  //}
  if(esp.find("SEND OK")) { 
    Serial.println("Packet sent");
  }
  while (esp.available()) {
    String tmpResp = esp.readString();
    Serial.println(tmpResp);
  }
  // close the connection
  esp.println("AT+CIPCLOSE");
}


String sendData(String command, const int timeout, boolean debug)
{
  // Envio dos comandos AT para o modulo
  String response = "";
  esp.print(command);
  long int time = millis();
  while ( (time + timeout) > millis())
  {
    while (esp.available())
    {
      // The esp has data so display its output to the serial window
      char c = esp.read(); // read the next character.
      response += c;
    }
  }
  if (debug)
  {
    Serial.print(response);
  }
  return response;
}


String getJSON(){
  String json;
  if (USB)
    json = "[";
  else
    json = "%5B";
  for(int i=0; i < sensors.size(); i++){
    for (int j=0; j < sensors[i].nProbes; j++){
      if (!(i==0 && j==0)){
        if (USB)
          json = json + ",";
        else
          json = json + "%2C";
      }
      if(USB)
        json = json + "{\"reader_id\":" + sensors[i].readerId[j] + ", \"value\":" + sensors[i].value[j] + "}";
      else
        json = json + "%7B%22reader_id%22%3A" + sensors[i].readerId[j] + "%2C+%22value%22%3A" + sensors[i].value[j] + "%7D";
    }
  }
  if (USB)
    json += "]";
  else
    json += "%5D";
  return json;
}
  
void printvalues(){
  Serial.println("Sensores cadastrados: " + String(sensors.size()));
  for (int i=0; i < sensors.size(); i++){
      Serial.print("Sensor " + String(i) + ": ");
      Serial.println(sensors[i].name);
      for (int j=0; j < sensors[i].nProbes; j++){
        Serial.print("\t");
        Serial.print(sensors[i].probe[j]);
        Serial.print(": ");
        Serial.println(sensors[i].value[j]);
      }
  }
}

void temphum(){
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  sensors[0].readerId[0] = 1;
  sensors[0].value[0] = String(t);
  sensors[0].magnitude[0] = "C";
  sensors[0].readerId[1] = 2;
  sensors[0].value[1] = String(h);
  sensors[0].magnitude[1] = "%";
  
  //sensors[0].value[2] = String(hic);
  //sensors[0].magnitude[2] = "C";
}

void pressalt(){
  //sensors[1].readerId[0] = 4;
  //sensors[1].value[0] = String(bmp180.readTemperature());
  //sensors[1].magnitude[0] = "C";
  sensors[1].readerId[0] = 3;
  sensors[1].value[0] = String(bmp180.readPressure());
  sensors[1].magnitude[0] = "Pa";
  //sensors[1].value[2] = String(bmp180.readAltitude());
  //sensors[1].magnitude[2] = "m";
}

void luminosidade(){
  int val = analogRead(LDRPIN);
  //int lux = (2500/val - 500) / 3.3;
  sensors[2].readerId[0] = 4;
  sensors[2].value[0] = String(val);
  sensors[2].magnitude[0] = "lm";
}


void setup() {
  //Initiate serial port
  Serial.begin(9600);
  dht.begin();
  if (!bmp180.begin()) 
  {
    Serial.println("BMP180 Sensor not found!");
    while (1) {}
  }
  //Initialize wifi
  esp.begin(19200);
  
 if(!USB){
    resetWifi();
    connectWifi();
  }

  printThread = Thread();
  sensor1Thread = Thread();
  sensor2Thread = Thread();
  sensor3Thread = Thread();
  
  printThread.setInterval(300000); //5 min //300000
  sensor1Thread.setInterval(3000);
  sensor2Thread.setInterval(1000);//Fixed to 1000 
  sensor3Thread.setInterval(1000);//Fixed to 1000 

  Sensor sensor1(2);
  sensor1.name = "DHT22"; 
  sensor1.probe[0] = "Temperatura";
  sensor1.probe[1] = "Humidade";
  //sensor1.probe[2] = "Index de Calor";
  sensors.push_back(sensor1);
  Sensor sensor2(1);
  sensor2.name = "BMP180"; 
  //sensor2.probe[0] = "Temperatura";
  sensor2.probe[0] = "Pressao";
  //sensor2.probe[1] = "Altitude";
  sensors.push_back(sensor2);
  Sensor sensor3(1);
  sensor3.name = "LDR"; 
  sensor3.probe[0] = "Luminosidade";
  sensors.push_back(sensor3);
  
  printThread.onRun(postData);
  sensor1Thread.onRun(temphum);
  sensor2Thread.onRun(pressalt);
  sensor3Thread.onRun(luminosidade);

  temphum();
  pressalt();
  luminosidade();
  postData();
}

void loop() {
  if(printThread.shouldRun())
    printThread.run();
  if(sensor1Thread.shouldRun())
    sensor1Thread.run();
  if(sensor2Thread.shouldRun())
    sensor2Thread.run();
  if(sensor3Thread.shouldRun())
    sensor3Thread.run();
}


