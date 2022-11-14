#include <SoftwareSerial.h>  //Software Serial Port
#define RxD 4
#define TxD 5

int recvcmp(const char *str);
int strcmp(char *a, const char *b);

int setupBlueToothConnection();
int sendBlueToothCommand(char command[]);
int recvMsg(unsigned int timeout);
long water_measure();

SoftwareSerial blueToothSerial(RxD, TxD);  //the software serial port

int trigPin = 6;
int echoPin = 7;
int Slide1 = 8;                                //slide 버튼
int Slide2 = 9;                                //slide 버튼
int L = 6;                                     //물통 바닥 길이
long int_water = 0, fin_water = 0, drink = 0;  //처음 물의 양과 나중 물의 양, 최종 음수량
int water_state = 0;                           //물 측정 상태
long duration, distance, r, water;             //물 수위 측정을 위한 변수
char recv_str[100];

void setup() {
  Serial.begin(9600);        //Serial port for debugging
  pinMode(8, INPUT_PULLUP);  //slide
  pinMode(9, INPUT_PULLUP);  //slide
  pinMode(RxD, INPUT);       //UART pin for Bluetooth
  pinMode(TxD, OUTPUT);      //UART pin for Bluetooth
  pinMode(echoPin, INPUT);   // echoPin 입력
  pinMode(trigPin, OUTPUT);  // trigPin 출력
  Serial.println("\r\nPower on!!");
  setupBlueToothConnection();  //initialize Bluetooth
                               //    this block is waiting for connection was established.
  sendBlueToothCommand("AT+START");

  while (1) {
    if (recvMsg(100) == 0) {
      if (recvcmp("OK+CONN") == 0) {
        Serial.println("connected\r\n");
        break;
      }
    }
    delay(200);
  }
}

void loop() {
  if (recvMsg(200) == 0) {
    if (recvcmp("OK+LOST") == 0) {
      Serial.println("disconnected");
      while (1) {
        if (sendBlueToothCommand("AT") == 0) {
          if (recvcmp("OK") == 0) {
            Serial.println("Bluetooth exists\r\n");
            break;
          }
        }
        delay(500);
      }
      sendBlueToothCommand("AT+START");
      while (1) {
        if (recvMsg(100) == 0) {
          if (recvcmp("OK+CONN") == 0) {
            Serial.println("connected\r\n");
            break;
          }
        }
        delay(200);
      }
    }
  }
  //처음 물 측정 시작
  if ((water_state == 0) && (digitalRead(Slide1) == 1)) {
    drink = 0;
    Serial.println("start");
    int_water = water_measure();
    Serial.println(int_water);
    water_state = 1;
  } else if ((water_state == 1) && (digitalRead(Slide2) == 1)) {
    Serial.println("finish");
    fin_water = water_measure();
    Serial.println(fin_water);
    water_state = 0;
    drink = int_water - fin_water;
  }

  delay(300);
  if (drink > 0) {
    blueToothSerial.println(drink);
    drink = 0;
  }
}

int recvcmp(const char *str) {
  return strcmp((char *)recv_str, str);
}

int strcmp(char *a, const char *b) {
  for (unsigned int ptr = 0; a[ptr] != '\0'; ptr++) {
    if (a[ptr] != b[ptr]) return -1;
  }
  return 0;
}

//configure the Bluetooth through AT commands
int setupBlueToothConnection() {
  Serial.println("this is SLAVE\r\n");

  Serial.print("Setting up Bluetooth link\r\n");
  delay(2000);  //wait for module restart
  blueToothSerial.begin(9600);

  //wait until Bluetooth was found
  while (1) {
    if (sendBlueToothCommand("AT") == 0) {
      if (recvcmp("OK") == 0) {
        Serial.println("Bluetooth exists\r\n");
        break;
      }
    }
    delay(500);
  }

  //configure the Bluetooth
  sendBlueToothCommand("AT+RENEW");  //restore factory configurations
  delay(2000);
  sendBlueToothCommand("AT+IMME1");
  sendBlueToothCommand("AT+NAMEKDB");
  sendBlueToothCommand("AT+ROLE0");    //set to slave mode
  sendBlueToothCommand("AT+RESTART");  //restart module to take effect
  delay(2000);                         //wait for module restart


  //check if the Bluetooth always exists
  if (sendBlueToothCommand("AT") == 0) {
    if (recvcmp("OK") == 0) {
      Serial.print("Setup complete\r\n\r\n");
      return 0;
    }
  }

  return -1;
}

//send command to Bluetooth and return if there is a response received
int sendBlueToothCommand(char command[]) {
  Serial.print("send: ");
  Serial.print(command);
  Serial.println("");

  blueToothSerial.print(command);
  delay(300);

  if (recvMsg(200) != 0) return -1;

  Serial.print("recv: ");
  Serial.print(recv_str);
  Serial.println("");
  return 0;
}

//receive message from Bluetooth with time out
int recvMsg(unsigned int timeout) {
  //wait for feedback
  unsigned int time = 0;
  unsigned char num;
  unsigned char i;

  //waiting for the first character with time out
  i = 0;
  while (1) {
    delay(50);
    if (blueToothSerial.available()) {
      recv_str[i] = char(blueToothSerial.read());
      i++;
      break;
    }
    time++;
    if (time > (timeout / 50)) return -1;
  }

  //read other characters from uart buffer to string
  while (blueToothSerial.available() && (i < 100)) {
    recv_str[i] = char(blueToothSerial.read());
    i++;
  }
  recv_str[i] = '\0';

  return 0;
}


long water_measure() {

  digitalWrite(trigPin, HIGH);  // trigPin에서 초음파 발생(echoPin도 HIGH)
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);  // echoPin 이 HIGH를 유지한 시간을 저장 한다.
  distance = ((float)(340 * duration) / 1000) / 2;
  water = L * L * (120 - distance) / 10;  //물 부피 계산
  return water;
}