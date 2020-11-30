#define C_DEFAULT "\e[39m"
#define C_RED  "\e[31m"
#define C_GREEN "\e[32m"
#define C_YELLOW "\e[33m"
#define C_BLUE "\e[34m"
#define C_MAGENTA " \e[35m"
#define C_CYAN "\e[36m"
#define C_WHITE "\e[97m"


enum color {RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE, DEFAULT};

void colorPrint(char *message, enum color colore);