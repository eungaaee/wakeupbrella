#include <Servo.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#define PREVIEW_MODE

// Both
struct hour_minute {
    int hour;
    int minute;
};

struct hour_minute clock = {-1, -1};
struct hour_minute alarm_time = {0, 0};

enum ALARM_STATUS { NOT_SET, SETTING_HOUR, SETTING_MINUTE, ALARM_SET, CLOCK_UPDATED };
enum ALARM_STATUS status;
bool display_updated = false;
bool rainable = false;

void ChangeStatus(enum ALARM_STATUS new_status) {
    if (new_status != CLOCK_UPDATED) status = new_status;
    display_updated = true;
}

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
        ChangeStatus(CLOCK_UPDATED);
    } else if (millis() - elapsed_time >= 60000) {  // 1분이 지날 때마다 시간 업데이트
        clock.minute++;
        if (clock.minute == 60) {
            clock.minute = 0;
            clock.hour++;
            if (clock.hour == 24)
                clock.hour = 0;
        }
        elapsed_time = millis();

        ChangeStatus(CLOCK_UPDATED);
    }

    // delay(100);
}

// Umbrella
Servo servo;
int servo_pin = 2;

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

#if defined(UMBRELLA_DEBUG) || defined(PREVIEW_MODE)
    rainable = true;
#endif
}

void OpenStand() {
#ifdef UMBRELLA_DEBUG
    Serial.println("OPEN");
#endif
    servo.write(0);
    is_closed = false;
}

void CloseStand() {
#ifdef UMBRELLA_DEBUG
    Serial.println("CLOSE");
#endif
    servo.write(180);
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
const int decrease_button_pin = 8, increase_button_pin = 7, confirm_button_pin = 9, cancel_button_pin = 13;
const int pressure_pin = A3;

LiquidCrystal_I2C lcd(0x27, 20, 4); // 0x3F 또는 0x27

bool alarm_setting = false, alarm_set = false;
bool alarming = false;
bool watered = false;

void Water(bool on_off) {
    if (on_off == true) { // Hydro-pump
        digitalWrite(waterpump_A_pin, HIGH);
        digitalWrite(waterpump_B_pin, LOW);
    } else { // Stop
        digitalWrite(waterpump_A_pin, LOW);
        digitalWrite(waterpump_B_pin, LOW);
    }
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
        ChangeStatus(NOT_SET);
        return;
    } else if (!alarm_setting && confirm_button_status) {
        alarm_setting = true;
        hour_set = false;
        minute_set = false;
        ChangeStatus(SETTING_HOUR);
        return;
    }

    if (alarm_setting) {
        if (!hour_set) {
            if (decrease_button_status) {
                alarm_time.hour--;
                if (alarm_time.hour < 0) {
                    alarm_time.hour = 23;
                }
                ChangeStatus(SETTING_HOUR);
            }

            if (increase_button_status) {
                alarm_time.hour++;
                if (alarm_time.hour >= 24) {
                    alarm_time.hour = 0;
                }
                ChangeStatus(SETTING_HOUR);
            }

            if (confirm_button_status) {
                hour_set = true;
                ChangeStatus(SETTING_MINUTE);
            }
        } else if (!minute_set) {
            if (decrease_button_status) {
                alarm_time.minute--;
                if (alarm_time.minute < 0) {
                    alarm_time.minute = 59;
                }
                ChangeStatus(SETTING_MINUTE);
            }

            if (increase_button_status) {
                alarm_time.minute++;
                if (alarm_time.minute >= 60) {
                    alarm_time.minute = 0;
                }
                ChangeStatus(SETTING_MINUTE);
            }

            if (confirm_button_status) {
                minute_set = true;
                alarm_setting = false;
                alarm_set = true;
                ChangeStatus(ALARM_SET);
            }
        }
    }
}

bool IsSleeping(int threshold = 900) {
    // Serial.println(analogRead(pressure_pin));
    if (analogRead(pressure_pin) <= threshold) return true;
    else return false;
}

void Alarm() {
    static unsigned long rang_time;

    if (alarm_set && !alarming && clock.hour == alarm_time.hour && clock.minute == alarm_time.minute) {
        tone(buzzer_pin, 261);
        rang_time = millis();
        alarming = true;
    }

    if (alarming && !watered && IsSleeping() && millis() - rang_time >= 5000) {
        Water(true);
        watered = true;
    }

    bool cancel_button = digitalRead(cancel_button_pin);

    if (((watered && alarming && !IsSleeping()) || (!watered && alarming)) && cancel_button) {
        noTone(buzzer_pin);
        Water(false);
        alarming = false;
        alarm_set = false;
        watered = false;
        ChangeStatus(NOT_SET);
    }
}

void UpdateDisplay() {
    if (!display_updated) return;
    display_updated = false;
    lcd.clear();

    // show current time
    lcd.setCursor(0, 0);
    lcd.print("Clock: ");
    if (clock.hour < 10) lcd.print('0');
    lcd.print(clock.hour);
    lcd.print(':');
    if (clock.minute < 10) lcd.print('0');
    lcd.print(clock.minute);

    // show alarm time
    lcd.setCursor(0, 1);
    lcd.print("Alarm: ");
    if (alarm_time.hour < 10) lcd.print('0');
    lcd.print(alarm_time.hour);
    lcd.print(':');
    if (alarm_time.minute < 10) lcd.print('0');
    lcd.print(alarm_time.minute);

    // show status
    lcd.setCursor(0, 3);
    lcd.print("Status: ");
    if (status == NOT_SET) lcd.print("Not Set");
    else if (status == SETTING_HOUR) lcd.print("Hour");
    else if (status == SETTING_MINUTE) lcd.print("Minute");
    else if (status == ALARM_SET) lcd.print("Alarm Set");

    // show weather
    lcd.setCursor(19, 0);
    if (rainable) lcd.print("R"); // rainy
    else lcd.print("S"); // sunny
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

    lcd.init();
    lcd.backlight();

    ChangeStatus(NOT_SET);
}

void loop() {
    UpdateTime();
    
    SetAlarm();
    Alarm();

    UpdateRainable();
    Umbrella();

    UpdateDisplay();

    IsSleeping();

    delay(100);
}