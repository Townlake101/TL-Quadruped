#include <Arduino.h>

void mainKinematics(float xH , float xFB, float xLR, int hipMotor, float xRot, float yRot, float zRot);

float pytherm(float sidea, float sideb); //returns hypotenuse c
float raddec(float rad); //radians to degress
float loc(float a, float b, float c); // law of cosines, returns angle c in degress 
float pythermhypt(float sidea, float sidec); //returns side b
float decrad(float deg); //degrees to radians