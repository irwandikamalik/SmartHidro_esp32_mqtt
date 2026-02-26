#ifndef DISTANCESENSOR_H
#define DISTANCESENSOR_H

#include <Arduino.h>

// Deklarasi pin untuk JSN-SR04T
const int trigPin = 5;  // Pin untuk Trigger
const int echoPin = 18; // Pin untuk Echo

class DistanceSensor {
public:
    DistanceSensor(int trig, int echo);
    void begin();
    long measureDistance();
    int calculatePercentage(long distance);
private:
    int trigPin;
    int echoPin;
};

#endif
