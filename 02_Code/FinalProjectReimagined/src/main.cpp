#include <Arduino.h>
#include <math.h>
#include <Wire.h>
#include <SPI.h>

#include <Ramp.h>
#include <Adafruit_PWMServoDriver.h>
#include <utility/imumaths.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <PID_v1.h> 
#include <RF24.h>
#include <nRF24L01.h>
#include<externFunctions.h>



//SCK 18
//MOSI 8
//MISO 10
//CE 16
//CSN 9


//SPI.begin(16, 8, 10, 16);
struct PayloadStruct {
  bool eStop; //sw2
  int state;
  bool gyro;
  bool PID;
  float j1_x;
  float j1_y;
  float j2_x;
  float j2_y;
  bool j1_b;
  bool j2_b;
};
PayloadStruct payload;

//values for movement
//current /default values
float testHeight = 150;
float testHeightBACK = 170;
float testLR = 0;
float testFB = 0;

//time for a cycle (in ms)
float timee = 100;

//amount to change values by in a cycle
float backDistance = 50; //(FB)
float upDistance = -50; //xH
float LRDistance =100; //xLR


//motor definitions
// old way to desinate each leg, legacy, but the kinematic model depends on these values.
// DO NOT CHANGE
int aHip = 0; //
int bHip = 3; //
int cHip = 6; //
int dHip = 9; //

rampLeg aLeg(aHip);
rampLeg bLeg(bHip);
rampLeg cLeg(cHip);
rampLeg dLeg(dHip);

//angle variables
double yPreRot= 0, zPreRot = 0;
double yRot= 0, zRot = 0;

//PID setup for gyro
double Kp= .25, Ki=.01, Kd =0;
double angleGoal = 0;


 //soon to be deprecated, as the gyro will need to be init everytime to
 //allow switching of modes that have it enabled
#ifdef gyro
  Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28, &Wire);
#endif
sensors_event_t event;

PID yPID(&yPreRot, &yRot, &angleGoal, Kp, Ki, Kd, DIRECT);
PID zPID(&zPreRot, &zRot, &angleGoal, Kp, Ki, Kd, DIRECT);

//PCA9685 setup
#define SERVO_FREQ 50
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);
Adafruit_PWMServoDriver pwm1 = Adafruit_PWMServoDriver(0x41);



RF24 radio(16,9);
const byte thisSlaveAddress[5] = {'R','x','A','A','A'};

void setup() {
  Serial.begin(115200);
  Serial.println("alive");
  Wire.begin(17,15); //SDA, SCL
  Serial.println("IC2 alive");

  SPI.begin(18, 8, 10, 16);
  #ifdef gyro
    bno.begin();
    //set up gyro
    Serial.println("gyro started");
    if(!bno.begin())
    {
      /* There was a problem detecting the BNO055 ... check your connections */
      Serial.print("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
      while(1);
    }
    //bno.setExtCrystalUse(true);
    delay(1000); 
    Serial.println("gyroalive");
    yPID.SetOutputLimits(-45.0,45.0);
    zPID.SetOutputLimits(-45.0,45.0);

        //set the pid mode
    yPID.SetMode(AUTOMATIC);
    zPID.SetMode(AUTOMATIC);
    Serial.println("PID alive");
  #endif
  
    radio.begin();
    radio.setDataRate( RF24_250KBPS );
    radio.openReadingPipe(1, thisSlaveAddress);
    radio.startListening();

  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {}  // hold in infinite loop
  }

  Serial.println("radio Alive");
  //Set up PCA9685
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);  // Analog servos run at ~50 Hz updates
  delay(10);  

  pwm1.begin();
  pwm1.setOscillatorFrequency(27000000);
  pwm1.setPWMFreq(SERVO_FREQ);  // Analog servos run at ~50 Hz updates

  //where in the walk cycle the robot is at. Way to share state across all movements
  aLeg.setCycle(0);
  bLeg.setCycle(3);
  cLeg.setCycle(3);
  dLeg.setCycle(0);

  mainKinematics(testHeight, 0, 0, aHip,0,0,0);
  delay(200);
  mainKinematics(testHeight, 0, 0, cHip,0,0,0);
  mainKinematics(testHeight, 0, 0, bHip,0,0,0);
  delay(200);
  mainKinematics(testHeight, 0, 0, dHip,0,0,0);

  aLeg.reset();
  bLeg.reset();
  cLeg.reset();
  dLeg.reset();
}

void loop() {
  if (radio.available()) {
    radio.read(&payload, sizeof(PayloadStruct));
    Serial.print("recieved");
  }else{
    payload.eStop = true;
    //radio.printPrettyDetails();
  }
  
  #ifdef gyro  
    bno.getEvent(&event);
    delay(5);
    yPreRot = event.orientation.y;
    zPreRot = event.orientation.z;
   yPID.Compute();
    zPID.Compute();
  #endif
  #ifndef gyro
    yRot =0;
    zRot = 0;
  #endif

if(payload.eStop != true){
  switch (payload.state) {
    case 0:{ //standing
      if(payload.gyro && payload.PID){
        mainKinematics(150,0,0,aHip,0,yRot,zRot);
        mainKinematics(150,0,0,bHip,0,yRot,zRot);
        mainKinematics(150,0,0,cHip,0,yRot,zRot);
        mainKinematics(150,0,0,dHip,0,yRot,zRot);
      }else if(payload.gyro && !payload.PID){
        mainKinematics(150,0,0,aHip,0,yPreRot,zPreRot);
        mainKinematics(150,0,0,bHip,0,yPreRot,zPreRot);
        mainKinematics(150,0,0,cHip,0,yPreRot,zPreRot);
        mainKinematics(150,0,0,dHip,0,yPreRot,zPreRot);
      }else{
        mainKinematics(150,0,0,aHip,0,0,0);
        mainKinematics(150,0,0,bHip,0,0,0);
        mainKinematics(150,0,0,cHip,0,0,0);
        mainKinematics(150,0,0,dHip,0,0,0);
      }
    }
    case 1:{ //IK mode
      float xH = payload.j1_x * 150;
      float xLR = payload.j2_x * 50;
      float xFB = payload.j2_y * 50;
      if(payload.gyro && payload.PID){
        mainKinematics(xH,xFB,xLR,aHip,0,yRot,zRot);
        mainKinematics(xH,xFB,xLR,bHip,0,yRot,zRot);
        mainKinematics(xH,xFB,xLR,cHip,0,yRot,zRot);
        mainKinematics(xH,xFB,xLR,dHip,0,yRot,zRot);
      }else if(payload.gyro && !payload.PID){
        mainKinematics(xH,xFB,xLR,aHip,0,yPreRot,zPreRot);
        mainKinematics(xH,xFB,xLR,bHip,0,yPreRot,zPreRot);
        mainKinematics(xH,xFB,xLR,cHip,0,yPreRot,zPreRot);
        mainKinematics(xH,xFB,xLR,dHip,0,yPreRot,zPreRot);
      }else{
        mainKinematics(xH,xFB,xLR,aHip,0,0,0);
        mainKinematics(xH,xFB,xLR,bHip,0,0,0);
        mainKinematics(xH,xFB,xLR,cHip,0,0,0);
        mainKinematics(xH,xFB,xLR,dHip,0,0,0);
      }
      break;
    }
    case 2:{//FWalk
      if(payload.gyro && payload.PID){
        WalkF(yRot,zRot, true);
      }else if (payload.gyro && !payload.PID){
        WalkF(yPreRot, zPreRot, true);
      }else{
        WalkF(0,0,true);
      }
      break;
    }
    case 3:{ //Fturn
      if(payload.gyro && payload.PID){
        turn(yRot,zRot, true);
      }else if (payload.gyro && !payload.PID){
        turn(yPreRot, zPreRot, true);
      }else{
        turn(0,0,true);
      }
      break;
    }
    case 4:{ //user
      float xFB = payload.j2_y * 50;
      float xLR = payload.j2_x * 50;
      float xH = payload.j1_x * 150;
      float timee = payload.j1_y * 50;

      bool direction = true;
      if(xFB < 0 && xLR < 0){
        direction = false;  
      }

      if(payload.gyro && payload.PID){
        //user controll will be the hardest one
        // float j1_x; forward distance
        // float j1_y; time?
        // float j2_x; xLR
        // float j2_y; xFB
        //bool j1_b; turn 1 way
        //bool j2_b; turn other way
        //turning and moving can NOT mix
        if(!payload.j1_b && !payload.j2_b){
          WalkF(yRot, zRot, direction); //need to make WalkF take in values
        }else if (payload.j1_b){
          turn(yRot, zRot, true);
        }else if (payload.j2_b){
          turn(yRot, zRot, direction);
        }
      }else if (payload.gyro && !payload.PID){
        int i = 0;
      }else{
        int i = 0;
      }
    }
      

    default:
      break;
    }
  }else{
    pwm1.sleep();
    pwm.sleep();
  }
}

