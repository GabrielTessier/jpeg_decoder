#include <stdio.h>
#include "test_utils.h"

#define RED_BOLD   "\e[1;31m"
#define GREEN_BOLD "\e[1;32m"
#define ANSI_RESET "\x1b[0m"

void test_res(int test_var, char *test_name, char *argv[]) {
  if (test_var) {
    printf("%s %s\r\t\t\tPASSED%s %s\n", argv[0], GREEN_BOLD, ANSI_RESET, test_name);
  } else {
    printf("%s %s\r\t\t\tFAILED%s\t%s\n", argv[0], RED_BOLD, ANSI_RESET, test_name);
  }
}
