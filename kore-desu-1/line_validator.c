#include "line_validator.h"
#include "error.h"      
#include <string.h>
#include <ctype.h>
#include <stdio.h>

// global variables to store declared variable names
char vars[MAX_VARS][MAX_VAR_LENGTH];
int valid_buffer_counter = 0;

char used_vars[MAX_VARS];
int registers[30];
int used_registers[30];

// TO DO (and optional): add more
char *forbidden[] = {
    "int", "return", "for", "while", "if", "else",
    "char", "float", "double", "goto", "main", "double"
};


// check if variable is already declared
int IsVariableDeclared(char *variableName) {
    for(int v = 0; v < valid_buffer_counter; v++)
        if(strcmp(vars[v], variableName) == 0)
            return 1;
    return 0;
}


// ================= Parses and validates an expression after '=' =========================
// allowClosing: 0 = no closing parenthesis allowed, 1 = allows closing one level of parenthesis
int AfterEqualsCheck(char *buffer, int *startCounter, int allowClosing) {
    int expectOperand = 1; // Expect variable/number/expression first
    int counter = *startCounter;

    while(buffer[counter] != '\0' && buffer[counter] != ';' && buffer[counter] != ',') {
        // skip spaces
        while(buffer[counter] == ' ')
            counter++;

        if(expectOperand) {
            if(buffer[counter] == '(') {
                counter++; // consume '('
                if(!AfterEqualsCheck(buffer, &counter, 1)) // recursive check, allow closing ')'
                    return 0;
                expectOperand = 0; // next expect operator
                continue;
            }
            else if(isalpha(buffer[counter]) || buffer[counter] == '_') {
                int start = counter;
                // parse variable name
                while(isalnum(buffer[counter]) || buffer[counter] == '_')
                    counter++;
                int len = counter - start;
                char var_name[MAX_VAR_LENGTH];
                strncpy(var_name, buffer + start, len);
                var_name[len] = '\0';
                // variable must be declared
                if(!IsVariableDeclared(var_name))
                    return 0;
                expectOperand = 0; // next expect operator
            }
            else if(buffer[counter] == '-') {
                counter++;
                // parse number
                while(isdigit(buffer[counter]))
                    counter++;
                expectOperand = 0; // next expect operator
            }
            else if(isdigit(buffer[counter])) {
                // parse number
                while(isdigit(buffer[counter]))
                    counter++;
                expectOperand = 0; // next expect operator
            } 
            else {
                // invalid operand
                return 0;
            }
        }
        else {
            // operator or closing
            while(buffer[counter] == ' ')
                counter++;
            if(buffer[counter] == ')') {
                if(allowClosing) {
                    counter++; // consume ')'
                    *startCounter = counter;
                    return 1; // successfully closed parentheses
                }
                else
                    return 0; // unexpected ')'
            }
            else if(strchr("+-*/", buffer[counter])) {
                counter++; // consume operator
                expectOperand = 1; // expect operand next
            }
            else if(buffer[counter] == ';' || buffer[counter] == ',') {
                // end of this expression segment: handled by caller
                break;
            }
            else {
                // unknown character in expression
                return 0;
            }
        }
    }

    // must end expression after a valid operand
    if(expectOperand) {
        // still expecting operand but hit end, then invalid
        return 0;
    }

    *startCounter = counter;
    return 1;
}


// ============== Parses one variable declaration or initialization from current startIndex =============
// examples it handles: "x", "x = 2", "x = (a+3)"
// moves startIndex to after parsed variable declaration including optional initialization
ErrorType ParseVariableAssignment(char *buffer, int *startIndex, char *errinfo) {
    int i = *startIndex;

    // skip leading spaces
    while(buffer[i] == ' ')
        i++;

    // invalid: identifier cannot start with digit or '_'
    if(buffer[i] == '_' || isdigit(buffer[i])) {
        strncpy(errinfo, buffer + i, 1);
        errinfo[1] = '\0';
        return ERR_INVALID_IDENTIFIER;
    }

    int startVar = i;
    while(isalnum(buffer[i]) || buffer[i] == '_')
        i++;

    if(i == startVar) // no variable name
        return ERR_SYNTAX;

    int len = i - startVar;
    char var_name[MAX_VAR_LENGTH];
    strncpy(var_name, buffer + startVar, len);
    var_name[len] = '\0';

    // check keyword-as-identifier
    for(int k = 0; k < 11; k++) {
        if(strcmp(var_name, forbidden[k]) == 0) {
            strcpy(errinfo, var_name);
            return ERR_KEYWORD_AS_IDENTIFIER;
        }
    }

    // skip spaces after var name
    while(buffer[i] == ' ')
        i++;

    // optional initialization '='
    if(buffer[i] == '=') {
        i++; // consume '='
        while(buffer[i] == ' ')
            i++;

        // validate expression
        if(!AfterEqualsCheck(buffer, &i, 0)) {
            strcpy(errinfo, var_name);
            return ERR_INVALID_EXPRESSION;
        }
    }

    // redeclared?
    if(IsVariableDeclared(var_name)) {
        strcpy(errinfo, var_name);
        return ERR_REDECLARED;
    }

    // store variable
    strcpy(vars[valid_buffer_counter], var_name);
    valid_buffer_counter++;

    // skip trailing spaces after variable/init
    while(buffer[i] == ' ')
        i++;

    *startIndex = i;
    return ERR_NONE;
}


//ErrorType StartsWithVariableName(char *buffer, char *errinfo);

// ============================= Buffer starts with "int" =====================================
// e.g., "int a; int b;"  or  "int x = 5; int y, z;"
ErrorType StartsWithInt(char *buffer, char *errinfo) {
    // previous: does not handle multiple declarations in one line and multiple ; (e.g., int a; int b;;;)
    //     int i = 3; // position right after "int"

    //     if(strncmp(buffer, "int ", 4) != 0)
    //         return ERR_SYNTAX;

    //     i++; // skip space

    //     while(1) {
    //         ErrorType err = ParseVariableAssignment(buffer, &i, errinfo);
    //         if(err != ERR_NONE)
    //             return err;

    //         while(buffer[i] == ' ')
    //             i++;

    //         if(buffer[i] == ',') {
    //             i++;
    //             continue;
    //         }
    //         else if(buffer[i] == ';') {
    //             i++;
    //             break;
    //         }
    //         else {
    //             return ERR_SYNTAX;
    //         }
    //     }


    // fixed + added a block to handle multiple declarations mixed with assignment/s
    int i = 0;

    while(buffer[i] != '\0') {
        // skip whitespace between statements
        while(isspace(buffer[i])) i++;

        if(buffer[i] == '\0')
            break;

        // must start with "int "
        if(strncmp(buffer + i, "int ", 4) == 0) {
            i += 4;  // skip "int "
            while(1) {
                ErrorType err = ParseVariableAssignment(buffer, &i, errinfo);
                if(err != ERR_NONE)
                    return err;

                // skip spaces after variable
                while(buffer[i] == ' ')
                    i++;

                if(buffer[i] == ',') {
                    i++;  // consume comma, continue same declaration line
                    continue;
                }
                else if(buffer[i] == ';') {
                    // skip all trailing ';' and spaces b4 next int statement
                    while(buffer[i] == ' ' || buffer[i] == ';')
                        i++;
                    break;
                }
                else {
                    return ERR_SYNTAX;
                }
            }
        }
        else {
            // the next segment is not "int", delegate to StartsWithVariableName
            return StartsWithVariableName(buffer + i, errinfo);
        }
        // loop continues for the next int declarations
    }

    // adter procrssing all "int" statements, nothing else should remain
    // whitespaces are alr skipped in the loop
    while(isspace(buffer[i]))
        i++;
    if(buffer[i] != '\0')
        return ERR_SYNTAX;

    return ERR_NONE;
}


// ========================== Buffer starts with a variable ===============================
// e.g. "x = 5; y = x + 3; z = 1;"
ErrorType StartsWithVariableName(char *buffer, char *errinfo) {
    int i = 0;

    // last: doesn't hanlde multiple assignments in one line and multiple ; (like x = 12; y = x * 2;;;)
    //     if(buffer[0] == '_' || isdigit(buffer[0])) {
    //         strncpy(errinfo, buffer, 1);
    //         errinfo[1] = '\0';
    //         return ERR_INVALID_IDENTIFIER;
    //     }

    //     int startVar = i;
    //     while(isalnum(buffer[i]) || buffer[i] == '_')
    //         i++;

    //     if(i == startVar) {
    //         strncpy(errinfo, buffer, 1);
    //         errinfo[1] = '\0';
    //         return ERR_SYNTAX;
    //     }

    //     int len = i - startVar;
    //     char var_name[MAX_VAR_LENGTH];
    //     strncpy(var_name, buffer + startVar, len);
    //     var_name[len] = '\0';

    //     strncpy(errinfo, var_name, MAX_VAR_LENGTH-1);
    //     errinfo[MAX_VAR_LENGTH-1] = '\0';

    //     while(buffer[i] == ' ')
    //         i++;

    //     if(buffer[i] != '=')
    //         return ERR_SYNTAX;

    //     i++;

    //     while(buffer[i] == ' ')
    //         i++;

    //     if(!AfterEqualsCheck(buffer, &i, 0))
    //         return ERR_INVALID_EXPRESSION;

    //     while(buffer[i] == ' ')
    //         i++;

    //     if(buffer[i] != ';')
    //         return ERR_MISSING_SEMICOLON;

    //     i++;

    //     while(buffer[i] == ' ' || buffer[i] == '\n' || buffer[i] == '\r')
    //         i++;

    //     if(buffer[i] != '\0')
    //         return ERR_SYNTAX;

    //     if(!IsVariableDeclared(var_name))
    //         return ERR_UNDECLARED;

    //     return ERR_NONE;

    // fixed + added a block for multiple assignments mixed with "int" declaration/s
    while(buffer[i] != '\0') {
        // skip whitespace between statements
        while(isspace(buffer[i]))
            i++;
        if(buffer[i] == '\0')
            break;
        
        // if the next statement starts with "int", hand it to StartsWithInt
        // to handle cases like (with sudden int): 
        // result = result + a;;;; int b;
        if(strncmp(buffer + i, "int ", 4) == 0) {
            return StartsWithInt(buffer + i, errinfo);
        }

        // parse identifier (must be a valid variable name)
        if(buffer[i] == '_' || isdigit(buffer[i])) {
            strncpy(errinfo, buffer + i, 1);
            errinfo[1] = '\0';
            return ERR_INVALID_IDENTIFIER;
        }

        int startVar = i;
        while(isalnum(buffer[i]) || buffer[i] == '_')
            i++;

        if(i == startVar) // no identifier found
            return ERR_SYNTAX;
        
        int len = i - startVar;
        char var_name[MAX_VAR_LENGTH];
        strncpy(var_name, buffer + startVar, len);
        var_name[len] = '\0';
        strcpy(errinfo, var_name);  // for error reporting

        if(!IsVariableDeclared(var_name)) {
            // check keyword-as-identifier
            for(int k = 0; k < 11; k++) {
                if(strcmp(var_name, forbidden[k]) == 0) {
                    strcpy(errinfo, var_name);
                    return ERR_SYNTAX;
                }
            }
            while(isspace(buffer[(strlen(var_name) - 1) + i]))
                i++;
            if(buffer[i] == '=')
                return ERR_UNDECLARED;
            else 
                return ERR_SYNTAX;
        }

        // skip spaces after variable name
        while(buffer[i] == ' ')
            i++;

        // must have '='
        if(buffer[i] != '=') 
            return ERR_SYNTAX;
        i++;  // consume '='

        while(buffer[i] == ' ')
            i++;

        // validate the expression after '='
        if(!AfterEqualsCheck(buffer, &i, 0))
            return ERR_INVALID_EXPRESSION;

        // skip spaces after expression
        while(buffer[i] == ' ')
            i++;

        // must end with ';'
        if(buffer[i] != ';')
            return ERR_MISSING_SEMICOLON;
        else {
            while(buffer[i] == ' ' || buffer[i] == ';')
                i++;
        }
    }

    // after processing all assignments, skip trailing whitespace
    while(isspace(buffer[i]))
        i++;
    if(buffer[i] != '\0')
        return ERR_SYNTAX;

    return ERR_NONE;
}

// ======================== Remove leading and trailing spaces from the buffer =======================
void RemoveLeadingAndTrailingSpaces(char *buffer) {
    int start = 0;
    int end = strlen(buffer) - 1;

    while(isspace(buffer[start]))
        start++;
    while(end >= start && isspace(buffer[end]))
        end--;

    int j = 0;
    for(int i = start; i <= end; i++)
        buffer[j++] = buffer[i];
    buffer[j] = '\0';
}

// ==================== Removes all spaces from the string (used for output purposes) ===================
char* RemoveAllSpaces(char *buffer, char *spacelessBuffer) {
    int i, j;
    for(i = 0, j = 0; buffer[i] != '\0'; i++) {
        if(buffer[i] == ' ')
            continue;
       spacelessBuffer[j++] = buffer[i];
    }
    spacelessBuffer[j] = '\0';
    return spacelessBuffer;
}
