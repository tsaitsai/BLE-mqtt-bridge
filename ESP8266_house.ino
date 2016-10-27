/* Eric Tsai, github.com/tsaitsai
 * magic bluetooth house
 * updated 2016-10-22: added servo motor to default mqtt example
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Servo.h>

// Update these with values suitable for your network.

const char* ssid = "november24";
const char* password = "letmeinplease";
const char* mqtt_server = "192.168.3.34";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

/*
static const uint8_t D0   = 16;
static const uint8_t D1   = 5;
static const uint8_t D2   = 4;
static const uint8_t D3   = 0;
static const uint8_t D4   = 2;
static const uint8_t D5   = 14;
static const uint8_t D6   = 12;
static const uint8_t D7   = 13;
static const uint8_t D8   = 15;
static const uint8_t D9   = 3;
static const uint8_t D10 = 1;
static const uint8_t SDA = 4;
static const uint8_t SCL = 5;
static const uint8_t LED_BUILTIN = 16;
static const uint8_t BUILTIN_LED = 16;
 */



//ultrasonic
int trigPin = 5;    //Trig, pin 5=D1 on ESP8266
int echoPin = 4;    //Echo, pin 4=D2 on ESP8266


  //servo pin
  int servoPin = 14; // attach servo to pin D5, which is 14 on NodeMCU
  int servoGarage =  0; //pin D6 = 12;  D3=0
  int servoFace = 13; //pin D7 = 13, 
  int servoMailbox = 15; //pin D8 = 15


//ultrasonic
long duration, cm, inches;
long lastUltra;

//servo front door
Servo myservo;  // create servo object to control a servo
int servo_pos = 0;    // variable to store the servo position
const int servo_min = 30;
const int servo_max = 110;
long servo_time = 0;
int servo_target = 0;

//servo garage door
Servo myservo_garage;
int myservo_garage_pos = 0;
const int myservo_garage_min = 1;
const int myservo_garage_max = 170;
int myservo_garage_target = 0;

//servo face
Servo myservo_face;
int myservo_face_pos = 0;
const int myservo_face_min = 10;
const int myservo_face_max = 170;
int myservo_face_target = 0;

//servo mail
Servo myservo_mail;
int myservo_mail_pos = 0;
const int myservo_mail_min = 10;
const int myservo_mail_max = 90;
int myservo_mail_target = 0;

//flag for incoming mqtt
int topic_flag;
const int topic_front = 1;
const int topic_garage = 2;
const int topic_face = 3;
const int topic_mail = 4;

//callback
long manual_time = 0;
bool flag_manual = 0;


void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address is is: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();


  //TOPIC NAME
  //find out which bluetooth module talked
  if ((topic[0] == '5') && (topic[1] == '0') && (topic[2] == 'C'))
    Serial.println("is 50C51C570B00");
  if ((topic[0] == '7') && (topic[1] == '4') && (topic[2] == '7'))
    Serial.println("is 747E0C570B00");



    
  //if (topic=="747E0C570B00")      //messy tape, mailbox
  if ((topic[0] == '7') && (topic[1] == '4') && (topic[2] == '7'))
  {
    Serial.println("is 747E0C570B00 mailbox");
    topic_flag = topic_mail;
    if  ((char)payload[0] == '1')
    {
      myservo_mail_target = myservo_mail_min;
    }
    else
    {
      myservo_mail_target = myservo_mail_max;
    }
  }//end if messy tape mailbox
    
  //if (topic=="2D490B570B00")      //good reed switch 2D, front door
  if ((topic[0] == '2') && (topic[1] == 'D') && (topic[2] == '4'))
  {
    Serial.println("is 2D490B570B00 front door");
    topic_flag = topic_front;
    if  ((char)payload[0] == '1')
    {
      servo_target = servo_max;
    }
    else
    {
      servo_target = servo_min;
    }
  }//end if front door
  
  //if (topic=="BE710C570B00")      //good reed switch BE, garage door
  if ((topic[0] == 'B') && (topic[1] == 'E') && (topic[2] == '7'))
  {
    Serial.print("is BE710C570B00 garage");
    topic_flag = topic_garage;
    if  ((char)payload[0] == '1')
    {
      myservo_garage_target  = myservo_garage_min;
      Serial.println(" close");
    }
    else
    {
      myservo_garage_target = myservo_garage_max;
      Serial.println(" opened");
    }
  }//end if garage door

  //if (topic=="faceface")      //face topic 50C51C570B00
  if ((topic[0] == '5') && (topic[1] == '0') && (topic[2] == 'C'))
  {
    Serial.println("is 50C51C570B00 face");
    topic_flag = topic_face;
    if  ((char)payload[0] == '1')
    {
      myservo_face_target   = myservo_face_min;
    }
    else
    {
      myservo_face_target  = myservo_face_max;
    }
  }//end if garage door



  flag_manual = 1;
  manual_time = millis();
  Serial.println("manual mode ON");

  Serial.print("garage target = ");
  Serial.print(myservo_garage_target);
  Serial.print("garage pos = ");
  Serial.println(myservo_garage_pos);
  Serial.print("serv target = ");
  Serial.print(servo_target);
  Serial.print("servo pos = ");
  Serial.println(servo_pos);

  //  PAYLOAD
  // Switch on the LED if an 1 was received as first character
  if ( ((char)payload[0] == '1') || ((char)payload[0] == '3') || ((char)payload[0] == '5') ) {
    digitalWrite(BUILTIN_LED, HIGH);   // Turn the LED OFF (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
    Serial.println("MQTT received 1, 3 or 5");
    
  } else {
    digitalWrite(BUILTIN_LED, LOW);  // Turn the LED ON by making the voltage HIGH
   
    Serial.println("MQTT received not not");
  }

}//end call back

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    //if (client.connect(clientId.c_str())) {
    if (client.connect("memq")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("747E0C570B00");   //bad tape
      client.subscribe("BE710C570B00");   //good reed 2
      client.subscribe("2D490B570B00");   //good reed 1
      client.subscribe("1111");
      client.subscribe("50C51C570B00");   //switched ble
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  //ultrasonic
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  //servo
  myservo.attach(servoPin);  // attach servo to pin 2, which is D4 on NodeMCU
  myservo_garage.attach(servoGarage);
  myservo_face.attach(servoFace);
  myservo_mail.attach(servoMailbox);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();

  


  //ultrasonic
  if (now - lastUltra > 5000)
  {
    lastUltra = now;
    
    //ultrasonic
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    // Read the signal from the sensor: a HIGH pulse whose
    // duration is the time (in microseconds) from the sending
    // of the ping to the reception of its echo off of an object.
    //pinMode(echoPin, INPUT);
    duration = pulseIn(echoPin, HIGH);
    // convert the time into a distance
    cm = (duration/2) / 29.1;
    inches = (duration/2) / 74; 
    //Serial.print(inches);
    //Serial.print("in, ");
    //Serial.print(cm);
    //Serial.print("cm");
    //Serial.println();
    //delay(250);

    //target servo
    if (!flag_manual)
    {
      //servo_target = (int)((inches * servo_max)/15);
      //Serial.println("set target by ultrasonic");
    }

    //MQTT publish
    snprintf (msg, 75, "%ld", inches);
    //Serial.print("Publish message: ");
    //Serial.println(msg);
    //client.publish("Sensor_dist", msg);
    //snprintf (msg, 75, "%ld", servo_target);
    //client.publish("target", msg);




    
  }//if lastUltra

  //servo control
  if (now - servo_time > 150)
  {
    servo_time = now;





    //**** front door servo position
    //slowly move towards target
    if (servo_pos < servo_target-9)     // 30 < 30 - 9
    {
      servo_pos = servo_pos + 10;
    }
    else if (servo_pos > servo_target+9) // 20 > 15+9
    {
      servo_pos = servo_pos - 10;
    }

    
    //check max/min
    if (servo_pos > servo_max)
    {
      servo_pos = servo_max;
    }
    else if (servo_pos < servo_min)
    {
      servo_pos = servo_min;
    }
    
    myservo.write(servo_pos);
    

    //**** garage door servo position
    //slowly move towards target
    if (myservo_garage_pos < myservo_garage_target -9)     // 30 < 30 - 9
    {
      myservo_garage_pos = myservo_garage_pos + 10;
    }
    else if (myservo_garage_pos > myservo_garage_target+9) // 20 > 15+9
    {
      myservo_garage_pos = myservo_garage_pos - 10;
    }
    //check max/min
    
    if (myservo_garage_pos > myservo_garage_max )
    {
      myservo_garage_pos = myservo_garage_max ;
    }
    else if (myservo_garage_pos < myservo_garage_min )
    {
      myservo_garage_pos = myservo_garage_min ;
    }
    myservo_garage.write(myservo_garage_pos);



    //**** mailbox servo position
    //slowly move towards target
    if (myservo_mail_pos < myservo_mail_target -9)     // 30 < 30 - 9
    {
      myservo_mail_pos = myservo_mail_pos + 10;
    }
    else if (myservo_mail_pos > myservo_mail_target+9) // 20 > 15+9
    {
      myservo_mail_pos = myservo_mail_pos - 10;
    }
    //check max/min
    
    if (myservo_mail_pos > myservo_mail_max )
    {
      myservo_mail_pos = myservo_mail_max ;
    }
    else if (myservo_mail_pos < myservo_mail_min )
    {
      myservo_mail_pos = myservo_mail_min ;
    }
    myservo_mail.write(myservo_mail_pos);


    //**** mailbox servo position
    //slowly move towards target
    if (myservo_face_pos < myservo_face_target - 9)     // 30 < 30 - 9
    {
      myservo_face_pos = myservo_face_pos + 10;
    }
    else if (myservo_face_pos > myservo_face_target+9) // 20 > 15+9
    {
      myservo_face_pos = myservo_face_pos - 10;
    }
    //check max/min
    
    if (myservo_face_pos > myservo_face_max )
    {
      myservo_face_pos = myservo_face_max ;
    }
    else if (myservo_face_pos < myservo_face_min )
    {
      myservo_face_pos = myservo_face_min ;
    }
    myservo_face.write(myservo_face_pos);



    
  }//end if servo_time



  
} //end loop
  



