
// Lets draw saucy ponys !!!1elf!!


#include <StepControl.h>
#include <SoftwareSerial.h>   // We need this even if we're not using a SoftwareSerial object
// Due to the way the Arduino IDE compiles

#define HWSERIAL Serial1

#define LENGTH_MAX 1645//2340 //max length of both threads in mm
#define LENGTH_C 1250 //length of C in mm (distance between spules)
#define M2_TRANSLATION_X 0 // x position of M2 origin in M1 
#define M2_TRANSLATION_Y (sqrt(pow(LENGTH_MAX, 2) - pow(LENGTH_C / 2.0, 2))) // y position of M2 origin in M1, use is thread length is limited
#define DELTA_L 10.0
#define STEP_PER_REVOLUTION 400
#define SPEED_MAX 250
#define DIAMETER 38
#define LENGTH_PER_REVOLUTION  (DIAMETER * PI) //mm
#define LPS (LENGTH_PER_REVOLUTION / STEP_PER_REVOLUTION) 

struct Position_in_M1_Struct{
  float x;
  float y;
};

struct Position_in_M2_Struct{
  float a;
  float b;
};

struct Position_in_M1_Struct pos_in_M1;
struct Position_in_M2_Struct pos_in_M2;

Stepper motor_a(11, 10);   //STEP pin =  2, DIR pin = 3
Stepper motor_b(9, 8);  //STEP pin =  9, DIR pin = 10

StepControl<> controller;

char serialData[8];

void setup()
{

  Serial.begin(9600);
  HWSERIAL.begin(115200);

// setup the motors
  motor_b
  .setMaxSpeed(SPEED_MAX)       // steps/s
  .setAcceleration(10000000); // steps/s^2

  motor_a
  .setMaxSpeed(SPEED_MAX)       // steps/s 
  .setAcceleration(100000000); // steps/s^2

  delay(5000);

  /*pos_in_M1.x = 200;
  pos_in_M1.y = 0;
  Serial.printf("Original: x: %f y: %f \n", pos_in_M1.x, pos_in_M1.y);

  pos_in_M1 = to_M1(to_M2(pos_in_M1));
  Serial.printf("Back in M1 x: %f y: %f \n", pos_in_M1.x, pos_in_M1.y);
  */

  pos_in_M1.x = (LENGTH_C / 2.0) - 300;
  pos_in_M1.y = 0;
  line(pos_in_M1);


}

void loop()
{
 
  pos_in_M1.y = 600;
  line(pos_in_M1);

  pos_in_M1.x = (LENGTH_C / 2.0) + 300;
  line(pos_in_M1);

  pos_in_M1.y = 0;
  line(pos_in_M1);

  pos_in_M1.x = (LENGTH_C / 2.0) -300;
  //pos_in_M1.y = 0;
  line(pos_in_M1);
  
}






////////////////////////////////////////////////////////
//        MATHE-FOO DO NOT PFOT!                      //
////////////////////////////////////////////////////////


// M1 = kartesian, M2 = custom triangle whiteboad system
// Transformation M1 -> M2 
struct Position_in_M2_Struct to_M2(Position_in_M1_Struct M1_pos){
  struct Position_in_M2_Struct M2_pos;
  M2_pos.a = sqrt( pow(M1_pos.x, 2) +  pow(M2_TRANSLATION_Y - M1_pos.y, 2));
  M2_pos.b = sqrt( pow(M1_pos.x - LENGTH_C, 2) +  pow(M2_TRANSLATION_Y - M1_pos.y, 2));
  return M2_pos;
}

// Transformation M2 -> M1
struct Position_in_M1_Struct to_M1(Position_in_M2_Struct M2_pos){
  struct Position_in_M1_Struct M1_pos;
  float a = M2_pos.a;
  float b = M2_pos.b;
  float c = LENGTH_C;
  float cos_alpha = (c*c + b*b - a*a) / (2*b*c);
  M1_pos.x = LENGTH_C - b*cos_alpha;
  M1_pos.y = M2_TRANSLATION_Y - b*sin(acos(cos_alpha)); //use if thread length is limited
  return M1_pos;
}

int length2step(float l){
  return (LENGTH_MAX - l) / LPS;
}

float step2length(int s){
  return (float)(LENGTH_MAX - s * LPS);
}

void line(Position_in_M1_Struct M1_target){
  struct Position_in_M2_Struct M2_cur;
  struct Position_in_M2_Struct M2_tmp;
  struct Position_in_M1_Struct M1_cur;
  struct Position_in_M1_Struct M1_del;
  
  M2_cur.a = step2length(motor_a.getPosition());
  M2_cur.b = step2length(motor_b.getPosition());

  M1_cur = to_M1(M2_cur);
  
  float x_d = M1_target.x - M1_cur.x;
  float y_d = M1_target.y - M1_cur.y;
  float distance = sqrt(x_d*x_d + y_d*y_d);
  float divisor = (int)(distance / DELTA_L);
  if(divisor < 1) divisor = 1;
  
  for(int i=1;i<=divisor;i++){
      M1_del.x = M1_cur.x + x_d * (i/divisor);
      M1_del.y = M1_cur.y + y_d * (i/divisor);
      M2_tmp = to_M2(M1_del);
      motor_a.setTargetAbs(length2step(M2_tmp.a));
      motor_b.setTargetAbs(length2step(M2_tmp.b));
      controller.move(motor_a, motor_b);    
  }
}






