#include <Servo.h>
#define DEBUG_MODE

Servo servo;
int servo_pin = 2;

bool rainable = true;
bool is_closed = true;

const int trig_pin = 4;
const int echo_pin = 5;

bool IsNear(int threshold = 25) {
    digitalWrite(trig_pin, HIGH);
    delayMicroseconds(10); // 10us
    digitalWrite(trig_pin, LOW);

    unsigned long duration = pulseIn(echo_pin, HIGH);
    unsigned long distance = duration / 58;

#ifdef DEBUG_MODE
    Serial.print(distance);
    Serial.println("cm");
#endif

    delay(100);

    if (distance <= threshold) return true;
    else return false;
}

void UpdateRainable() {
    /* if (cur_time.hour == 6 && cur_time.minute == 0) {
        while (Serial.available());
        delay(250);

        rainable = Serial.parseInt();
    }  */
#ifdef DEBUG_MODE
    rainable = true;
#endif
}

void OpenStand() {
#ifdef DEBUG_MOD
    Serial.println("OPEN");
#endif
    servo.write(0);    // 시계 회전
    delay(400);
    servo.write(90);   // 정지

    is_closed = false;
}

void CloseStand() {
#ifdef DEBUG_MOD
    Serial.println("CLOSE");
#endif
    servo.write(180); // 반시계
    delay(400);
    servo.write(90); // 정지

    is_closed = true;
}

void Umbrella(int in_a_row = 5) {
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

void setup() {
    Serial.begin(9600);

    pinMode(trig_pin, OUTPUT);
    pinMode(echo_pin, INPUT);

    servo.attach(servo_pin);
}

void loop() {
    Umbrella();
}