#include <stdlib.h>

#include "fliki.h"
#include "global.h"
#include "debug.h"

/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the various options that were specified will be
 * encoded in the global variable 'global_options', where it will be
 * accessible elsewhere in the program.  For details of the required
 * encoding, see the assignment handout.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * @modifies global variable "global_options" to contain an encoded representation
 * of the selected program options.
 * @modifies global variable "diff_filename" to point to the name of the file
 * containing the diffs to be used.
 */

int validargs(int argc, char **argv) {
    // Case 1: No flags
    if(argc == 1) {
        global_options = 0;
        //printf("Global Options: %ld\n", global_options); // Should be 0
        return -1;
    }

    int i = 0;
    for (char **p = argv + 1; *p != NULL; p++, i++) {
        // Case 2: Flags
        if(**p == '-') {
            (*p)++;
            // Case 2a: -h
            if(**p == 'h') {
                if(i != 0) {
                    return -1;
            }
                // Case 2aii: h is the first arg
                else {
                    global_options = HELP_OPTION;
                    // printf("Global Options: %ld\n", global_options); // Should be 1
                    return 0;
                }
            }
            // Case 2b: -n
            if(**p == 'n') {
                if(argc == (i+2)) {
                    // ERROR
                    global_options = 0;
                    return -1;
                }
                global_options = NO_PATCH_OPTION | global_options; // Should be 4
            }
            // Case 2c: -q
            if(**p == 'q') {
                if(argc == (i+2)) {
                    // ERROR
                    global_options = 0;
                    return -1;
                }
                global_options = QUIET_OPTION | global_options; // Should be 4
            }
        }
        // Case 3: Filename
            else {
                if(argc == 2)
                    global_options = 0;
                diff_filename = *p;
                return 0;
            }
    }
    return 0;
}