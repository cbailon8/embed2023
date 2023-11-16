# ***Embedded Systems Project***
## ***House Monitoring and Control***

- [***Embedded Systems Project***](#embedded-systems-project)
  - [***House Monitoring and Control***](#house-monitoring-and-control)
    - [**Components**](#components)
    - [**Functions**](#functions)

### **Components**
- Membrane Keyboard 4x4
- DC Motors x2
- Buzzer
- Ultrasonic Sensor HC-SR04 x2
- Temperature Sensor DS18B20 x1
- ~~AC Dimmer Module~~
- ~~Incandescent Bulbs~~ Leds
- ~~Light Sensor BH1750~~

### **Functions**
**read_temp**: Uses the DS18B20 temperature sensor to measure the current temperature in °C.

**read_key**: Uses the membrane keypad to obtain the values of the input characters and puts together the input password string.

**access_granted**: Plays a tune indicating a correct password input, and opens the door.

**access_denied**: Plays a tune indicating an incorrect password input, and triggers an alarm flag.

**read_distance**: Uses the ultrasonic sensors to measure the distance between them and any object in their range.

**oled**: Handles the oled display in real time.

**check_pwd**: Compares the input password to the user password saved in memory, granting or denying access.