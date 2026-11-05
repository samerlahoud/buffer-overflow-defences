#include <stdio.h>
#include <string.h>

/* Input contract: one line of terminal text, without embedded NUL bytes.
 * Accept at most 15 input bytes. Reject a longer line rather than use a prefix.
 * This one-shot program exits after rejection; a retry loop must drain the line. */
int main(void) {
    char password[16];
    int passcheck = 0;

    setvbuf(stdout, NULL, _IONBF, 0);
    printf("\nWhat is the secret password?\n");
    if (fgets(password, sizeof password, stdin) == NULL) {  // bounded read
        puts("Rejected: no input or read error.");
        return 1;
    }
    char *newline = strchr(password, '\n');
    if (newline != NULL) {
        *newline = '\0';
    } else {
        /* A full buffer can mean exactly 15 bytes or an oversized line.
         * Look ahead without writing another byte into password. */
        int next = getchar();
        if (next != '\n' && next != EOF) {
            puts("Rejected: maximum is 15 input bytes.");
            return 1;
        }
        if (ferror(stdin)) {
            puts("Rejected: read error.");
            return 1;
        }
    }

    if (strcmp(password, "password1") == 0) {
        printf("\nCorrect Password\n");
        passcheck = 1;
    } else {
        printf("\nYou fail!\n");
    }

    if (passcheck) {
        printf("\n[Privileged action executed]\n");
    }

    return 0;
}
