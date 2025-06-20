#include "buzzer.h"
#include <Arduino.h>

// Função auxiliar para gerar o som, não exposta no .h
static void buzzerBeep(int freq, int duration)
{
    ledcWriteTone(0, freq);
    delay(duration);
    ledcWriteTone(0, 0);
    delay(50);
}

void buzzer_init(int pin)
{
    pinMode(pin, OUTPUT);
    ledcAttachPin(pin, 0);
}

void beepConnecting()
{
    // Bip simples
    buzzerBeep(2000, 100);
}

void beepSuccess()
{
    // 3 bips rápidos
    for (int i = 0; i < 3; i++)
    {
        buzzerBeep(2500, 70);
        delay(60);
    }
}

void beepFailAP()
{
    // Bip longo
    buzzerBeep(1000, 400);
}

void beepSaved()
{
    // 2 bips médios
    for (int i = 0; i < 2; i++)
    {
        buzzerBeep(1800, 200);
        delay(100);
    }
}

void beepResetWarning()
{
    // Alerta sonoro antes do reset (3 bips)
    for (int i = 0; i < 3; i++)
    {
        buzzerBeep(1500, 300);
        if (i < 2)
            delay(700);
    }
}