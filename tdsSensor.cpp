#include "tdsSensor.h"

// Constructor
TdsSensor::TdsSensor(int tdsPin, int numSamples, float ecCalibration, float tempCoefficient) {
  _tdsPin = tdsPin;
  _numSamples = numSamples;
  _ecCalibration = ecCalibration;
  _tempCoefficient = tempCoefficient;
  _voltage = 0.0;
  _ecValue = 0.0;
  _tdsValue = 0.0;
}

float TdsSensor::readSensorValue() {
  long total = 0;
  for (int i = 0; i < _numSamples; i++) {
    total += analogRead(_tdsPin);  // Membaca nilai dari sensor
    delay(10);  // Delay kecil untuk stabilitas pembacaan
  }
  return total / _numSamples;  // Mengembalikan rata-rata pembacaan sensor
}

float TdsSensor::calculateVoltage(float averageSensorValue) {
  return (averageSensorValue / 4095.0) * 3.3;  // 3.3V adalah tegangan referensi ESP32
}

float TdsSensor::calculateEC(float voltage) {
  _ecValue = _ecCalibration * voltage;  // Menggunakan kalibrasi untuk menghitung EC
  return _ecValue;
}

float TdsSensor::compensateECForTemperature(float ecValue, float temperature) {
  return ecValue * (1 + _tempCoefficient * (temperature - 25));  // Temp kompensasi pada 25°C
}

float TdsSensor::calculateTDS(float ecTemperatureCompensated) {
  _tdsValue = ecTemperatureCompensated * 500;  // Menggunakan konversi umum dari EC ke TDS (ppm)
  return _tdsValue;
}

float TdsSensor::getTDS() {
  return _tdsValue;
}