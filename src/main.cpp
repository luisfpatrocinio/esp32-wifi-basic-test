#include <Arduino.h>

#define LED_BUILTIN 2
#define BUTTON_PIN 26
#define BUZZER_PIN 27
#define BAUDRATE 115200

int buttonState = 0;

void setup()
{
    Serial.begin(BAUDRATE);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    ledcAttachPin(BUZZER_PIN, 0);
}

void loop()
{
    buttonState = digitalRead(BUTTON_PIN);
    Serial.println(buttonState);

    digitalWrite(LED_BUILTIN, !buttonState);

    if (buttonState == LOW)
    {
        int _freq = sin(millis() / 100.0) * 500 + 1000;
        ledcWriteTone(0, _freq);
    }
    else
    {
        ledcWriteTone(0, 0);
    }

    delay(100);
}
