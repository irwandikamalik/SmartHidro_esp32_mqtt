#include "tdsSensor.h"

#define ADC_RANGE 4095
#define SAMPLE_INTERVAL 40

TDSSensor::TDSSensor(uint8_t pin, float vref) {
  _pin = pin;
  _vref = vref;
  _temperature = 25.0;
  bufferIndex = 0;
  lastSampleTime = 0;
}

void TDSSensor::begin() {

  analogReadResolution(12);
  analogSetPinAttenuation(_pin, ADC_11db);
}

void TDSSensor::setTemperature(float temp) {
  _temperature = temp;
}

void TDSSensor::update() {

  unsigned long now = millis();

  if (now - lastSampleTime >= SAMPLE_INTERVAL) {

    lastSampleTime = now;

    buffer[bufferIndex] = analogRead(_pin);
    bufferIndex++;

    if (bufferIndex >= SAMPLE_COUNT)
      bufferIndex = 0;

    int median = getMedian(buffer, SAMPLE_COUNT);

    voltage = median * _vref / ADC_RANGE;

    float ecValue = (133.42 * pow(voltage, 3)
                    -255.86 * pow(voltage, 2)
                    +857.39 * voltage);

    float ec25 = ecValue / (1.0 + 0.02 * (_temperature - 25.0));

    ec = ec25;
    tds = ec25 * 0.5;
  }
}

float TDSSensor::getVoltage() {
  return voltage;
}

float TDSSensor::getEC() {
  return ec;
}

float TDSSensor::getTDS() {
  return tds;
}

int TDSSensor::getMedian(int *array, int len) {

  int temp[len];

  for (int i = 0; i < len; i++)
    temp[i] = array[i];

  for (int j = 0; j < len - 1; j++) {
    for (int i = 0; i < len - j - 1; i++) {

      if (temp[i] > temp[i + 1]) {
        int t = temp[i];
        temp[i] = temp[i + 1];
        temp[i + 1] = t;
      }
    }
  }

  if (len % 2)
    return temp[(len - 1) / 2];
  else
    return (temp[len / 2] + temp[len / 2 - 1]) / 2;
}