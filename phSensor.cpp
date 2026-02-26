#include "phSensor.h"

phSensor::phSensor(uint8_t pin) {
  _pin = pin;
  _slope = -5.70;
  _offset = 23.85;
  _temperature = 25.0;
  index = 0;
  lastSample = 0;
  filteredVoltage = 0;
}

void phSensor::begin() {
  pinMode(_pin, INPUT);
  analogSetPinAttenuation(_pin, ADC_11db);
  analogSetWidth(12);

  for (int i = 0; i < bufferSize; i++)
    buffer[i] = 0;
}

void phSensor::setCalibration(float slope, float offset) {
  _slope = slope;
  _offset = offset;
}

void phSensor::setTemperature(float tempC) {
  _temperature = tempC;
}

void phSensor::update() {

  if (millis() - lastSample >= sampleInterval) {

    lastSample += sampleInterval;

    float voltage = analogRead(_pin) * 3.3 / 4095.0;

    buffer[index] = voltage;
    index++;

    if (index >= bufferSize)
      index = 0;

    filteredVoltage = movingAverage();
  }
}

float phSensor::movingAverage() {

  float sum = 0;

  for (int i = 0; i < bufferSize; i++)
    sum += buffer[i];

  return sum / bufferSize;
}

float phSensor::getVoltage() {
  return filteredVoltage;
}

float phSensor::getPH() {

  // Temperature compensation (industrial approach)
  float tempFactor = 1 + 0.03 * (_temperature - 25.0);
  float compensatedSlope = _slope * tempFactor;

  return compensatedSlope * filteredVoltage + _offset;
}
