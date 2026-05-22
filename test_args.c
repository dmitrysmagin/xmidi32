#include <stdio.h>
int main(int argc, char *argv[]) {
    FILE *f = fopen("test_args_log.txt", "w");
    if (f) {
        fprintf(f, "argc=%d\n", argc);
        for (int i = 0; i < argc; i++) fprintf(f, "argv[%d]=%s\n", i, argv[i]);
        fprintf(f, "hello from test_args\n");
        fclose(f);
    }
    return 42;
}
