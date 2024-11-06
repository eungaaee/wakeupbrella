#include <IRremote.h>
// #include <ESP8266WiFi.h>
#include <time.h>

int recvPin = 7;

decode_results result;

IRrecv irrecv(recvPin);

void setup() {
  Serial.begin(9600);

  irrecv.enableIRIn();
}

bool is_alarm_setting = false;
int hhmm[4], hhmm_idx;
int input_num = 5;

void setAlarm() {
  is_alarm_setting = true;
}

int result_to_int(long unsigned int res) {
  int num;
  if (res == 0xFF6897) num = 0;
  else if (res == 0xFF30CF) num = 1;
  else if (res == 0xFF18E7) num = 2;
  else if (res == 0xFF7A85) num = 3;
  else if (res == 0xFF10EF) num = 4;
  else if (res == 0xFF38C7) num = 5;
  else if (res == 0xFF5AA5) num = 6;
  else if (res == 0xFF42BD) num = 7;
  else if (res == 0xFF4AB5) num = 8;
  else if (res == 0xFF52AD) num = 9;

  return num;
}

void getRemoteInput() {
  if (irrecv.decode(&result)) {
    switch (result.value) {
      case 0xFF906F:  // EQ
        setAlarm();
        Serial.println("EQ");
        break;
      case 0xFFE01F: // -
        if (is_alarm_setting && hhmm_idx > 0)
          hhmm[--hhmm_idx] = 0;
        break;
      case 0xFF6897:  // 0
      case 0xFF30CF:  // 1
      case 0xFF18E7:  // 2
      case 0xFF7A85:  // 3
      case 0xFF10EF:  // 4
      case 0xFF38C7:  // 5
      case 0xFF5AA5:  // 6
      case 0xFF42BD:  // 7
      case 0xFF4AB5:  // 8
      case 0xFF52AD:  // 9
        input_num = result_to_int(result.value);
        if (is_alarm_setting && hhmm_idx < 4)
          hhmm[hhmm_idx++] = input_num; 
        break;
      default:
        break;
    }

    for (int i = 0; i < 4; i++) Serial.print(hhmm[i]);
    Serial.println();

    delay(250);
    irrecv.resume();
  }
}

void loop() {
  getRemoteInput();
}
