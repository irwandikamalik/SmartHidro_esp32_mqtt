#ifndef PH_SENSOR_H
#define PH_SENSOR_H

#include <Arduino.h>

class phSensor {
  public:
    phSensor(uint8_t pin);

    void begin();
    void setCalibration(float slope, float offset);
    void setTemperature(float tempC);

    void update();          // panggil setiap loop (non blocking)
    float getPH();
    float getVoltage();

  private:
    uint8_t _pin;

    float _slope;
    float _offset;
    float _temperature;

    static const uint8_t bufferSize = 20;
    float buffer[bufferSize];
    uint8_t index;

    unsigned long lastSample;
    const uint16_t sampleInterval = 20; // ms

    float filteredVoltage;

    float movingAverage();
};

#endif
