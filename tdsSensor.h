#ifndef TDS_SENSOR_H
#define TDS_SENSOR_H

#include <Arduino.h>

class TDSSensor {

  public:

    TDSSensor(uint8_t pin, float vref = 3.3);

    void begin();
    void update();

    float getVoltage();
    float getEC();
    float getTDS();

    void setTemperature(float temp);

  private:

    uint8_t _pin;
    float _vref;
    float _temperature;

    static const int SAMPLE_COUNT = 15;
    int buffer[SAMPLE_COUNT];
    int bufferIndex;

    unsigned long lastSampleTime;

    int getMedian(int *array, int len);

    float voltage;
    float ec;
    float tds;
};

#endif