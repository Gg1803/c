//  CITS2002 Project 1 2024
//  Student1:   23862402    Kavishani Vimalathasan
//  Student2:   23887876    Gargi Garg
//  Platform:   Apple

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <unistd.h>  // For getpid() function to generate unique process ID
#include <ctype.h>   // For character type functions (like islower)

#define line_length 256  // Define the maximum line length for reading input

// Function to check if the input file has a valid ".ml" extension
int check_extension(const char *filename) {
    const char *dot = strrchr(filename, '.');  // Find the last occurrence of '.'
    if (!dot || dot == filename) return 0;  // If no extension or the filename starts with '.', return 0 (invalid)
    else return (strcmp(dot, ".ml") == 0);  // Return 1 if the extension is ".ml", indicating it's a valid ML file
}

// Function to print syntax error messages
void report_syntax_error(const char *message) {
    fprintf(stderr, "! %s\n", message);  // Error messages start with '!' to differentiate from debug messages
}

// Function to print debug messages for tracking
void report_debug_info(const char *message) {
    printf("@ %s\n", message);  // Debug messages start with '@' for clarity during execution
}

// Function to validate variable names in ML files
// The name must be lowercase, alphabetic, and 1-12 characters long
bool variable_name_validation(char *variable) {
    // Check if the first character is lowercase and alphabetic, and the length is within bounds
    if (islower(variable[0]) && isalpha(variable[0]) && (strlen(variable) >= 1) && (strlen(variable) <= 12)) {
        return true;  // Return true if valid
    } else {
        report_syntax_error("Invalid Variable Name.");  // Report an error if the validation fails
        return false;
    }
}

// Function to translate assignment statements from ML to C
void translate_assignment_statement(char *line, FILE *c_fptr, bool *is_global) {
    char variable_name[50];  // Buffer to hold variable name
    double value = 0.0;      // Variable to hold numerical values
    char expression[line_length];  // Buffer to hold mathematical expressions

    // Handle the simple case: variable <- value
    if (sscanf(line, "%49s <- %lf", variable_name, &value) == 2) {
        if (is_global) {  // If it's a global variable
            if (variable_name_validation(variable_name))
                fprintf(c_fptr, "double %s = %.1f;\n", variable_name, value);  // Write C code for global variable assignment
        } else {  // If it's a local variable
            if (variable_name_validation(variable_name))
                fprintf(c_fptr, "   double %s = %.1f;\n", variable_name, value);  // Write C code for local variable assignment
        }
    } 
    // Handle the complex case: variable <- expression
    else if (sscanf(line, "%s <- %[^\n]", variable_name, expression) == 2) {
        if (is_global) {  // If it's a global variable
            if (variable_name_validation(variable_name))
                fprintf(c_fptr, "double %s = %s;\n", variable_name, expression);  // Write C code for global assignment with expression
        } else {  // If it's a local variable
            if (variable_name_validation(variable_name))
                fprintf(c_fptr, "   double %s = %s;\n", variable_name, expression);  // Write C code for local assignment with expression
        }
    } else {
        report_syntax_error("Invalid assignment statement.");  // Report syntax error if neither case is matched
    }
}

// Function to translate print statements from ML to C
void translate_print_statement(char *line, FILE *c_fptr) {
    char arguments[line_length];  // Buffer to hold print arguments
    char *trimmed_line = line;    // Trimmed version of the line to remove leading spaces

    // Skip leading spaces and tabs
    while (*trimmed_line == ' ' || *trimmed_line == '\t') {
        trimmed_line++;
    }

    // Check if the line starts with "print"
    if (strncmp(trimmed_line, "print", 5) == 0) {
        trimmed_line += 5;  // Move past "print"
        while (*trimmed_line == ' ' || *trimmed_line == '\t') {
            trimmed_line++;
        }
        strncpy(arguments, trimmed_line, sizeof(arguments));  // Copy the arguments after "print"

        // Generate C code to print integers or floating-point numbers with appropriate format
        fprintf(c_fptr, "    if (floor(%s) == %s) {\n", arguments, arguments);  // Check if it's an integer
        fprintf(c_fptr, "        printf(\"%%.0f\\n\", %s);\n", arguments);  // Print as an integer (no decimal places)
        fprintf(c_fptr, "    } else {\n");
        fprintf(c_fptr, "        printf(\"%%.6f\\n\", %s);\n", arguments);  // Print as a float with 6 decimal places
        fprintf(c_fptr, "    }\n");
    } else {
        report_syntax_error("Invalid print statement.");  // Report syntax error if the statement doesn't start with "print"
    }
}

// Function to translate return statements from ML to C
void translate_return_statement(char *line, FILE *c_fptr) {
    char *return_keyword = "return";  // Define the keyword "return"
    char *return_expression;          // Buffer to hold the return expression

    // Check if the line contains the return keyword
    if ((return_expression = strstr(line, return_keyword)) != NULL) {
        return_expression += strlen(return_keyword);  // Move past the "return" keyword
        while (*return_expression == ' ') return_expression++;  // Skip spaces after "return"

        fprintf(c_fptr, "   return %s;\n", return_expression);  // Write C code for the return statement
    } else {
        report_syntax_error("Invalid return statement.");  // Report error if no return statement is found
    }
}

// Function to check if a function contains a return statement
bool function_type(FILE *ml_fptr) {
    char line[line_length];     // Buffer to hold lines from the ML file
    bool is_function = false;   // Track if inside a function
    bool function_type = false; // Track if the function contains a return statement

    // Read each line of the ML file to detect function definitions and return statements
    while (fgets(line, line_length, ml_fptr)) {
        if (strncmp(line, "function", 8) == 0) {
            is_function = true;  // Detect the start of a function definition
            report_debug_info("Function definition detected.");
        } else if (is_function && strstr(line, "return") != NULL) {
            function_type = true;  // Detect the return statement in the function
            report_debug_info("Return statement detected in function.");
        } else if (is_function && (strncmp(line, "#", 1) == 0 || strncmp(line, "\n", 1) == 0)) {
            is_function = false;  // End of the function block
            report_debug_info("End of function block detected.");
        }
    }

    fclose(ml_fptr);  // Close the file after reading
    return function_type;  // Return true if a return statement is found in the function
}

// Function to translate function definitions from ML to C
void translate_function_definition(char *line, FILE *ml_fptr, FILE *c_fptr, bool function_condition) {
    char function_name[50];  // Buffer to hold the function name
    char parameters[line_length];  // Buffer to hold the function parameters
    char parameter_list[line_length] = "";  // Initialize an empty parameter list
    char function_line[line_length];  // Buffer to hold lines of the function body

    // Parse the function name and its parameters
    if (sscanf(line, "function %s %[^\n]", function_name, parameters) < 1) {
        report_syntax_error("Invalid function definition.");  // Report error if the function definition is incorrect
        return;
    }

    // Prepare the function parameters for the C code
    char *elements = strtok(parameters, " ");  // Split parameters by space
    while (elements != NULL) {
        if (strlen(parameter_list) > 0) 
            strcat(parameter_list, ", ");  // Add a comma between parameters
        sprintf(parameter_list + strlen(parameter_list), "double %s", elements);  // Format each parameter as double in C
        elements = strtok(NULL, " ");  // Move to the next parameter
    }

    // Write the translated function definition to the C output file
    fprintf(c_fptr, "double %s(%s) {\n", function_name, parameter_list);

    // Translate the function body
    bool has_return = false;
    while (fgets(function_line, sizeof(function_line), ml_fptr)) {
        if (strstr(function_line, "<-")) {
            translate_assignment_statement(function_line, c_fptr, NULL);  // Translate assignment statements within the function
        } else if (strstr(function_line, "return")) {
            translate_return_statement(function_line, c_fptr);  // Translate return statements within the function
            has_return = true;  // Mark that the return statement has been handled
        } else if (strstr(function_line, "print")) {
            translate_print_statement(function_line, c_fptr);  // Translate print statements within the function
        } else if (strstr(function_line, "#")) {
            break;  // End of function block, stop reading further
        } else {
            report_syntax_error("Invalid function body statement.");  // Report error if an invalid statement is found
        }
    }

    // Ensure that non-void functions have a return statement
    if (!has_return) {
        fprintf(c_fptr, "   return 0;  // Default return value if missing\n");  // Insert default return if none exists
    }

    fprintf(c_fptr, "}\n\n");  // Close the function definition in C
}

// Main function that handles translating ML code to C
void translate_ml_to_c(FILE *ml_fptr, FILE *c_fptr, bool *function_return) {
    char function_name[50];  // Buffer to hold function names
    char arguments[line_length];  // Buffer to hold function arguments
    char line[line_length];  // Buffer to hold each line of ML code
    bool main_started = false;  // Flag to track if the main() function has started
    bool has_global_variable = false;  // Flag to track if there are global variables

    // Loop to read and translate each line of the ML file
    while (fgets(line, sizeof(line), ml_fptr)) {
        if (line[0] == '#' || line[0] == '\n') continue;  // Skip comments and blank lines

        if (strncmp(line, "function", 8) == 0) {
            // Translate function definitions (outside of main())
            translate_function_definition(line, ml_fptr, c_fptr, *function_return);
        } else if (strstr(line, "<-")) {
            // Handle global variable assignments (outside of main())
            translate_assignment_statement(line, c_fptr, NULL);  // NULL indicates a global variable
            has_global_variable = true;
        } else {
            // Start main() if not already started
            if (!main_started) {
                fprintf(c_fptr, "int main() {\n");
                main_started = true;
            }

            // Handle print and function call statements within main()
            if (strncmp(line, "print ", 6) == 0) {
                translate_print_statement(line, c_fptr);  // Translate print statements
            } else if (sscanf(line, "%[^()](%[^)])", function_name, arguments) == 2) {
                // If function has a return value, print it
                if (*function_return) {
                    fprintf(c_fptr, "    printf(\"%%.6f\\n\", (double)(%s(%s)));\n", function_name, arguments);
                } else {  // Otherwise, just call the function
                    fprintf(c_fptr, "    %s(%s);\n", function_name, arguments);
                }
            } else {
                report_syntax_error("Invalid statement.");  // Report error for invalid statements
            }
        }
    }

    // Ensure main() function is properly closed
    if (main_started) {
        fprintf(c_fptr, "   return 0;\n}\n");  // Close main with return 0
    } else if (!main_started && has_global_variable) {
        // If global variables exist but no main, create an empty main() function
        fprintf(c_fptr, "int main() {\n   return 0;\n}\n");
    }
}

int main(int argc, char *argv[]) {
    // Check if the number of arguments is less than 2, meaning no input file is provided
    if (argc < 2) {
        // Print the correct usage of the program to standard error
        fprintf(stderr, "! Usage: %s <input_file.ml> [args...]\n", argv[0]);
        return EXIT_FAILURE;  // Exit the program with failure status
    }

    // Check if the file provided has a valid ".ml" extension
    if (!check_extension(argv[1])) {
        // Print error message if the file does not have a valid extension
        fprintf(stderr, "! The file %s is not a valid .ml language file\n", argv[1]);
        return EXIT_FAILURE;  // Exit with failure status
    }

    // Open the .ml file for reading
    FILE *ml_fptr = fopen(argv[1], "r");
    if (ml_fptr == NULL) {
        // Print error message if the file could not be opened
        perror("! Error opening .ml file");
        return EXIT_FAILURE;  // Exit with failure status
    }

    // Check if the function contains a return statement by analyzing the ML file
    bool function_return = function_type(ml_fptr);
    fclose(ml_fptr);  // Close the file after determining if there's a return statement

    // Generate unique file names based on the process ID to avoid conflicts
    int pid = getpid();  // Get the process ID
    char c_filename[20], exec_filename[20];  // Buffers to store C file and executable names
    snprintf(c_filename, sizeof(c_filename), "ml-%d.c", pid);  // Name for the generated C file
    snprintf(exec_filename, sizeof(exec_filename), "ml-%d", pid);  // Name for the executable file

    // Open a new file to write the translated C code
    FILE *c_fptr = fopen(c_filename, "w");
    if (c_fptr == NULL) {
        // Print error message if the C file could not be created
        perror("! Could not create C output file");
        return EXIT_FAILURE;  // Exit with failure status
    }

    // Write necessary includes for the C program to the output file
    fprintf(c_fptr, "#include <stdio.h>\n#include <stdlib.h>\n#include <math.h>\n\n");

    // Re-open the .ml file to begin translating its contents to C
    ml_fptr = fopen(argv[1], "r");
    if (ml_fptr == NULL) {
        // Print error message if the file could not be opened again
        perror("! Error re-opening .ml file");
        return EXIT_FAILURE;  // Exit with failure status
    }

    // Translate the ML code to C and write it to the C file
    translate_ml_to_c(ml_fptr, c_fptr, &function_return);
    fclose(c_fptr);  // Close the C file after writing
    fclose(ml_fptr);  // Close the ML file after reading

    // Command to compile the generated C file using gcc
    char compile_command[150];

    // Check if the platform is Windows (_WIN32), and use appropriate compile command
    #ifdef _WIN32
        snprintf(compile_command, sizeof(compile_command), "gcc -std=c11 -mconsole -o %s %s", exec_filename, c_filename);
    #else
        // Use the regular gcc command for non-Windows platforms
        snprintf(compile_command, sizeof(compile_command), "gcc -std=c11 -o %s %s", exec_filename, c_filename);
    #endif

    // Compile the C program and check the status
    int compile_status = system(compile_command);
    if (compile_status != 0) {
        // Print error message if compilation failed
        fprintf(stderr, "! Compilation failed.\n");
        unlink(c_filename);  // Delete the generated C file
        return EXIT_FAILURE;  // Exit with failure status
    }

    // Command to execute the compiled C program with any additional arguments
    char exec_command[200] = {0};  // Buffer to hold the execution command
    snprintf(exec_command, sizeof(exec_command), "./%s", exec_filename);  // Prepare the execution command

    // Append any extra arguments passed to the program
    for (int i = 2; i < argc; i++) {
        strcat(exec_command, " ");
        strcat(exec_command, argv[i]);
    }

    // Execute the compiled program and get the status
    int exec_status = system(exec_command);

    // Clean up generated C file and executable after execution
    unlink(c_filename);  // Delete the C file
    unlink(exec_filename);  // Delete the executable file

    // Return success or failure based on the execution status of the compiled program
    return exec_status == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}