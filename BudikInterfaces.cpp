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
    {0, 14},
    {3, 1}
};

BudikInterface::UIPosition BudikSetAlarmInterface::nibbles[] = {
    {2, 3},
    {2, 8},
    {2, 11},
    {3, 0},
    {3, 2},
    {3, 4},
    {3, 6},
    {3, 8},
    {3, 10},
    {3, 12},
    {3, 14}
};

