#include "logger.h"

int main()
{
    srand(time(NULL));
    printf("getpid: %d\n", getpid());
    sleep(5);
    init(NULL);

    for (int i = 0; i < 20; i++)
    {
        myLog(rand() % 3, "log nr: %d", i + 1);
        sleep(2);
    }

    destroy();
    return 0;
}