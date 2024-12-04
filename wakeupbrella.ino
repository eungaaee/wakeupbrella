#include <Servo.h>
#define ALARM_DEBUG

// Both
struct hour_minute {
    int hour;
    int minute;
};

struct hour_minute clock = {-1, -1};
struct hour_minute alarm_time = {0, 0};

void UpdateTime() {
    static unsigned long elapsed_time;

    if (clock.hour == -1 && clock.minute == -1) {  // 처음 시간은 시리얼 통신으로 받아오기
        if (!Serial.available()) return;
        delay(1000);  // * 중요: 모두 전송될 때 까지 기다리기

        clock.hour = Serial.parseInt();
        clock.minute = Serial.parseInt();

#ifdef ALARM_DEBUG
    if (clock.hour < 10) Serial.print('0');
    Serial.print(clock.hour);
    Serial.print(':');
    if (clock.minute < 10) Serial.print('0');
    Serial.print(clock.minute);
#endif
    } else {
        if (millis() - elapsed_time >= 60000) {  // 1분이 지날 때마다 시간 업데이트
            clock.minute++;
            if (clock.minute == 60) {
                clock.minute = 0;
                clock.hour++;
                if (clock.hour == 24)
                    clock.hour = 0;
            }
            elapsed_time = millis();
        }
    }

    delay(100);
}

// Umbrella
Servo servo;
int servo_pin = 2;

bool rainable = false;
bool is_closed = true;

const int trig_pin = 4;
const int echo_pin = 5;

bool IsNear(int threshold = 25) {
    digitalWrite(trig_pin, HIGH);
    delayMicroseconds(10);  // 10us
    digitalWrite(trig_pin, LOW);

    unsigned long duration = pulseIn(echo_pin, HIGH);
    unsigned long distance = duration / 58;

#ifdef UMBRELLA_DEBUG
    Serial.print(distance);
    Serial.println("cm");
#endif

    delay(100);

    if (distance <= threshold) return true;
    else return false;
}

void UpdateRainable() {
    if (clock.hour == 6 && clock.minute == 0) {
        if (!Serial.available()) return;
        delay(1000);  // * 중요: 모두 전송될 때 까지 기다리기

        if (Serial.parseInt() == 1) rainable = true;
        else rainable = false;
    }

#ifdef UMBRELLA_DEBUG
    rainable = true;
#endif
}

void OpenStand() {
#ifdef DEBUG_MOD
    Serial.println("OPEN");
#endif
    servo.write(0);  // 시계 회전
    delay(400);
    servo.write(90);  // 정지

    is_closed = false;
}

void CloseStand() {
#ifdef DEBUG_MOD
    Serial.println("CLOSE");
#endif
    servo.write(180);  // 반시계
    delay(400);
    servo.write(90);  // 정지

    is_closed = true;
}

void Umbrella(const int in_a_row = 5) {
    static int near_count, far_count;

    if (IsNear(25)) {
        near_count++;
        far_count = 0;
    } else {
        far_count++;
        near_count = 0;
    }

    if (rainable && is_closed && near_count >= in_a_row) {
        near_count = 0;
        OpenStand();
    }

    if (!is_closed && far_count >= in_a_row) {
        far_count = 0;
        CloseStand();
    }
}

// Alarm
const int buzzer_pin = 12;
const int waterpump_A_pin = 10, waterpump_B_pin = 6;
const int decrease_button_pin = 7, increase_button_pin = 8, confirm_button_pin = 9;
const int pressure_pin = A5;

bool alarm_setting = false, alarm_set = false;
bool alarmed = false;

void Water() {
    // Hydro-pump
    digitalWrite(waterpump_A_pin, HIGH);
    digitalWrite(waterpump_B_pin, LOW);

    /* // Stop
      digitalWrite(AA, LOW);
      digitalWrite(AB, LOW); */
}

void SetAlarm() {
#ifdef ALARM_DEBUG
    Serial.print(alarm_time.hour);
    Serial.print(':');
    Serial.println(alarm_time.minute);
#endif
    bool decrease_button_status = digitalRead(decrease_button_pin);
    bool increase_button_status = digitalRead(increase_button_pin);
    bool confirm_button_status = digitalRead(confirm_button_pin);

    static bool hour_set, minute_set; // 0

    if (alarm_set && confirm_button_status) {
        alarm_set = false;
        return;
    } else if (!alarm_setting && confirm_button_status) {
        alarm_setting = true;
        hour_set = false;
        minute_set = false;
        return;
    }

    if (alarm_setting) {
        if (!hour_set) {
            if (decrease_button_status) {
                alarm_time.hour--;
                if (alarm_time.hour < 0) {
                    alarm_time.hour = 23;
                }
                // delay(100);  // Debounce delay
            }

            if (increase_button_status) {
                alarm_time.hour++;
                if (alarm_time.hour >= 24) {
                    alarm_time.hour = 0;
                }
                // delay(100);
            }

            if (confirm_button_status) {
                Serial.println("시간 설정 확인");
                hour_set = true;
                // delay(100);
            }
        } else if (!minute_set) {
            if (decrease_button_status) {
                alarm_time.minute -= 10;
                if (alarm_time.minute < 0) {
                    alarm_time.minute = 50;
                }
                // delay(100);  // Debounce delay
            }

            if (increase_button_status) {
                alarm_time.minute += 10;
                if (alarm_time.minute >= 60) {
                    alarm_time.minute = 0;
                }
                // delay(100);
            }

            if (confirm_button_status) {
                Serial.println("분 설정 확인");
                minute_set = true;
                alarm_setting = false;
                alarm_set = true;
                // delay(100);
            }
        }
    }
}

bool IsSleeping(int threshold = 500) {
    if (analogRead(pressure_pin) <= threshold) return true;
    else return false;
}

void Alarm() {
    static unsigned long rang_time;

    if (!alarmed && clock.hour == alarm_time.hour && clock.minute == alarm_time.minute) {
        tone(buzzer_pin, 261);
        rang_time = millis();
        alarmed = true;
    }

    if (alarmed && (millis() - rang_time >= 30000) && IsSleeping()) Water();
}

void setup() {
    Serial.begin(9600);

    // Umbrella
    pinMode(trig_pin, OUTPUT);
    pinMode(echo_pin, INPUT);

    servo.attach(servo_pin);

    // Alarm
    pinMode(waterpump_A_pin, OUTPUT);
    pinMode(waterpump_B_pin, OUTPUT);

    pinMode(decrease_button_pin, INPUT_PULLUP);
    pinMode(increase_button_pin, INPUT_PULLUP);
    pinMode(confirm_button_pin, INPUT_PULLUP);

    pinMode(pressure_pin, INPUT);
}

void loop() {
    UpdateTime();
    
    SetAlarm();
    Alarm();

    UpdateRainable();
    Umbrella();
    
    delay(100);
}