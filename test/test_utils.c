#include <stdbool.h>
#include <stdarg.h>

#include <stdio.h>
#include "test_utils.h"

#define RED_BOLD   "\e[1;31m"
#define GREEN_BOLD "\e[1;32m"
#define ANSI_RESET "\x1b[0m"

void test_res(bool test_var, char *argv[], char *format, ...) {
  va_list args;
  va_start(args, format);
  char str[80];
  sprintf(str, format, args);
  if (test_var) {
    printf("%s %s\r\t\t\tPASSED%s %s\n", argv[0], GREEN_BOLD, ANSI_RESET, str);
  } else {
    printf("%s %s\r\t\t\tFAILED%s %s\n", argv[0], RED_BOLD, ANSI_RESET, str);
  }
}
