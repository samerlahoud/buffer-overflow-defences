#include <stdio.h>
#include <string.h>

/* Deliberately unsafe teaching example. gets was removed from ISO C11;
 * Ubuntu's glibc retains the symbol, so declare it explicitly for this lab. */
extern char *gets(char *);

int main(void) {
    char password[16];
    int passcheck = 0;

    /* Keep evidence visible even if the process later aborts. */
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("\nWhat is the secret password?\n");
    if (gets(password) == NULL) return 1;  // intentionally unbounded

    if (strcmp(password, "password1") == 0) {  // INSPECT: input has returned
        printf("\nCorrect Password\n");
        passcheck = 1;  // only set when password is correct
    } else {
        printf("\nYou fail!\n");
    }

    if (passcheck) {   // any nonzero value means "authenticated"
        // Simulated privileged action
        printf("\n[Privileged action executed]\n");
    }

    return 0;
}
