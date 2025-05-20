#include <stdbool.h>
#include <stdarg.h>

#include <colors.h>
#include <stdio.h>
#include "test_utils.h"

void test_res(bool test_var, char *argv[], char *format, ...) {
  va_list args;
  va_start(args, format);
  char str[150];
  vsprintf(str, format, args);
  va_end(args);
  if (test_var) {
     printf("%s %s\r\t\t\tPASSED%s %s\n", argv[0], BOLDGREEN, RESET, str);
  } else {
    printf("%s %s\r\t\t\tFAILED%s %s\n", argv[0], BOLDRED, RESET, str);
  }
}
