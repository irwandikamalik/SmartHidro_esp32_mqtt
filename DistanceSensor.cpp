#include "DistanceSensor.h"

DistanceSensor::DistanceSensor(int trig, int echo) {
    trigPin = trig;
    echoPin = echo;
}

void DistanceSensor::begin() {
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
}

long DistanceSensor::measureDistance() {
    // Mengirimkan trigger
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    // Membaca durasi waktu echo
    long duration = pulseIn(echoPin, HIGH, 30000);

    // Menghitung jarak (cm)
    long distance = duration * 0.0344 / 2;

    return distance;
}

int DistanceSensor::calculatePercentage(long distance) {
    int percentage = 100;

    if (distance >= 20 && distance <= 50) {
        // Hitung persentase berdasarkan jarak antara 20 hingga 50 cm
        percentage = map(distance, 20, 50, 100, 0);
    } else if (distance > 50) {
        // Jika jarak lebih dari 50 cm, tetap 100%
        percentage = 0;
    } else {
        // Jika jarak kurang dari 20 cm, anggap persentase tetap 100%
        percentage = 100;
    }

    return percentage;
}
