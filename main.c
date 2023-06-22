#define INTPRT_IMPL
#include "intprt.h"

int main(void) {
  while (1) {
    buffer = alloc_input();
    char *saved = buffer;
    if (buffer == NULL) {
      printf("malloc failed, exiting\n");
      exit(1);
    }
    printf("Enter an expression (Q/q to quit): ");
    fgets(buffer, MAX_BUFFER_SIZE, stdin);
    if (buffer[0] == 'q' || buffer[0] == 'Q') {
      free(buffer);
      printf("Quitting...\n");
      break;
    }

    // Evaluation
    lexAndPrintTokens(saved);
    buildAST();
    printf("\n= %ld\n", executeAST(ast));

    // Free memory
    freeNodes();
    free(saved);
  }
  return 0;
}

