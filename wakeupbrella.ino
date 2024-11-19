// 아두이노->컴퓨터로 시리얼 통신 할 때 Serial.println() 사용하면 오류 발생. Serial.print() 사용하기
#include <IRremote.h>
#include <time.h>
#define DEBUG_MODE

// remote
int recvPin = 7;
decode_results result;
IRrecv irrecv(recvPin);

// alarm
struct {
    int hour;
    int minute;
} cur_time = {-1, -1};
int hhmm[4], hhmm_idx;
bool is_alarm_setting = false;
bool is_alarm_set = false;

// water pump
int AA = 10;
int AB = 9;

// test led
int led_pin = 8;

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

void set_alarm() {
    if (irrecv.decode(&result)) {
        switch (result.value) {
            case 0xFF906F:  // EQ
                is_alarm_setting = !is_alarm_setting;
                break;
            case 0xFFE01F:  // -
                if (is_alarm_setting && hhmm_idx > 0)
                    hhmm[--hhmm_idx] = 0;
                break;
            case 0xFFC23D:  // Play
                is_alarm_setting = !is_alarm_setting;
                is_alarm_set = !is_alarm_set;
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
                if (is_alarm_setting && hhmm_idx < 4) {
                    int input_num = result_to_int(result.value);
                    hhmm[hhmm_idx++] = input_num;
                }
                break;
            default:
                break;
        }

        /* for (int i = 0; i < 4; i++) Serial.print(hhmm[i]);
        Serial.println(); */

        delay(250);
        irrecv.resume();
    }
}

void update_time() {
    static unsigned long elapsed_time;

    if (cur_time.hour == -1 && cur_time.minute == -1) {
        while (!Serial.available());
        delay(250);  // 모든 바이트가 전송되었을 때 까지 기다리기

        /* String raw = Serial.readString();
        cur_time.hour = raw.substring(0, 2).toInt();
        cur_time.minute = raw.substring(2, 4).toInt(); */
        cur_time.hour = Serial.parseInt();
        cur_time.minute = Serial.parseInt();

        #ifdef DEBUG_MODE
            if (cur_time.hour < 10) Serial.print('0');
            Serial.print(cur_time.hour);
            Serial.print(':');
            if (cur_time.minute < 10) Serial.print('0');
            Serial.println(cur_time.minute);
        #endif
    } else {
        if (millis() - elapsed_time >= 60000) {
            cur_time.minute++;
            if (cur_time.minute == 60) {
                cur_time.minute = 0;
                cur_time.hour++;
                if (cur_time.hour == 24)
                    cur_time.hour = 0;
            }
            elapsed_time = millis();
        }
    }
}

bool check_alarm() {
}

void water() {
    digitalWrite(AA, HIGH);
    digitalWrite(AB, LOW);
    delay(3000);
    digitalWrite(AA, LOW);
    digitalWrite(AB, LOW);
    delay(3000);
}

void setup() {
    Serial.begin(9600);
    Serial.setTimeout(1);

    irrecv = IRrecv(recvPin);
    irrecv.enableIRIn();

    pinMode(AA, OUTPUT);
    pinMode(AB, OUTPUT);

    pinMode(led_pin, OUTPUT);
}

void loop() {
    update_time();

    set_alarm();
    if (is_alarm_set) check_alarm();
}