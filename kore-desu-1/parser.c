#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "parser.h"

// removes leading/trailing spaces from the given s (works on local buffer)
static void TrimLocalBuffer(char *s) {
    char *p = s;
    while(*p && isspace(*p))
        p++;
    // shift string contents to remove leading spaces
    memmove(s, p, strlen(p) + 1);
    // remove trailing spaces
    int len = strlen(s);
    while(len > 0 && isspace(s[len - 1]))
        s[--len] = '\0';
}

// parse "int x;", "int a, b, c;"  into one or more Statement structures
int ParseStatement(const char *line, Statement *out) {
    char tmp[MAX_STMT_LEN];
    // copy input line into temporary buffer to allow modification
    strncpy(tmp, line, MAX_STMT_LEN - 1);
    tmp[MAX_STMT_LEN - 1] = '\0';
    // remove surrrounding spaces 
    TrimLocalBuffer(tmp);

    int count = 0; // number of parsed statements so far
    char *ptr = tmp;

    while(*ptr) {
        // skip whitespace between statements
        while(isspace(*ptr))
            ptr++;
        if(*ptr == '\0')
            break;

        // case 1: declaration statements
        if(strncmp(ptr, "int ", 4) == 0) {
            char *start = ptr + 4; // point after "int "
            char *semi = strchr(start, ';'); // find end of this declaration
            if(!semi)
                break; // incomplete line

            // temporarily terminate at semicolon so we process only one "int ...;" block
            *semi = '\0';
            char *vars = start;

            // split by commas
            char *tok = strtok(vars, ",");
            while(tok) {
                TrimLocalBuffer(tok); // remove spaces around each var token
                if(tok[0] == '\0') { // skip empty tokens 
                    tok = strtok(NULL, ",");
                    continue;
                }

                Statement s;
                s.type = STMT_DECL; // mark statement type as declaration
                s.raw[0] = '\0'; // initialize raw buffer

                // handle initialization, e.g., x = 5;
                char *eq = strchr(tok, '=');
                if(eq) {
                    *eq = '\0'; // split into LHS and RHS
                    TrimLocalBuffer(tok);
                    TrimLocalBuffer(eq + 1);

                    // copy variable name (LHS)
                    strncpy(s.lhs, tok, sizeof(s.lhs) - 1);
                    s.lhs[sizeof(s.lhs) - 1] = '\0';
                    // copy expression/value (RHS)
                    strncpy(s.rhs, eq + 1, sizeof(s.rhs) - 1);
                    s.rhs[sizeof(s.rhs) - 1] = '\0';
                } // handle simple declaration without initialization: e.g., int x; 
                else {
                    TrimLocalBuffer(tok);
                    strncpy(s.lhs, tok, sizeof(s.lhs) - 1);
                    s.lhs[sizeof(s.lhs) - 1] = '\0';
                    s.rhs[0] = '\0'; // no RHS
                }
                // store original input file for reference/debugging
                strncpy(s.raw, line, sizeof(s.raw) - 1);
                s.raw[sizeof(s.raw) - 1] = '\0';

                // save parsed statement
                out[count++] = s;

                // continue parsing next variable in same line, if any
                tok = strtok(NULL, ",");
            }

            ptr = semi + 1;  // move past the semicolon to check for more statements
            continue;
        }

        // case 2: assignment "x = expressiom;" (no "int")
        char *semi = strchr(ptr, ';');
        if(!semi)
            break;

        // Temporarily cut at semicolon
        *semi = '\0';
        char *eqpos = strchr(ptr, '=');
        if(eqpos) {
            *eqpos = '\0'; // separate LHS and RHS
            char *lhs = ptr;
            char *rhs = eqpos + 1;

            // remove extra spaces
            TrimLocalBuffer(lhs);
            TrimLocalBuffer(rhs);
            if(lhs[0] == '\0' || rhs[0] == '\0') // ensure both sides are non-empty
                goto next_statement;

            // create statement structure
            Statement s;
            s.type = STMT_ASSIGN;
            strncpy(s.lhs, lhs, sizeof(s.lhs) - 1);
            s.lhs[sizeof(s.lhs) - 1] = '\0';
            strncpy(s.rhs, rhs, sizeof(s.rhs) - 1);
            s.rhs[sizeof(s.rhs) - 1] = '\0';
            strncpy(s.raw, line, sizeof(s.raw) - 1);
            s.raw[sizeof(s.raw) - 1] = '\0';

            // store result
            out[count++] = s;
        }

    next_statement:
        ptr = semi + 1; // advance to after this semicolon
    }

    // case 3: not a declaration nor an assignment
    return count;
}