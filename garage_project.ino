#define ONLINE 0

#include <Servo.h>
#include <Ethernet.h>
#include <SPI.h>

#ifdef ONLINE
#include <PubSubClient.h>
#else
#include <SerialPubSubClient.h>
#endif

//=======================================================================
void callback(const char topic, byte* payload, unsigned int length);
//=======================================================================

Servo myservo;

int pos = 0;
int speedOpen = 15;
int speedClose = 15;
boolean lockDoor = true;
boolean light = false;
int spotLight = 3;

int onlineLight = 12;
int offlineLight = 2;

int buttonDoor = 8;
int buttonLight = 9;

boolean online = 0;

char actionDoor = 'F';
char beforeActionDoor = 'F';

int buttonDoorState = 0;
int buttonDoorStateBefore = 0;

int buttonLightState = 0;
int buttonLightStateBefore = 0;

char topicPub[] = "Garage_Project";
char topicSub[] = "Garage_Project";

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xAE, 0x03 };
IPAddress ip (192, 162, 1, 100);
char server[] = "m10.cloudmqtt.com";
int port = 12598;

char clientMQTTID[] = "MQTT-Senai";
char userMQTT[] = "IoT-A";
char passMQTT[] = "S3n41-1o7";

EthernetClient ethClient;
PubSubClient client(server,port,callback,ethClient);

void setup() {

  serialSetup();
  ethernetSetup();
  pinMode(spotLight, OUTPUT);
  
  pinMode(onlineLight, OUTPUT);
  pinMode(offlineLight, OUTPUT);
  
  pinMode(buttonLight, INPUT);
  pinMode(buttonDoor, INPUT);
  
  myservo.attach(7);
  myservo.write(0);
  
}

void loop() {

  inputRequest();
  showOnline();
  
  client.loop();
  
  readButtonDoorState();
  readButtonLightState();

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
  char msgMQTT; 

  if (!client.connected()) {
     reconnectMQTT();
  }

  if (actionDoor == 'A' && beforeActionDoor == 'F') {
    message = "Garagem aberta e luzes acessas!";
    client.publish(topicPub, "A");
    msgMQTT = "A";
  } else if (actionDoor == 'F' && beforeActionDoor == 'A') {
    message = "Garagem fechada e luzes apagadas!";
    client.publish(topicPub, "F");   
    msgMQTT = "F";

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

void reconnectMQTT() {
  
    char messageMQTT[] = "Online";
    while (!client.connected()) {
      
       Serial.print("Conectando MQTT ...");
    
       if (client.connect(clientMQTTID,userMQTT,passMQTT)) {
          Serial.println("conectado");    
          client.publish(topicPub,messageMQTT);
          if (!client.subscribe("Garage_Project")) {
              Serial.println("Erro na subscrição");
          } else {
              Serial.println("Subscrição realizada com sucesso");
          }
          online = 1;
       } else {
          Serial.print("falha, rc=");
          Serial.print(client.state());
          Serial.println(" nova tentativa em 5 segundos");
          
          online = 0;
       }
   }

}

void readButtonDoorState() {
  
   buttonDoorState = digitalRead(buttonDoor);

    if (buttonDoorState == LOW && buttonDoorStateBefore == HIGH ) {
      
       if (actionDoor == 'A') {
          actionDoor = 'F';
       } else {
          actionDoor = 'A';
       }
       
    }

    buttonDoorStateBefore = buttonDoorState;
}

void readButtonLightState() {
  
   buttonLightState = digitalRead(buttonLight);

    if (buttonLightState == LOW && buttonLightStateBefore == HIGH ) {
      
       if (light) {
          light = lightOff();
       } else {
          light = lightOn();
       }
       
    }

    buttonLightStateBefore = buttonLightState;
}

void showOnline() {

  
  if (online) {
     digitalWrite(onlineLight, HIGH);  
     digitalWrite(offlineLight, LOW);    
  } else {
     digitalWrite(offlineLight, HIGH);
     digitalWrite(onlineLight, LOW);    
  }

}

void showInformationMQTT(String msg) {

  char msgMQTT = msg.c_str();

  if (!client.connected()) {
     reconnectMQTT();
  }
  
  client.publish(topicPub, msgMQTT);
   
}
     
void callback(const char topic, byte* payload, unsigned int length) {
  
  // handle message arrived
  Serial.print("Callback: ");
  Serial.println(String(topic));

  char* payloadAsChar = payload;

  // Workaround para pequeno bug na biblioteca
  payloadAsChar[length] = 0;

  // Converter em tipo String para conveniência
  String topicStr = String(topic);
  String msg = String(payloadAsChar);
  Serial.print("Topic received: "); Serial.println(String(topic));
  Serial.print("Message: "); Serial.println(msg);

  if (msg == "A") {
     actionDoor = 'A';
  } else if (msg == "F") {
     actionDoor = 'F';
  } 

  Serial.println(actionDoor);

  // Dentro do callback da biblioteca MQTT,
  // devemos usar Serial.flush() para garantir que as mensagens serão enviadas
  Serial.flush();

  // https://www.arduino.cc/en/Reference/StringToInt
  int msgComoNumero = msg.toInt();

  Serial.print("Numero recebido: "); Serial.println(msgComoNumero);
  Serial.flush();

}
     
