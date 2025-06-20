#ifndef BUZZER_H
#define BUZZER_H

void buzzer_init(int pin);
void beepConnecting();
void beepSuccess();
void beepFailAP();
void beepSaved();
void beepResetWarning();

#endif // BUZZER_H