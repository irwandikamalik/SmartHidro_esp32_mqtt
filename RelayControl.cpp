#include "RelayControl.h"

RelayControl::RelayControl(int relayPin) {
  pin = relayPin;
  state = false;
}

void RelayControl::begin() {
  pinMode(pin, OUTPUT);
  off();  // Pastikan relay dalam keadaan mati saat memulai
}

void RelayControl::on() {
  digitalWrite(pin, LOW);
  state = true;
}

void RelayControl::off() {
  digitalWrite(pin, HIGH);
  state = false;
}

void RelayControl::setState(bool newState) {
  if (newState) {
    on();
  } else {
    off();
  }
}

bool RelayControl::getState() {
  return state;
}
