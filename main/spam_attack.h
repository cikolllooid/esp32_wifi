#ifndef SPAM_ATTACK_H
#define SPAM_ATTACK_H

#include <stdint.h>

// Константы
#define BEACON_SSID_OFFSET 38
#define SRCADDR_OFFSET 10
#define BSSID_OFFSET 16
#define SEQNUM_OFFSET 22
#define TOTAL_LINES (sizeof(rick_ssids) / sizeof(char *))

// Глобальные переменные
extern uint8_t beacon_raw[];
extern char *rick_ssids[];

void start_spam(int time);

#endif // SPAM_TASK_H
