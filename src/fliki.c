#include <stdlib.h>
#include <stdio.h>

#include "fliki.h"
#include "global.h"
#include "debug.h"

// 0 is false, 1 is true
int change_eos;
int reading_hunk;
int serial_num;
int pass;
int error_in_hdr;
int eos_reached;
int lines_read;
char *arr;
int arr_index;
int prev_old_start;
int change_section;
HUNK hunk;

/**
 * @brief Get the header of the next hunk in a diff file.
 * @details This function advances to the beginning of the next hunk
 * in a diff file, reads and parses the header line of the hunk,
 * and initializes a HUNK structure with the result.
 *
 * @param hp  Pointer to a HUNK structure provided by the caller.
 * Information about the next hunk will be stored in this structure.
 * @param in  Input stream from which hunks are being read.
 * @return 0  if the next hunk was successfully located and parsed,
 * EOF if end-of-file was encountered or there was an error reading
 * from the input stream, or ERR if the data in the input stream
 * could not be properly interpreted as a hunk.
 */

int hunk_next(HUNK *hp, FILE *in) {
    // reading_hunk = 0;

    // initialize variables
    (*hp).type = HUNK_NO_TYPE;
    (*hp).old_start = 0;
    (*hp).old_end = 0;
    (*hp).new_start = 0;
    (*hp).new_end = 0;
    (*hp).serial = 0;

    int c;

    //(1) Read diff file until you hit an integer
    if(serial_num == 0) {
        serial_num++;
        c = fgetc(in);
        if(!(c >= '0' && c <= '9')){
            // if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
            //     fprintf(stderr, "Error: File must begin with valid header\n");
            //     hunk_show(hp, stderr);
            // }
            return ERR;
        }
        while(c != EOF) {
            if(c >= '0' && c <= '9')
                break;
            c = fgetc(in);
        }
        if(c==EOF){
            c= fgetc(in);
            return EOF;
        }
    } else {
        while(c!=EOF){
            c = fgetc(in);
            if(c=='\n'){
                c = fgetc(in);
                if(c >= '0' && c <= '9')
                    break;
            }
        }
        if(c==EOF)
            return EOF;
    }

    // (2) Parse header
    // (2a) Get old_start
    int number = 0;
    while(c != '\0'){
        if(c >= '0' && c <= '9')
            number = number*10 + c - '0';
        else
            break;
        c = fgetc(in);
    }
    (*hp).old_start = number;

    // (2b) If a comma exists, find old_end
    if(c==','){
        number = 0;
        c = fgetc(in);
            if(!(c >= '0' && c <= '9')){
                // if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
                //     fprintf(stderr, "Error: Value after comma must be a number\n");
                //     hunk_show(hp, stderr);
                // }
                return ERR;
            }
            while(c >= '0' && c <= '9'){
                number = number*10 + c - '0';
                c = fgetc(in);
            }
            if(number < (*hp).old_start) {
                // if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
                //     fprintf(stderr, "Error: End value cannot be bigger than start value\n");
                //     hunk_show(hp, stderr);
                // }
                return ERR;
            }
            (*hp).old_end = number;
    } else
        (*hp).old_end = (*hp).old_start;

    // (2c) Get command
    if(c=='a') {
        if((*hp).old_end != (*hp).old_start){
            // if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
            //     fprintf(stderr, "Error: Addition command not allowed to have ending line number in old file.\n");
            //     hunk_show(hp, stderr);
            // }
            error_in_hdr = 1;
            while(c != '\n') // Skip to end of line
                c = fgetc(in);
            return ERR;
        }(*hp).type = HUNK_APPEND_TYPE;
    } else {
        if(c=='d'){
            (*hp).type = HUNK_DELETE_TYPE;
            if((*hp).old_start == 0){
                // if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
                //     fprintf(stderr, "Error for old value: Value cannot be 0\n");
                //     hunk_show(hp, stderr);
                // }
                return ERR;
            }
        }
        else {
            if(c=='c')
                (*hp).type = HUNK_CHANGE_TYPE;
            else {
                // if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
                //     fprintf(stderr, "Error: Command of unknown type\n");
                //     hunk_show(hp, stderr);
                // }
                error_in_hdr = 1;
                while(c != '\n') // Skip to end of line
                    c = fgetc(in);
                return ERR;
            }
        }
    }

    // (2d) Get new_start
    c = fgetc(in);
    number = 0;
    while(c != '\0'){
        if(c >= '0' && c <= '9'){
            number = number*10 + c - '0';
        } else {
            break;
        }
        c = fgetc(in);
    }
    if(number == 0 && (*hp).type == HUNK_APPEND_TYPE) {
        // if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
        //     fprintf(stderr, "Error for new_start value: Value cannot be 0\n");
        //     hunk_show(hp, stderr);
        // }
        return ERR;
    }
    (*hp).new_start = number;

    // (2e) If a comma exists, find new_end
    if(c==','){
        number = 0;
        c = fgetc(in);
            if(!(c >= '0' && c <= '9')){
                // if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
                //     fprintf(stderr, "Error: Value after comma must be a number\n");
                //     hunk_show(hp, stderr);
                // }
                return ERR;
            }
            while(c >= '0' && c <= '9'){
                number = number*10 + c - '0';
                c = fgetc(in);
            }
            if(number == 0 && (*hp).type == HUNK_APPEND_TYPE) {
                // if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
                //     fprintf(stderr, "Error: Value cannot be 0\n");
                //     hunk_show(hp, stderr);
                // }
                return ERR;
            }
            if(number < (*hp).new_start) {
                // if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
                //     fprintf(stderr, "Error: End value cannot be bigger than start value\n");
                //     hunk_show(hp, stderr);
                // }
                return ERR;
            }
            (*hp).new_end = number;
    } else
        (*hp).new_end = (*hp).new_start;

    if(c != '\n'){
        // if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
        //     fprintf(stderr, "Error: Newline value expected\n");
        //     hunk_show(hp, stderr);
        // }
        return ERR;
    }

    // (2f) Return ERRs
    if((*hp).type == HUNK_DELETE_TYPE && (*hp).new_end != (*hp).new_start){
        // if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
        //     fprintf(stderr, "Error: Delete command not allowed to have ending line value for new file\n");
        //     hunk_show(hp, stderr);
        // }
        error_in_hdr = 1;
        while(c != '\n') // Skip to end of line
            c = fgetc(in);
        return ERR;
    }

    // (3) Pass into HUNK
    (*hp).serial = serial_num;
    serial_num++;

    ungetc('\n', in);
    error_in_hdr = 0;
    eos_reached = 0;
    return(0);
}

/**
 * @brief  Get the next character from the data portion of the hunk.
 * @details  This function gets the next character from the data
 * portion of a hunk.  The data portion of a hunk consists of one
 * or both of a deletions section and an additions section,
 * depending on the hunk type (delete, append, or change).
 * Within each section is a series of lines that begin either with
 * the character sequence "< " (for deletions), or "> " (for additions).
 * For a change hunk, which has both a deletions section and an
 * additions section, the two sections are separated by a single
 * line containing the three-character sequence "---".
 * This function returns only characters that are actually part of
 * the lines to be deleted or added; characters from the special
 * sequences "< ", "> ", and "---\n" are not returned.
 * @param hdr  Data structure containing the header of the current
 * hunk.
 *
 * @param in  The stream from which hunks are being read.
 * @return  A character that is the next character in the current
 * line of the deletions section or additions section, unless the
 * end of the section has been reached, in which case the special
 * value EOS is returned.  If the hunk is ill-formed; for example,
 * if it contains a line that is not terminated by a newline character,
 * or if end-of-file is reached in the middle of the hunk, or a hunk
 * of change type is missing an additions section, then the special
 * value ERR (error) is returned.  The value ERR will also be returned
 * if this function is called after the current hunk has been completely
 * read, unless an intervening call to hunk_next() has been made to
 * advance to the next hunk in the input.  Once ERR has been returned,
 * then further calls to this function will continue to return ERR,
 * until a successful call to call to hunk_next() has successfully
 * advanced to the next hunk.
 */

int hunk_getc(HUNK *hp, FILE *in) {
    change_eos = 0;
    int c = fgetc(in);

    if(error_in_hdr == 1) {
        // if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
        //     fprintf(stderr, "Error: Cannot make successful call to hunk_getc() when there is an error in the header\n");
        //     hunk_show(hp, stderr);
        // }
        return ERR;
    }

    if(eos_reached == 1) {
        // if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
        //     fprintf(stderr, "Error: Cannot make successful call to hunk_getc() when EOS has been reached for this hunk\n");
        //     hunk_show(hp, stderr);
        // }
        return ERR;
    }

    if(c=='\n'){
        reading_hunk = 1;

        if((arr_index+3) < 510){
            *arr = c;
            arr+=3;
            arr_index+=3;
        }

        if(pass == 1)
            return c;
        else{
            c = fgetc(in);
            if(arr_index < 510){
                *arr = c;
                arr++;
                arr_index++;
            }
        }

        pass = 1;
    } else {
        if(arr_index < 510){
            *arr = c;
            arr++;
            arr_index++;
        }
    }

    if(reading_hunk == 1) { // Means a \n has just been read
        if(c=='>' || c=='<'){
            if((hunk.type == HUNK_APPEND_TYPE && c=='<') || (hunk.type == HUNK_DELETE_TYPE && c=='>')){
                // if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
                //     fprintf(stderr, "Error: Wrong symbol for specified command\n");
                //     hunk_show(hp, stderr);
                // }
                return ERR;
            }
            if((hunk.type == HUNK_CHANGE_TYPE && change_section == 1 && c=='<') || (hunk.type == HUNK_CHANGE_TYPE && change_section  == 0 && c=='>')){
                // if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
                //     fprintf(stderr, "Error: Wrong symbol for specified command\n");
                //     hunk_show(hp, stderr);
                // }
                return ERR;
            }
            c = fgetc(in);
            if(arr_index < 510){
                *arr = c;
                arr++;
                arr_index++;
            }
            if(c == ' '){
                c = fgetc(in);
                if(arr_index < 510){
                    *arr = c;
                    arr++;
                    arr_index++;
                }
                reading_hunk = 0;
                return c;
            } else {
                // if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
                //     fprintf(stderr, "Error: No space after < or > symbol\n");
                //     hunk_show(hp, stderr);
                // }
                return ERR;
            }
        } else {
            if(c=='-'){
                c = fgetc(in);
                if(arr_index < 510){
                    *arr = c;
                    arr++;
                    arr_index++;
                }
                if(c=='-'){
                    c = fgetc(in);
                    if(arr_index < 510){
                        *arr = c;
                        arr++;
                        arr_index++;
                    }
                    if(c=='-') {
                        c = fgetc(in);
                        if(c=='\n') {
                            if((*hp).type == HUNK_CHANGE_TYPE){
                                change_eos = 1;
                                reading_hunk = 0;
                                arr -= arr_index;
                                arr = hunk_additions_buffer;
                                arr_index = 0;
                                change_section = 1;
                                ungetc('\n', in);
                                pass = 0;
                                return EOS;
                            } else {
                                // if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
                                //     fprintf(stderr, "Error: Only change hunk allowed to have ---\n");
                                //     hunk_show(hp, stderr);
                                // }
                                reading_hunk = 0;
                                return ERR;
                            }
                        } else {
                            if(arr_index < 510){
                                *arr = c;
                                arr++;
                                arr_index++;
                            }
                            // if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
                            //     fprintf(stderr, "Error: Line formatted incorrectly\n");
                            //     hunk_show(hp, stderr);
                            // }
                            reading_hunk = 0;
                            return ERR;
                        }
                    } else {
                        // if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
                        //     fprintf(stderr, "Error: Line formatted incorrectly\n");
                        //     hunk_show(hp, stderr);
                        // }
                        reading_hunk = 0;
                        return ERR;
                    }
                } else {
                    // if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
                    //     fprintf(stderr, "Error #7: Line formatted incorrectly\n");
                    //     hunk_show(hp, stderr);
                    // }
                    reading_hunk = 0;
                    return ERR;
                }
            } else {
                if(c>='0' && c<='9') {
                    ungetc(c,in);
                    ungetc('\n', in);
                    reading_hunk = 0;
                    eos_reached = 1;
                    return EOS;
                } else {
                    if(c == EOF){
                        c = getc(in);
                        if(c == -1){
                            ungetc(c, in);
                            ungetc('\n', in);
                            eos_reached = 1;
                            return EOS;
                        }
                        else {
                            // if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
                            //     fprintf(stderr, "Error #8: EOF reached in middle of file\n");
                            //     hunk_show(hp, stderr);
                            // }
                            return ERR;
                        }
                    } else {
                        // if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
                        //     fprintf(stderr, "Error #7: Line formatted incorrectly\n");
                        //     hunk_show(hp, stderr);
                        // }
                        reading_hunk = 0;
                        return ERR;
                    }
                }
            }
        }
    }
    else
        return c;
}

/**
 * @brief  Print a hunk to an output stream.
 * @details  This function prints a representation of a hunk to a
 * specified output stream.  The printed representation will always
 * have an initial line that specifies the type of the hunk and
 * the line numbers in the "old" and "new" versions of the file,
 * in the same format as it would appear in a traditional diff file.
 * The printed representation may also include portions of the
 * lines to be deleted and/or inserted by this hunk, to the extent
 * that they are available.  This information is defined to be
 * available if the hunk is the current hunk, which has been completely
 * read, and a call to hunk_next() has not yet been made to advance
 * to the next hunk.  In this case, the lines to be printed will
 * be those that have been stored in the hunk_deletions_buffer
 * and hunk_additions_buffer array.  If there is no current hunk,
 * or the current hunk has not yet been completely read, then no
 * deletions or additions information will be printed.
 * If the lines stored in the hunk_deletions_buffer or
 * hunk_additions_buffer array were truncated due to there having
 * been more data than would fit in the buffer, then this function
 * will print an elipsis "..." followed by a single newline character
 * after any such truncated lines, as an indication that truncation
 * has occurred.
 *
 * @param hp  Data structure giving the header information about the
 * hunk to be printed.
 * @param out  Output stream to which the hunk should be printed.
 */

void hunk_show(HUNK *hp, FILE *out) {
    int truncation = 0;

    // (1) Print header to stderr no matter what
    fprintf(out, "%d", (*hp).old_start);

    if((*hp).old_end != (*hp).old_start){
        fputc(',', out);
        fprintf(out, "%d", (*hp).old_end);
    }

    if((*hp).type == HUNK_DELETE_TYPE)
        fputc('d', out);
    else {
        if((*hp).type == HUNK_APPEND_TYPE)
            fputc('a', out);
        else{
            if((*hp).type == HUNK_CHANGE_TYPE)
                fputc('c', out);
        }
    }

    fprintf(out, "%d", (*hp).new_start);

    if((*hp).new_end != (*hp).new_start){
        fputc(',', out);
        fprintf(out, "%d", (*hp).new_end);
    }

    fprintf(out,"\n");

    eos_reached = 1;

    if(eos_reached == 1){
        // (2) Fill buffer
        int j = arr_index;
        if(arr_index == 510)
            truncation = 1;
        int count = 0;
        while(j != 0) {
            if(*arr == 0) {
                if(count > 511) {
                    *arr = (unsigned char)512;
                    arr--;
                    j--;
                    *arr = 0;
                    arr--;
                    j--;
                } else {
                    if(count > 255) {
                        *arr = (unsigned char)256;
                        arr--;
                        j--;
                        count -= 256;
                        *arr = count;
                        arr--;
                        j--;
                    } else {
                        // *arr = 0
                        arr--;
                        j--;
                        *arr = count;
                        arr--;
                        j--;
                    }
                }

                count = 0;
                continue;
                }
            arr--;
            count++;
            j--;
        }

        // (3) Print buffer
        arr++; // Because the last while loop went one over with arr--
        int i = 0;
        unsigned char char_ct = 0;
        count = 0;
        while(i != arr_index){
            if(char_ct == count){
                char_ct = (unsigned char)*arr;
                count = 0;

                fprintf(out, "%u ", (unsigned char)*arr);
                arr++;

                char_ct += (unsigned char)(*arr);
                fprintf(out, "%u", (unsigned char)*arr);
                arr++;

                i+=2;
                continue;
            }
            fprintf(out, "%c", *arr);
            arr++;
            i++;
            count++;
        }

        if(truncation == 1)
            fprintf(out,"...\n");
    }
}

static int a_type(FILE *in, FILE *out, FILE *diff){
    // (1) Copy all lines from input file <= old_start
    // Use getc() to copy characters from input file, increment line_number with \n
    int c;
    int header_check = 0;

    while(lines_read <= (hunk.old_start)) {
        c = fgetc(in);
        if(global_options != NO_PATCH_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION)))
            fputc(c, out);
        if(c == '\n')
            lines_read++;
    }

    // (2) At old_start + 1, copy from diff file to output file
    // Use hunk_getc() to copy characters from diff file until it reaches EOS
    int d = hunk_getc(&hunk, diff); // Skip first \n?
    if(d == '\n')
        d = hunk_getc(&hunk, diff);
    if(d == ERR)
        return ERR;
    while(d != EOS) {
        if(d == ERR)
            return ERR;
        if(d == '\n')
            header_check++;
        if(global_options != NO_PATCH_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION)))
            fputc(d, out);
        d = hunk_getc(&hunk, diff);
    }

    if(header_check != (hunk.new_end - hunk.new_start + 1)){
        // if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
        //     fprintf(stderr, "Error: Incorrect number of lines\n");
        //     hunk_show(&hunk, stderr);
        // }
        return ERR;
    }
    arr -= arr_index;
    arr_index = 0;
    return 0;
}

static int d_type(FILE *in, FILE *out, FILE *diff){
    // (1) Copy all lines from input file <= old_start
        int c;
        int header_check = 0;
        while(lines_read <= (hunk.old_start-1)) {
            c = fgetc(in);
            if(global_options != NO_PATCH_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION)))
                fputc(c, out);
            if(c == '\n')
                lines_read++;
        }

    // (2) At old_start + 1, skip old_end - old_start + 1 lines from input file
        int skip_amt = hunk.old_end - hunk.old_start + 1;

        int y;
        if(hunk.serial != 1)
            y= hunk_getc(&hunk, diff);
        int count = 0;
        while(count != skip_amt) {
            c = fgetc(in);
            y = hunk_getc(&hunk, diff);
            if(y == ERR)
                return ERR;
            if(y!=ERR && y!=EOS){
                if(c != y){
                    // if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
                    //     fprintf(stderr, "Error: Does not match line from input file\n");
                    //     hunk_show(&hunk, stderr);
                    // }
                    return ERR;
                }
            }
            if(c == '\n'){
                count++;
                lines_read++;
                header_check++;
            }
        }

        if(header_check != (hunk.old_end - hunk.old_start + 1)){
            // if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
            //     fprintf(stderr, "Error: Incorrect number of lines\n");
            //     hunk_show(&hunk, stderr);
            // }
            return ERR;
        }
    arr -= arr_index;
    arr_index = 0;
    return 0;
}

static int c_type(FILE *in, FILE *out, FILE *diff){
    // (1) Copy all lines from input file <= old_start
    // Use getc() to copy characters from input file, increment line_number with \n
    int c;
    int header_check_sep = 0;
    while(lines_read <= (hunk.old_start-1)) {
        c = fgetc(in);
        if(global_options != NO_PATCH_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION)))
            fputc(c, out);
        if(c == '\n')
            lines_read++;
    }

    // (2) At old_start + 1, skip old_end - old_start + 1 lines from input file
    int skip_amt = hunk.old_end - hunk.old_start + 1;
    int y;
    if(hunk.serial != 1)
        y= hunk_getc(&hunk, diff);
    int count = 0;
    while(count != skip_amt) {
        c = fgetc(in);
        y = hunk_getc(&hunk, diff);
        if(y == ERR)
            return ERR;
        if(y!=ERR && y!=EOS){
            if(c != y){
                // if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
                //     fprintf(stderr, "Error: Does not match line from input file\n");
                //     hunk_show(&hunk, stderr);
                // }
                return ERR;
            }
        }
        if(c == '\n'){
            count++;
            lines_read++;
            header_check_sep++;
        }
    }

    if(header_check_sep != (hunk.old_end - hunk.old_start + 1)){
        // if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
        //     fprintf(stderr, "Error: Incorrect number of lines\n");
        //     hunk_show(&hunk, stderr);
        // }
        return ERR;
    }

    // (3) Copy after --- EOS
    int x = hunk_getc(&hunk, diff);
    int header_check_two = 0;
    while((x != EOS) && (change_eos != 1)){
        if(x == ERR)
            return ERR;
        x = hunk_getc(&hunk, diff);
    }

    // (4) Copy amt
    int copy_amt = 1;
    if(hunk.new_end != hunk.new_start)
        copy_amt = hunk.new_end - hunk.new_start + 1;

    count = 0;
    while(count != copy_amt) { // COunt != copy_amt prevents you from confirming that enough lines are there
        x = hunk_getc(&hunk, diff);
        if(x == ERR)
            return ERR;
        if(global_options != NO_PATCH_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION)))
            fputc(x, out);
        if(x == '\n'){
            count++;
            header_check_two++;
        }
    }

    if(header_check_two != (hunk.new_end - hunk.new_start + 1)){
        // if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
        //     fprintf(stderr, "Error: Incorrect number of lines\n");
        //     hunk_show(&hunk, stderr);
        // }
        return ERR;
    }

    arr -= arr_index;
    arr_index = 0;
    ungetc(x, diff);

    return 0;
}

/**
 * @brief  Patch a file as specified by a diff.
 * @details  This function reads a diff file from an input stream
 * and uses the information in it to transform a source file, read on
 * another input stream into a target file, which is written to an
 * output stream.  The transformation is performed "on-the-fly"
 * as the input is read, without storing either it or the diff file
 * in memory, and errors are reported as soon as they are detected.
 * This mode of operation implies that in general when an error is
 * detected, some amount of output might already have been produced.
 * In case of a fatal error, processing may terminate prematurely,
 * having produced only a truncated version of the result.
 * In case the diff file is empty, then the output should be an
 * unchanged copy of the input.
 *
 * This function checks for the following kinds of errors: ill-formed
 * diff file, failure of lines being deleted from the input to match
 * the corresponding deletion lines in the diff file, failure of the
 * line numbers specified in each "hunk" of the diff to match the line
 * numbers in the old and new versions of the file, and input/output
 * errors while reading the input or writing the output.  When any
 * error is detected, a report of the error is printed to stderr.
 * The error message will consist of a single line of text that describes
 * what went wrong, possibly followed by a representation of the current
 * hunk from the diff file, if the error pertains to that hunk or its
 * application to the input file.  If the "quiet mode" program option
 * has been specified, then the printing of error messages will be
 * suppressed.  This function returns immediately after issuing an
 * error report.
 *
 * The meaning of the old and new line numbers in a diff file is slightly
 * confusing.  The starting line number in the "old" file is the number
 * of the first affected line in case of a deletion or change hunk,
 * but it is the number of the line *preceding* the addition in case of
 * an addition hunk.  The starting line number in the "new" file is
 * the number of the first affected line in case of an addition or change
 * hunk, but it is the number of the line *preceding* the deletion in
 * case of a deletion hunk.
 *
 * @param in  Input stream from which the file to be patched is read.
 * @param out Output stream to which the patched file is to be written.
 * @param diff  Input stream from which the diff file is to be read.
 * @return 0 in case processing completes without any errors, and -1
 * if there were errors.  If no error is reported, then it is guaranteed
 * that the output is complete and correct.  If an error is reported,
 * then the output may be incomplete or incorrect.
 */

int patch(FILE *in, FILE *out, FILE *diff) {
    if (diff == NULL)
        return -1;

    int p = fgetc(diff);
    if(p == EOF) { // If file is empty, copy input into output file
        int j = fgetc(in);
        while(j != EOF){
            fputc(j, out);
            j = fgetc(in);
        } return 0;
    } else
        ungetc(p, diff);

    lines_read = 1;
    int h = hunk_next(&hunk, diff);
    while(h != EOF){
        if(h == ERR){
            if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
                    fprintf(stderr, "Error: Error in reading header\n");
                    hunk_show(&hunk, stderr);
            }
            return -1;
        }

        if(hunk.type == HUNK_DELETE_TYPE){
            arr = hunk_deletions_buffer;
            if(hunk.old_start < prev_old_start){
                if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
                    fprintf(stderr, "Error: Line range cannot be below previous line ranges\n");
                    hunk_show(&hunk, stderr);
                }
                return -1;
            }
            if (d_type(in, out, diff) == ERR){
                if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
                    fprintf(stderr, "Error: Error in parsing hunk\n");
                    hunk_show(&hunk, stderr);
                }
                return -1;
            }
        }
        else{
            if(hunk.type == HUNK_APPEND_TYPE){
                arr = hunk_additions_buffer;
                if(hunk.old_start < prev_old_start){
                    if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
                        fprintf(stderr, "Error: Line range cannot be below previous line ranges\n");
                        hunk_show(&hunk, stderr);
                    }
                    return -1;
                }
                if(a_type(in, out, diff) == ERR){
                    if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
                        fprintf(stderr, "Error: Error in parsing hunk\n");
                        hunk_show(&hunk, stderr);
                    }
                    return -1;
                }
            }
            else{
                if(hunk.type == HUNK_CHANGE_TYPE){
                    arr = hunk_deletions_buffer; // To start with...
                    if(hunk.old_start < prev_old_start){
                        if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
                            fprintf(stderr, "Error: Line range cannot be below previous line ranges\n");
                            hunk_show(&hunk, stderr);
                        }
                        return -1;
                    }
                    if(c_type(in, out, diff) == ERR){
                        if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
                            fprintf(stderr, "Error: Error in parsing hunk\n");
                            hunk_show(&hunk, stderr);
                        }
                        return -1;
                    }
                }
                else{
                    if(global_options != QUIET_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION))){
                        fprintf(stderr, "Error: Not valid hunk type\n");
                        hunk_show(&hunk, stderr);
                    }
                    return -1;
                }
            }
        }
        prev_old_start = hunk.old_start;
        h = hunk_next(&hunk, diff);
    }

    int c;
    c = fgetc(in);
    while(c != EOF){
        if(global_options != NO_PATCH_OPTION && (global_options != (QUIET_OPTION|NO_PATCH_OPTION)))
            putc(c, out);
        c = fgetc(in);
    }

    return 0;
}