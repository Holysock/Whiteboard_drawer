// Lets draw saucy ponys !!!1elf!!

#include <StepControl.h>
#include <SoftwareSerial.h>   // We need this even if we're not using a SoftwareSerial object
#include <Servo.h>

#define HWSERIAL Serial1

#define LENGTH_MAX 1645//2340 //max length of both threads in mm
#define LENGTH_C 1250 //length of C in mm (distance between spules)
#define M2_TRANSLATION_X 0 // x position of M2 origin in M1 
#define M2_TRANSLATION_Y (sqrt(pow(LENGTH_MAX, 2) - pow(LENGTH_C / 2.0, 2))) // y position of M2 origin in M1, use is thread length is limited
#define DELTA_L 10.0
#define STEP_PER_REVOLUTION 400
#define DIAMETER 38
#define LENGTH_PER_REVOLUTION  (DIAMETER * PI) //mm
#define LPS (LENGTH_PER_REVOLUTION / STEP_PER_REVOLUTION)

#define SERVOUP 40
#define SERIALBUFFERLEN 40 //bufferlength of serial parser

Servo myservo;

struct Position_in_M1_Struct {
  float x;
  float y;
};

struct Position_in_M2_Struct {
  float a;
  float b;
};

float xPos;  //x pos
float yPos; //y pos
float zPos; //z pos

char *pos2; //pointer 1
char *pos1; //pos pointer 2

int servodown = 50; //servoposition value
int linecount;      //internal linecounter

char Message[SERIALBUFFERLEN]; // incoming String

float feedRate = 250;

struct Position_in_M1_Struct pos_in_M1;
struct Position_in_M2_Struct pos_in_M2;

Stepper motor_a(11, 10);   //STEP pin =  2, DIR pin = 3
Stepper motor_b(9, 8);  //STEP pin =  9, DIR pin = 10

StepControl<> controller;

char serialData[8];

void setup()
{
  myservo.attach(3);
  myservo.write(SERVOUP);
  delay(100);
  Serial.begin(115200);
  HWSERIAL.begin(115200);

  // setup the motors
  motor_b
  .setMaxSpeed(feedRate)       // steps/s
  .setAcceleration(10000000); // steps/s^2

  motor_a
  .setMaxSpeed(feedRate)       // steps/s
  .setAcceleration(100000000); // steps/s^2

  /*
  delay(1000);

  pos_in_M1.x = 0;//(LENGTH_C / 2.0) - 300;
  pos_in_M1.y = 0;
  //line(pos_in_M1)
  pos_in_M2 = to_M2(pos_in_M1);
  Serial.println(pos_in_M2.a);
  Serial.println(pos_in_M2.b);
  pos_in_M1 = to_M1(pos_in_M2);
  Serial.println(pos_in_M1.x);
  Serial.println(pos_in_M1.y);
  while(1) delay(1);
  */
}

void loop()
{
  if (Serial.available() > 0) { //skip unconnected slots
    memset(&Message[0], 0, sizeof(Message)); //clear the array as the message length varies
    pos1 = 0; //needs to be cleared for every client
    pos2 = 0; //this one too
    for (int j = 0; j < SERIALBUFFERLEN - 1; j++) { //fetch the message
      Message[j] = Serial.read(); //faster the readBytesUntil()
      if (Message[j] == '\n') { //exit on newline
        break;
      }
    }

    if (!strncmp(Message, "G01 ", 3) || !strncmp(Message, "G00 ", 3)) { //parsing part, it works...
      for (int j = 3; j < SERIALBUFFERLEN - 1; j++) { //iterate throu the message
        if (Message[j] == 'X') {  //check for an X
          pos1 = Message + j; //set pointer
          while (Message[++j] == ' ')pos1++;  //increment until next whitespace
          pos_in_M1.x = strtof(Message + j, &pos1);  //get float value
        } else if (Message[j] == 'Y') { //check for an Y
          pos1 = Message + j; //set pointer
          while (Message[++j] == ' ')pos1++; //increment until next whitespace
          pos_in_M1.y = strtof(Message + j, &pos1); //get float value
        } else if (Message[j] == 'Z') { //check for an Y
          pos1 = Message + j; //set pointer
          while (Message[++j] == ' ')pos1++; //increment until next whitespace
          servodown = (int)strtof(Message + j, &pos1); //get float value
          pos_in_M1.y = strtof(Message + j, &pos1); //get float value
        } else if (Message[j] == 'S') { //check for an Z
          pos1 = Message + j; //set pointer
          while (Message[++j] == ' ')pos1++; //increment until next whitespace
          zPos = strtof(Message + j, &pos1); //get float value
        }
      }
    } else if (!strncmp(Message, "G28 ", 3)) {
      for (int j = 3; j < SERIALBUFFERLEN - 1; j++) { //iterate throu the message
        if (Message[j] == 'X') {  //check for an X
          pos1 = Message + j; //set pointer
          while (Message[++j] == ' ')pos1++;  //increment until next whitespace
          motor_b.setPosition(0);       // steps/s
          motor_a.setPosition(0);      // steps/s
        } else if (Message[j] == 'Y') {  //check for an X
          pos1 = Message + j; //set pointer
          while (Message[++j] == ' ')pos1++;  //increment until next whitespace
          motor_b.setPosition(0);       // steps/s
          motor_a.setPosition(0);      // steps/s
        } else if (Message[j] == 'Z') {  //check for an X
          pos1 = Message + j; //set pointer
          while (Message[++j] == ' ')pos1++;  //increment until next whitespace
          zPos = 0;
        }
      }
    } else if (!strncmp(Message, "G95 ", 3)) {
      for (int j = 3; j < SERIALBUFFERLEN - 1; j++) { //iterate throu the message
        if (Message[j] == 'M') {  //check for an X
          pos1 = Message + j; //set pointer
          while (Message[++j] == ' ')pos1++;  //increment until next whitespace
          feedRate = strtof(Message + j, &pos1);  //get float value
          motor_b.setMaxSpeed(feedRate);       // steps/s
          motor_a.setMaxSpeed(feedRate);      // steps/s
        }
      }
    }
  }

  while (controller.isRunning())delay(0); //wait untill all movement is finished, then start the new one

  if (zPos == 1000) myservo.write(servodown);
  else if (zPos == 0) myservo.write(SERVOUP);

  line(pos_in_M1);
  Serial.println("done");
}




////////////////////////////////////////////////////////
//        MATHE-FOO DO NOT PFOT!                      //
////////////////////////////////////////////////////////


// M1 = kartesian, M2 = custom triangle whiteboad system
// Transformation M1 -> M2
struct Position_in_M2_Struct to_M2(Position_in_M1_Struct M1_pos) {
  struct Position_in_M2_Struct M2_pos;
  M2_pos.a = sqrt( pow(M1_pos.x + LENGTH_C/2.0, 2) +  pow(M2_TRANSLATION_Y - M1_pos.y, 2));
  M2_pos.b = sqrt( pow(M1_pos.x - LENGTH_C/2.0, 2) +  pow(M2_TRANSLATION_Y - M1_pos.y, 2));
  return M2_pos;
}

// Transformation M2 -> M1
struct Position_in_M1_Struct to_M1(Position_in_M2_Struct M2_pos) {
  struct Position_in_M1_Struct M1_pos;
  float a = M2_pos.a;
  float b = M2_pos.b;
  float c = LENGTH_C;
  float cos_alpha = (c * c + b * b - a * a) / (2 * b * c);
  M1_pos.x = LENGTH_C/2.0 - b * cos_alpha;
  M1_pos.y = M2_TRANSLATION_Y - b * sin(acos(cos_alpha)); //use if thread length is limited
  return M1_pos;
}

int length2step(float l) {
  return (LENGTH_MAX - l) / LPS;
}

float step2length(int s) {
  return (float)(LENGTH_MAX - s * LPS);
}

void line(Position_in_M1_Struct M1_target) {
  struct Position_in_M2_Struct M2_cur;
  struct Position_in_M2_Struct M2_tmp;
  struct Position_in_M1_Struct M1_cur;
  struct Position_in_M1_Struct M1_del;

  M2_cur.a = step2length(motor_a.getPosition());
  M2_cur.b = step2length(motor_b.getPosition());

  M1_cur = to_M1(M2_cur);

  float x_d = M1_target.x - M1_cur.x;
  float y_d = M1_target.y - M1_cur.y;
  float distance = sqrt(x_d * x_d + y_d * y_d);
  if (distance <= 1.2) return;
  float divisor = (int)(distance / DELTA_L);
  if (divisor < 1) divisor = 1;

  for (int i = 1; i <= divisor; i++) {
    M1_del.x = M1_cur.x + x_d * (i / divisor);
    M1_del.y = M1_cur.y + y_d * (i / divisor);
    M2_tmp = to_M2(M1_del);
    motor_a.setTargetAbs(length2step(M2_tmp.a));
    motor_b.setTargetAbs(length2step(M2_tmp.b));
    controller.move(motor_a, motor_b);
  }
}






