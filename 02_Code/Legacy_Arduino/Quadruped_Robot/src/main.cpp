#include <allIncludes.h>

#define CONTROLLER
#define CONTROLLERA




singleCycle parseData(String input);

//------------------------------------------------------------------------------------------------
//angle goal values
double xAngle = 0;
double yAngle = 0;
double zAngle = 0;

double xAngleV = 0;
double yAngleV = 0;
double zAngleV = 0;


//------------------------------------------------------------------------------------------------
//angle variables
double yPreRot= 0, zPreRot = 0;
double yRot= 0, zRot = 0;


//------------------------------------------------------------------------------------------------
//PID setup for gyro
double Kp= .25, Ki=.01, Kd =0;
double angleGoal = 0;


//------------------------------------------------------------------------------------------------
//motor driver setup
#define SERVO_FREQ 50
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x60);
Adafruit_PWMServoDriver pwm1 = Adafruit_PWMServoDriver(0x7C);


//------------------------------------------------------------------------------------------------
//location variables
Cords aCords;
Cords bCords;
Cords cCords;
Cords dCords;



Cords AcurrentPosition;
Cords BcurrentPosition;
Cords CcurrentPosition;
Cords DcurrentPosition;

//------------------------------------------------------------------------------------------------
// Motor, Leg, kinematics, interpolation set up
//float LlimitV, float HlimitV, bool directionV, float offsetV)
singleCycle activeOffsets;

motor aHipM(&pwm, aHip, 45, 135, true, &(activeOffsets.legPositions[0]));
motor aKneeM(&pwm, aKnee, 0, 180, false, &(activeOffsets.legPositions[1]));
motor aAnkleM(&pwm, aAnkle, 0, 180, true, &(activeOffsets.legPositions[2]));

motor bHipM(&pwm, bHip, 45, 135, false, &(activeOffsets.legPositions[3]));
motor bKneeM(&pwm, bKnee, 0, 180, true, &(activeOffsets.legPositions[4]));
motor bAnkleM(&pwm, bAnkle, 0, 180, false, &(activeOffsets.legPositions[5]));

motor cHipM(&pwm1, cHip, 45, 135, false, &(activeOffsets.legPositions[6]));
motor cKneeM(&pwm1, cKnee, 0, 180, false, &(activeOffsets.legPositions[7]));
motor cAnkleM(&pwm1, cAnkle, 0, 180, true, &(activeOffsets.legPositions[8]));

motor dHipM(&pwm1, dHip, 45, 135, true, &(activeOffsets.legPositions[9]));
motor dKneeM(&pwm1, dKnee, 0, 180, true, &(activeOffsets.legPositions[10]));
motor dAnkleM(&pwm1, dAnkle, 0, 180, false, &(activeOffsets.legPositions[11]));


//------------------------------------------------------------------------------------------------
//each leg object
leg Aleg(&aHipM, &aKneeM, &aAnkleM, 'A');
leg Bleg(&bHipM, &bKneeM, &bAnkleM, 'B');
leg Cleg(&cHipM, &cKneeM, &cAnkleM, 'C');
leg Dleg(&dHipM, &dKneeM, &dAnkleM, 'D');


//------------------------------------------------------------------------------------------------
//kinematic objects
kinematics AlegK(&Aleg);
kinematics BlegK(&Bleg);
kinematics ClegK(&Cleg);
kinematics DlegK(&Dleg);


//------------------------------------------------------------------------------------------------
//Ramp(interpolation) objects
rampLeg aLegR(aHip);
rampLeg bLegR(bHip);
rampLeg cLegR(cHip);
rampLeg dLegR(dHip);


//------------------------------------------------------------------------------------------------
//gyro
#define BNO055_SAMPLERATE_DELAY_MS (100)
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28, &Wire);
sensors_event_t event;


//------------------------------------------------------------------------------------------------
//PID creation
PID yPID(&yPreRot, &yRot, &angleGoal, Kp, Ki, Kd, DIRECT);
PID zPID(&zPreRot, &zRot, &angleGoal, Kp, Ki, Kd, DIRECT);


//------------------------------------------------------------------------------------------------
//radio
PayloadStruct payload;
//RF24 radio(ce, csn)
RF24 radio(14,13);
bool newData = false;
#ifdef CONTROLLER
int oldState = 0;
const byte thisSlaveAddress[5] = {'R','x','A','A','A'};
#endif

//CE = 12, 
//CSN = 18
//sck = 13
//MISO = 17
//MOSI = 8
//------------------------------------------------------------------------------------------------
//more (different) location variables
Cords basicStand;

float cycleTime = 50;
float moveBackDistance = -50;
float moveUpDistance =-30;
//AxH,AxLR,AxFB,BxH,BxLR,BxFB,CxH,CxLR,CxFB,DxH,DxLR,DxFB
singleCycle walking0(0,0,moveBackDistance/3, // moving back
                    moveUpDistance,0,2*moveBackDistance/3, //returning to 0
                    moveUpDistance,0,2*moveBackDistance/3, //returning to 0
                    0,0,moveBackDistance/3); // moving back


singleCycle walking1(0,0,2*moveBackDistance/3, // moving back
                    moveUpDistance,0,moveBackDistance/3, //returning to 0
                    moveUpDistance,0,moveBackDistance/3,+//returning to 0
                    0,0,2*moveBackDistance/3); // moving back


singleCycle walking2(0,0,moveBackDistance, //moved back
                    0,0,0, //back at 0
                    0,0,0, //back at 0
                    0,0,moveBackDistance); //moved back


singleCycle walking3(moveUpDistance,0,2*moveBackDistance/3, //returning to 0
                    0,0,moveBackDistance/3, //moving back
                    0,0,moveBackDistance/3, //moving back
                    moveUpDistance,0,2*moveBackDistance/3); //returning to 0

singleCycle walking4(moveUpDistance,0,moveBackDistance/3, //returning to 0
                    0,0,2*moveBackDistance/3, // moving back
                    0,0,2*moveBackDistance/3, // moving back
                    moveUpDistance,0,moveBackDistance/3); //returning to 0

singleCycle walking5(0,0,0, //back at 0
                    0,0,moveBackDistance, //moved back
                    0,0,moveBackDistance, //moved back
                    0,0,0); //back at 0




//switch to size of instead of a strict n value
singleCycle walking[] = {walking0,walking1,walking2,walking3,walking4,walking5};
movementCycles walkForward(6, false,true,1000,walking);


cycleControl walkForwardCycle(&walkForward, 
                              &aLegR,&bLegR,&cLegR,&dLegR,
                              &AlegK,&BlegK,&ClegK,&DlegK,
                              &aCords,&bCords,&cCords,&dCords);


singleCycle recievedPositions;

void setup() {
  //start busses
  //start Serial
  Serial.begin(115200);
  Serial.println("Serial Active");


  //------------------------------------------------------------------------------------------------
  //Start I2C
  Wire.begin(04,05); //SDA, SCL
  Serial.println("IC2 Active");

  //------------------------------------------------------------------------------------------------
  //start SPI bus for NRF2401
  #ifdef CONTROLLER
    //SPI.begin(SCK, MISO, MOSI);
    SPI.begin(12, 10, 11);
    //SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));

  #endif

  //------------------------------------------------------------------------------------------------
  //enable the controller
  #ifdef CONTROLLER
  //start radio
    radio.begin();
    radio.setPALevel(RF24_PA_MAX);
    radio.setDataRate(RF24_250KBPS);
    radio.openReadingPipe(1, thisSlaveAddress);
    radio.startListening();
    Serial.println("radio Active");
  #endif


  applyBaseOffsets();


  // //------------------------------------------------------------------------------------------------
  // //Set up PCA9685s / motor drivers
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);  // Analog servos run at ~50 Hz updates
  delay(10);  
  pwm1.begin();
  pwm1.setOscillatorFrequency(27000000);
  pwm1.setPWMFreq(SERVO_FREQ);  // Analog servos run at ~50 Hz updates


  // //------------------------------------------------------------------------------------------------
  // start gyro
  bno.begin();
 
  Serial.println("gyro Started");

  // if(!bno.begin()){
  //   Serial.print("Gyro Error");
  //   while(1);
  // }
  bno.setExtCrystalUse(true);
  delay(1000); 
  Serial.println("Gyro Active");


  // //------------------------------------------------------------------------------------------------
  // //limit PID values to keep motors in safe range
  yPID.SetOutputLimits(-20.0,20.0);
  zPID.SetOutputLimits(-20.0,20.0);

  //set the pid mode
  yPID.SetMode(AUTOMATIC);
  zPID.SetMode(AUTOMATIC);
  Serial.println("PID Active");


  // //------------------------------------------------------------------------------------------------
  //where in the walk cycle the robot is at. Way to share state across all movements
  //setCycle();

  //set legs to stand in some default form
  basicStand.xH = 100;
  basicStand.xLR = 0;
  basicStand.xFB = 0;

  AcurrentPosition.xH=130;
  BcurrentPosition.xH=130;
  CcurrentPosition.xH=130;
  DcurrentPosition.xH=130;
  standing_0();

  // Sets all RAMPS to go to 0 in all joints in all legs
  //resetAll();

}

void loop() {
  bno.getEvent(&event);
  aCords.yRot = event.orientation.y;
  bCords.yRot = event.orientation.y;
  cCords.yRot = event.orientation.y;
  dCords.yRot = event.orientation.y;

  aCords.zRot = event.orientation.z;
  bCords.zRot = event.orientation.z;
  cCords.zRot = event.orientation.z;
  dCords.zRot = event.orientation.z;
  walkForwardCycle.continueCycle(AcurrentPosition,BcurrentPosition,CcurrentPosition,DcurrentPosition);
  // Serial.print("A: ");
  // Serial.print(aCords.xH);
  // Serial.print(" D: ");
  // Serial.print(dCords.xH);

  // Serial.print("B: ");
  // Serial.print(bCords.xH);
  // Serial.print(" C: ");
  // Serial.println(cCords.xH);


// if (Serial.available() > 0) {
//                 // Read the incoming data into a string
//                 String input = Serial.readStringUntil('\n');
//                 recievedPositions = parseData(input);
//                 //activeOffsets.setOffsets(recievedOffsets);
//                 //standing_0();
//   }
// Aleg.setAngles(recievedPositions.aHipV,recievedPositions.aKneeV,recievedPositions.aAnkleV);
// Bleg.setAngles(recievedPositions.bHipV,recievedPositions.bKneeV,recievedPositions.bAnkleV);
// Cleg.setAngles(recievedPositions.cHipV,recievedPositions.cKneeV,recievedPositions.cAnkleV);
// Dleg.setAngles(recievedPositions.dHipV,recievedPositions.dKneeV,recievedPositions.dAnkleV);
  // //Serial.print(dAnkleM.Degree);
  
  // // //standing_0();
  
  // // //bKneeM.setDegree(0);
  // // //FWalk_2(yAngleV, zAngleV);
  // // //update radio

  // getData();
  // //payload.state = 0;

  // //payload.state == 0;
  // //------------------------------------------------------------------------------------------------
  // // //update gyro
  // bno.getEvent(&event);

  // Serial.print("x: ");
  // Serial.print(event.orientation.x);
  // Serial.print(" y: ");
  // Serial.print(event.orientation.y);
  // Serial.print(" z: ");
  // Serial.println(event.orientation.z);
  // //delay(100);
  // if(payload.gyro == 1){
  //   yPreRot = event.orientation.y;
  //   zPreRot = event.orientation.z;
  //   if(payload.PID == 1){
  //     yPID.Compute();
  //     zPID.Compute();
  //     //yAngleV = yRot;
  //     //zAngleV = zRot;
  //   }else{
  //     yAngleV =  event.orientation.y;
  //     zAngleV =  event.orientation.z;
  //   }
    
  // }else{
  //   yAngleV = 0;
  //   zAngleV = 0;
  // }
  
  // // Serial.print("y: ");
  // //   Serial.print(yAngleV);
  // //   Serial.print("    z: ");
  // //   Serial.println(zAngleV);



  // //------------------------------------------------------------------------------------------------
  // #ifdef CONTROLLERA
  //   //switch modes and saftey mode
  //   if(payload.eStop != 1){
  //     wakeup_9();
  //     if(oldState !=payload.state){
  //         // aLegR.setCycle(0);
  //         // bLegR.setCycle(3);
  //         // cLegR.setCycle(3);
  //         // dLegR.setCycle(0);
  //     }
  //     switch (payload.state) {
  //       case 0:{ //standing
  //           standing_0();
  //         break;
  //       }
  //       case 1:{ //IK mode
  //           IK_1(0,yAngle,zAngle);
  //         break;
  //       }
  //       case 2:{//FWalk
  //           FWalk_2(xAngleV,yAngleV);
  //         break;
  //       }
  //       case 3:{ //
  //           FTurn_3(xAngleV,yAngleV);
  //         break;
  //       }
  //       case 4:{ //user
  //           User_4(xAngleV,yAngleV);
  //         break;
  //       } 
  //       case 5:{ //used to install new motors
  //       if (Serial.available() > 0) {
  //               // Read the incoming data into a string
  //               String input = Serial.readStringUntil('\n');
  //               positions recievedOffsets = parseData(input);
  //               activeOffsets.setOffsets(recievedOffsets);
  //               standing_0();
  //             }
  //             break;
  //       }
  //       case 6:{ //motor offset setup
  //             if (Serial.available() > 0) {
  //               // Read the incoming data into a string
  //               String input = Serial.readStringUntil('\n');
  //               positions recievedOffsets = parseData(input);
  //               activeOffsets.setOffsets(recievedOffsets);
  //               standing_0();
  //             }
  //             break;
  //          }
  //       case 7:{
  //           if (Serial.available() > 0) {
  //             // Read the incoming data into a string
  //             String input = Serial.readStringUntil('\n');
  //             positions recievedPositions = parseData(input);
  //             aHipM.setDegree(90);
  //             aKneeM.setDegree(45);
  //             aAnkleM.setDegree(0);
  //             bHipM.setDegree(90);
  //             bKneeM.setDegree(45);
  //             bAnkleM.setDegree(0);
  //             cHipM.setDegree(90);
  //             cKneeM.setDegree(45);
  //             cAnkleM.setDegree(0);
  //             dHipM.setDegree(90);
  //             dKneeM.setDegree(45);
  //             dAnkleM.setDegree(0);
  //           }
  //           break;
  //          }
  //     default:
  //         break;
  //       }
  //   }else{
  //       Default_9(); //turns off motors
  //   }
  //   oldState = payload.state;
  // #endif
}

//gets data from radio, checks if data was recieved
void getData(){
   if (radio.available()) {
    radio.read(&payload, sizeof(payload));
    newData = true;
  //  }else{
  //   payload.eStop ==true; 
  }

}



singleCycle parseData(String input) {
  singleCycle receivedValues;
  const int numFloats = 12;
  float values[numFloats];
  // Create an index for storing float values
  int index = 0;
  
  // Split the input string using a delimiter (assuming comma-separated values)
  int startIndex = 0;
  int endIndex = input.indexOf(',');

  while (endIndex != -1 && index < numFloats) {
    // Convert the substring to a float and store it in the values array
    values[index] = input.substring(startIndex, endIndex).toFloat();
    index++;

    // Update the indices for the next substring
    startIndex = endIndex + 1;
    endIndex = input.indexOf(',', startIndex);
  }

  // Handle the last value after the last comma
  if (index < numFloats) {
    values[index] = input.substring(startIndex).toFloat();  // This handles the last value in the string
  }

  receivedValues.legPositions[0] = values[0];
  receivedValues.legPositions[1] = values[1];
  receivedValues.legPositions[2] = values[2];
  receivedValues.legPositions[3] = values[3];
  receivedValues.legPositions[4] = values[4];
  receivedValues.legPositions[5] = values[5];
  receivedValues.legPositions[6] = values[6];
  receivedValues.legPositions[7] = values[7];
  receivedValues.legPositions[8] = values[8];
  receivedValues.legPositions[9] = values[9];
  receivedValues.legPositions[10] = values[10];
  receivedValues.legPositions[11] = values[11];
  return receivedValues;
}