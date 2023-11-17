#include <Arduino.h>
#include <EEPROM.h>

void writeStringEEPROM(String str, int addr) {
  int str_length = str.length();
  for (int i = 0; i < str_length; i++) {
    EEPROM.write(addr + i, str[i]);
  }
  EEPROM.write(addr + str_length, '\0');
  EEPROM.commit();
}

String readStringEEPROM(int addr) {
  String str = "";
  char character = EEPROM.read(addr);
  int i = 0;
  while (character != '\0' && i < 100) {
    str += character;
    i++;
    character = EEPROM.read(addr + i);
  }
  return str;
}