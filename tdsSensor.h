#ifndef TDSSENSOR_H
#define TDSSENSOR_H

#include <Arduino.h>

class TdsSensor {
  public:
    TdsSensor(int tdsPin, int numSamples, float ecCalibration, float tempCoefficient);
    float readSensorValue();
    float calculateVoltage(float averageSensorValue);
    float calculateEC(float voltage);
    float compensateECForTemperature(float ecValue, float temperature);
    float calculateTDS(float ecTemperatureCompensated);
    float getTDS();

  private:
    int _tdsPin;
    int _numSamples;
    float _ecCalibration;
    float _tempCoefficient;
    float _voltage;
    float _ecValue;
    float _tdsValue;
};

#endif