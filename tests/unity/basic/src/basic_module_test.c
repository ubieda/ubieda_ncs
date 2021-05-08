#include <unity.h>
#include "basic_module.h"

void setUp(void)
{
}

void tearDown(void)
{
    basic_module_destroy();
}

/* Suite teardown shall finalize with mandatory call to generic_suiteTearDown. */
extern int generic_suiteTearDown(int num_failures);

int test_suiteTearDown(int num_failures)
{
    return generic_suiteTearDown(num_failures);
}

void test_basic_init_retuns_zero(void)
{    
    int err;
    err = basic_module_init();
    TEST_ASSERT_EQUAL(0,err);
}

void test_basic_read_returns_negative_when_uninit(void)
{
    int err;
    err = basic_module_read();
    TEST_ASSERT_EQUAL(-1,err);
}

void test_basic_read_returns_zero_by_default(void)
{
    int err;
    basic_module_init();
    err = basic_module_read();
    TEST_ASSERT_EQUAL(0,err);
}

void test_basic_verify_write(void)
{
    int err;
    basic_module_init();
    basic_module_write(1);
    err = basic_module_read();
    TEST_ASSERT_EQUAL(1,err);
}

void test_basic_read_resets_to_zero_when_destroyed(void)
{
    int err;
    basic_module_init();
    basic_module_write(1);
    basic_module_read();
    basic_module_destroy();
    basic_module_init();
    err = basic_module_read();
    TEST_ASSERT_EQUAL(0,err);
}

void test_basic_multiple_writes_last_one_persists(void)
{
    int err;
    basic_module_init();
    basic_module_write(1);
    basic_module_write(2);
    err = basic_module_read();
    TEST_ASSERT_EQUAL(2,err);
}

/* It is required to be added to each test. That is because unity is using
 * different main signature (returns int) and zephyr expects main which does
 * not return value.
 */
extern int unity_main(void);

void main(void)
{
    (void)unity_main();
}