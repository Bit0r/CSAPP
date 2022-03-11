#include <stdio.h>

int main(int argc, char const *argv[], char const *envp[]) {
    puts("Command-line arguments:");
    for (int i = 0; i < argc; i++) {
        printf("\targv[%2d]: %s\n", i, argv[i]);
    }
    puts("Environment variables:");
    for (int i = 0; envp[i] != NULL; i++) {
        printf("\tenvp[%2d]: %s\n", i, envp[i]);
    }
    return 0;
}
