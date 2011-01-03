#include "BudikInterfaces.h"

char* downame[] = {"Po", "Ut", "St", "Ct", "Pa", "So", "Ne"};
char* monthname[] = {"Led", "Uno", "Bre", "Dub", "Kve", "Cer", "Cec", "Srp", "Zar", "Rij", "Lis", "Pro"};

uint8_t BudikSetTimeInterface::nibbles[] = {0, 2, 5, 8, 8, 12, 14};
uint8_t BudikSetTimeInterface::nibblerow[] = {0, 0, 1, 1, 0, 0, 0};

uint8_t BudikSetAlarmInterface::nibbles[] = {0, 2, 5, 8, 8, 12, 14};
uint8_t BudikSetAlarmInterface::nibblerow[] = {0, 0, 1, 1, 0, 0, 0};

