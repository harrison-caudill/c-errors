/*
 * Copyright (c) 2014 Harrison Caudill
 * All Rights Reserved
 * harrison@hypersphere.org
 */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "util.h"
#include "ctest.h"
#include "error.h"

int int_win(){return 0;}
int int_lose(){return 1;}

int sys_win(){return 0;}
int sys_lose(){
    void *ptr = malloc(1ull<<62ull);
    /* printf("PTR: %p\n", ptr); */
    /* printf("ERRNO: %d\n", *__error()); */
    return -1;
}

void *ptr_win(){return (void*)0xdeadbeef;}
void *ptr_lose(){return NULL;}

CTEST(ErrorFramework, test_int_win)
{
    ERR_INIT();

    ERR_CRIT_COND(int_win(),
                  ERR_ERRNO_ENOMEM,
                  "I really should not see this...");

    ERR_DONE();
    ASSERT_EQUAL(ERR_IS_ERROR(ERR_RETVAL()), FALSE);
    ASSERT_EQUAL(ERR_IS_OK(ERR_RETVAL()), TRUE);
}

CTEST(ErrorFramework, test_int_lose)
{
    ERR_INIT();

    ERR_CRIT_COND(int_lose(),
                  ERR_ERRNO_ENOMEM,
                  "I really should see this...");

    ERR_DONE();
    ASSERT_EQUAL(ERR_IS_ERROR(ERR_RETVAL()), TRUE);
    ASSERT_EQUAL(ERR_IS_OK(ERR_RETVAL()), FALSE);
    ASSERT_EQUAL((ERR_RETVAL().bits == ERR_ERRNO_ENOMEM.bits), TRUE);
}

CTEST(ErrorFramework, test_sys_win)
{
    ERR_INIT();

    ERR_CRIT_RET(sys_win(), "I really should not see this...");

    ERR_DONE();
    ASSERT_EQUAL(ERR_IS_ERROR(ERR_RETVAL()), FALSE);
    ASSERT_EQUAL(ERR_IS_OK(ERR_RETVAL()), TRUE);
}

CTEST(ErrorFramework, test_sys_lose)
{
    ERR_INIT();

    int ret = sys_lose();
    /* printf("ERRNO: %d\n", *__error()); */
    /* printf("ENOMEM: %d\n", ERR_ERRNO_ENOMEM.fields.code); */

    int _val = ret;
    int _lvl = ANDROID_LOG_FATAL;
    char *msg = "I really should see this";

    ERR_CRIT_RET(ret, "I really should see this...");

    printf("------------------> %d\n", errno);
    printf("------------------> %lld\n", ERR_ERRNO().bits);

    ERR_DONE();
    ASSERT_EQUAL(ERR_IS_ERROR(ERR_RETVAL()), TRUE);
    ASSERT_EQUAL(ERR_IS_OK(ERR_RETVAL()), FALSE);
    ASSERT_EQUAL((ERR_RETVAL().bits == ERR_ERRNO_ENOMEM.bits), TRUE);
}
