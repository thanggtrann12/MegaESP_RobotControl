#pragma once
#include <stdio.h>
static int g_fail = 0, g_pass = 0;
#define CHECK(cond, msg) do { if (cond) { ++g_pass; printf("  PASS  %s\n", msg); } else { ++g_fail; printf("  FAIL  %s\n", msg); } } while (0)
