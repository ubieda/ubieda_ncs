#include "basic_module.h"

#include <stdbool.h>

static bool initialized = false;
static int value = 0;

int basic_module_init(void)
{
    initialized = true;
    return 0;
}

void basic_module_destroy(void)
{
    initialized = false;
    value = 0;
}

int basic_module_read(void)
{
    if(!initialized)
        return -1;

    return value;
}

void basic_module_write(int val)
{
    value = val;
}