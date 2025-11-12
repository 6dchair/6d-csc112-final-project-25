#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "line_validator.h" // syntax validation functions
#include "parser.h" // parsing input lines into Statement structs
#include "assembly.h" // assembly code generation from parsed statements
#include "symbol_table.h" // variable-to-register mapping management
#include "machine_code.h"  // conversion of assembly to machine code
#include "usage_counter.h" // register priority handling

int main(void) {
    // 1) OPEN SOURCE FILE
    FILE *f = fopen("INPUT.txt", "r"); 
    if(!f) {
        printf("Unable to access the input text file\n");
        return 1;                        
    }

    // 2) INITIAL SETUP
    char buffer[BUFFER]; // stores each line read from source file
    Statement stmts[1024];  // global storage for all parsed statements from the entire text file
    int stmt_count = 0; // total count of valid parsed statements or keeps track of how many valid statements have been stored
    FILE *MACHINE_CODE = fopen("MACHINE_CODE.mc", "a+"); // for writing ALL machine codes later

    SymbolInit(); // initialize the symbol table before parsing

    // display header for readability in terminal
    printf("****** SOURCE->MIPS64->MACHINE CODE ******\n");

    int buffer_count = 1;
    // 3) READ FILE LINE BY LINE
    while(fgets(buffer, sizeof(buffer), f)) {
        RemoveLeadingAndTrailingSpaces(buffer); // trim leading/trailing spaces
        if(buffer[0] == '\0')
            continue; // skip blank lines
        printf("[%d]\n", buffer_count++);
        int isbuffervalid = 0; // flag for syntax validation result

        // 3A) DETERMINE LINE TYPE
        // if line starts with "int ", it’s a declaration
        // othewise, it must be an assignment or expression
        if(strncmp(buffer, "int ", 4) == 0)
            isbuffervalid = StartsWithInt(buffer);
        else
            isbuffervalid = StartsWithVariableName(buffer);

        // 3B) HANDLE INVALID LINES
        if(!isbuffervalid) {
            printf("\tSOURCE:\n\t\t%s\n\t\tError! Invalid syntax\n\n", buffer);  // print invalid line message
            // separate each source block visually in terminal
            // continue; // skip to next line
            return 1;
        }

        // 3C) VALID LINE HANDLING
        printf("\tSOURCE:\n\t\t%s\n\t\tTransform: Correct syntax\n\n", buffer);  // display the valid source line

        // parse valid line into Statement structures
        Statement parsed[32];
        int parsed_count = ParseStatement(buffer, parsed);


        // 4) PRINT ASSEMBLY CODE
        FILE *asmout = fopen("temp_buffer.asm", "w+");
        if(!asmout) {
            printf("Cannot open file\n");
            return 1;
        }
        printf("\tMIPS64 ASSEMBLY:\n");
        for(int k = 0; k < parsed_count; k++) {
            // store parsed statements globally
            if(stmt_count < (int)(sizeof(stmts)/sizeof(stmts[0])))
                stmts[stmt_count++] = parsed[k];
            // print assembly instructions for this line
            // GenerateAssemblyStatement(&parsed[k], stdout);
            GenerateAssemblyStatement(&parsed[k], asmout);
        }
        rewind(asmout);
        while(fgets(buffer, sizeof(buffer), asmout)) {
            printf("\t\t%s", buffer);
        }
        printf("\n");

        rewind(asmout);
        printf("\tMACHINE CODE:\n");
        MachineFromAssembly("temp_buffer.asm", "temp_buffer.mc");
        FILE *mcout = fopen("temp_buffer.mc", "r");
        if(mcout && MACHINE_CODE) {
            char line[BUFFER];
            while(fgets(line, BUFFER, mcout)) {
                printf("\t\t%s", line); // show each line from .mc
                fprintf(MACHINE_CODE, "%s", line);
            }
            printf("\n");
            fclose(mcout);
        }

        fclose(asmout);

    }

    fclose(f); // close source file

    // 8): FINAL OUTPUT FILES
    // before generating the final assembly, analyze variable usage frequency
    AnalyzeVariableUsage(stmts, stmt_count);

    // then generate the assembly using the (possibly re-prioritized) symbol table
    FILE *MIPS64_ASSEMBLY = fopen("MIPS64_ASSEMBLY.txt", "w");
    if(!MIPS64_ASSEMBLY) {
        printf("Cannot open file\n");
        return 1;
    }
    AssemblyGenerateProgram(stmts, stmt_count, MIPS64_ASSEMBLY);
    fclose(MIPS64_ASSEMBLY);
    fclose(MACHINE_CODE);
}