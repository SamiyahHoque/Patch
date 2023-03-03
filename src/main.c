#include <stdio.h>
#include <stdlib.h>

#include "fliki.h"
#include "global.h"
#include "debug.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

int main(int argc, char **argv)
{
    if(validargs(argc, argv) == -1)
        USAGE(*argv, EXIT_FAILURE);
    if(global_options == HELP_OPTION)
        USAGE(*argv, EXIT_SUCCESS);
    else {
        if(global_options == 0 || global_options == NO_PATCH_OPTION || global_options == QUIET_OPTION
        || global_options == (NO_PATCH_OPTION|QUIET_OPTION)) {
            FILE * fp = fopen (diff_filename, "r");

            int ret = patch(stdin, stdout, fp);
            fclose(fp);
            if(ret == 0) return EXIT_SUCCESS;
            else{
                if(ret == -1) return EXIT_FAILURE;
            }
        }
    }
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
