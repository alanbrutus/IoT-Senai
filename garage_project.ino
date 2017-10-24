#include <Servo.h>
#include <UIPEthernet.h>
#include <utility/logging.h>
#include <SPI.h>
#include <PubSubClient.h>

Servo myservo;

int pos = 0;
int speedOpen = 15;
int speedClose = 15;
boolean lockDoor = true;
boolean light = false;
int spotLight = 3;
int button = 8;
char actionDoor = 'F';
char beforeActionDoor = 'F';

int buttonState = 0;
int buttonStateBefore = 0;

char topicPub[] = "Garage_Project";
char topicSub[] = "Garage_Project";

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xAE, 0x03 };
IPAddress ip (192, 162, 1, 100);
char server[] = "m10.cloudmqtt.com";
int port = 12598;

//=======================================================================
void callback(char topic, byte payload, unsigned int length) {

  char* payloadAsChar = payload;
  payloadAsChar[length] = 0;
  String msg = String(payloadAsChar);
  int msgComoNumero = msg.toInt();
  
  Serial.print("Topic received: "); Serial.println(topic);
  Serial.print("Message: "); Serial.println(msg);

  Serial.flush();
  Serial.print("Numero recebido: "); Serial.println(msgComoNumero);
  Serial.println(msg);
  Serial.flush();
  
}
//=======================================================================

EthernetClient ethClient;
PubSubClient client(server,port,callback,ethClient);

char clientMQTTID[] = "MQTT-Senai-IoT-A_ACG";
char userMQTT[] = "IoT-A";
char passMQTT[] = "S3n41-1o7";

void setup() {

  serialSetup();
//  ethernetSetup();
//  serverMQTTSetup();
  pinMode(spotLight, OUTPUT);
  pinMode(button, INPUT);
  myservo.attach(7);
  
}

void loop() {

  //inputRequest();
  readButtonState();

  if (actionDoor != beforeActionDoor) {

     if ((actionDoor == 'A' && lockDoor) || (actionDoor == 'F' && !lockDoor)) {
    
        if (lockDoor) {
           lockDoor = openDoor();
           delay(1000);
           light = lightOn();
        } else {
          lockDoor = closeDoor();
          delay(1000);
          light = lightOff();    
        }

     } 
  
     showInformation();  
     beforeActionDoor = actionDoor;

    // delay(5000);

  }
  
}

void serialSetup() {

  Serial.begin(9600);
  while (!Serial) {    
  }
  Serial.println("Inicio");
 
}

boolean openDoor() {

   for (pos = 0; pos <= 90; pos += 1) {
        myservo.write(pos);
        delay(speedOpen);
  }  

  return false;

}

boolean closeDoor() {

  for (pos = 90; pos >= 0; pos -= 1) {
    myservo.write(pos);
    delay(speedClose);
  }

  return true;
  
}


void showInformation() {

  String message;
  
  if (actionDoor == 'A' && beforeActionDoor == 'F') {
    message = "Garagem aberta e luzes acessas!";
  } else if (actionDoor == 'F' && beforeActionDoor == 'A') {
    message = "Garagem fechada e luzes apagadas!";
  } else {
    if (actionDoor == 'A') {
      message = "Nao e possivel abrir a porta! Garagem ja aberta!";
    } else {
      message = "Nao e possivel fechar a porta! Garagem ja fechada!";
    }
  }
  
  Serial.println(message);
  
}

boolean lightOn() {
  digitalWrite(spotLight, HIGH);
  return true;
}

boolean lightOff() {
  digitalWrite(spotLight, LOW);
  return false;
}

void inputRequest() {
  if (!client.connected()) {
    reconnectMQTT();
  }
}

void ethernetSetup() {
  
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Falha em configurar Ethernet usando DHCP");
    Ethernet.begin(mac, ip);
  }
  delay(1000);
  Serial.println("Ethernet Ok...");
  
}

void serverMQTTSetup() {

   client.setServer(server, port);
   client.setCallback(callback);
  
}

void reconnectMQTT() {
  
    char messageMQTT[] = "Teste";

    while (!client.connected()) {
      
       Serial.print("Conectando MQTT ...");
    
       if (client.connect(clientMQTTID,userMQTT,passMQTT)) {
          Serial.println("conectado");    
          client.publish(topicPub,messageMQTT);
          client.subscribe(topicSub);
       } else {
          Serial.print("falha, rc=");
          Serial.print(client.state());
          Serial.println(" nova tentativa em 5 segundos");
          delay(5000);
       }
   }

}

void readButtonState() {
  
   buttonState = digitalRead(button);

    if (buttonState == LOW && buttonStateBefore == HIGH ) {
      
       if (actionDoor == 'A') {
          actionDoor = 'F';
       } else {
          actionDoor = 'A';
       }
       
    }

    buttonStateBefore = buttonState;
}

