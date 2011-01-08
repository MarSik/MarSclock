#include "BudikInterfaces.h"

const char* downame[] = {"Po", "Ut", "St", "Ct", "Pa", "So", "Ne"};
const char* monthname[] = {"Led", "Uno", "Bre", "Dub", "Kve", "Cer", "Cec", "Srp", "Zar", "Rij", "Lis", "Pro"};

BudikInterface::UIPosition BudikSetTimeInterface::nibbles[] = {
    {0, 0},
    {0, 2},
    {1, 5},
    {1, 8},
    {0, 8},
    {0, 12},
    {0, 14}
};

BudikInterface::UIPosition BudikSetAlarmInterface::nibbles[] = {
    {0, 0},
    {0, 2},
    {1, 5},
    {1, 8},
    {0, 8},
    {0, 12},
    {0, 14}
};

