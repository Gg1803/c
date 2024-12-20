//  CITS2002 Project 1 2024

//  Student1:   23862402    Kavishani Vimalathasan
//  Student2:   23887876    Gargi Garg

//  Platform:   Apple

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <unistd.h>  // For getpid()
#include <ctype.h>
#define line_length 256  // Define the maximum line length for input

// Function to check the file extension
// Ensures the input file has a '.ml' extension
int check_extension(const char *filename)
{
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return 0;
    else return (strcmp(dot, ".ml") == 0);
}

// Report syntax error to stderr with '!' prefix
void report_syntax_error(const char *message) {
    fprintf(stderr, "! %s\n", message);
}

// Report debug information to stdout with '@' prefix
void report_debug_info(const char *message) {
    printf("@ %s\n", message);
}

// Function to delete a file, return 0 on success
int file_deletion(const char *file)
{
    if (remove(file) == 0)
    {
        return 0;
    }
    else return 1;
}

// Validate variable name: must be lowercase, alphabetic, and 1-12 characters long
bool variable_name_validation(char *variable)
{
    if (islower(variable[0]) && isalpha(variable[0]) && (strlen(variable) >= 1) && (strlen(variable) <= 12))
    {
        return true;
    }
    else
    {
        report_syntax_error("Invalid Variable Name.");
        return false;
    }
}

// Translate assignment statements from mini-language to C
// Example: 'x <- 5' translates to 'double x = 5.0;'
void translate_assignment_statement(char *line, FILE *c_fptr)
{
    char variable_name[50];
    double value = 0.0;
    char expression[line_length];

    if (sscanf(line, "%49s <- %lf", variable_name, &value) == 2)
    {
        if (variable_name_validation(variable_name))
            fprintf(c_fptr, "   double %s = %.1f;\n", variable_name, value);
    }
    else if (sscanf(line, "%s <- %[^\n]", variable_name, expression) == 2)
    {
        if (variable_name_validation(variable_name))
            fprintf(c_fptr, "   double %s = %s;\n", variable_name, expression);
    }
    else {
        report_syntax_error("Invalid assignment statement.");
    }
}

// Translate print statements from mini-language to C
// Example: 'print x' translates to C code that checks if x is an integer or floating-point and prints accordingly
void translate_print_statement(char *line, FILE *c_fptr)
{
    char arguments[line_length];

    // Trim leading whitespace and check for the print keyword
    char *trimmed_line = line;
    while (*trimmed_line == ' ' || *trimmed_line == '\t') {
        trimmed_line++;
    }

    // Check if line starts with 'print'
    if (strncmp(trimmed_line, "print", 5) == 0) {
        // Move pointer to the part after 'print'
        trimmed_line += 5;
        
        // Skip leading spaces after 'print'
        while (*trimmed_line == ' ' || *trimmed_line == '\t') {
            trimmed_line++;
        }

        // Capture everything after 'print' as the arguments
        strncpy(arguments, trimmed_line, sizeof(arguments));

        // Generate C code to differentiate between integers and floating-point numbers
        fprintf(c_fptr, "    if (floor(%s) == %s) {\n", arguments, arguments);
        fprintf(c_fptr, "        printf(\"%%.0f\\n\", %s);\n", arguments);  // Print integer (no decimal places)
        fprintf(c_fptr, "    } else {\n");
        fprintf(c_fptr, "        printf(\"%%.6f\\n\", %s);\n", arguments);  // Print floating-point (6 decimal places)
        fprintf(c_fptr, "    }\n");
    } else {
        report_syntax_error("Invalid print statement.");
    }

}

// Translate return statements from mini-language to C
// Example: 'return x' translates to 'return x;'
void translate_return_statement(char *line, FILE *c_fptr)
{
    char *return_keyword = "return";
    char *return_expression;

    // Check if "return" exists in the line
    if ((return_expression = strstr(line, return_keyword)) != NULL)
    {
        // Move pointer to the part after "return"
        return_expression += strlen(return_keyword);
        
        // Remove leading spaces after "return"
        while (*return_expression == ' ') return_expression++;

        // Write the return statement to the C file
        fprintf(c_fptr, "   return %s; \n", return_expression);
    } else {
        report_syntax_error("Invalid return statement.");
    }
}

// Function to detect if a function returns a value (double) or is void
// This function scans through the mini-language file to identify functions and return statements
bool function_type(FILE *ml_fptr)
{
    char line[line_length];
    bool is_function = false;
    bool function_type = false;

    while (fgets(line, line_length, ml_fptr))
    {
        // Check if it is a function definition
        if (strncmp(line, "function", 8) == 0)
        {
            is_function = true;
            report_debug_info("Function definition detected.");
        }
        // Check for return statement inside a function
        else if (is_function && strstr(line, "return") != NULL)
        {
            function_type = true; // If return statement found, set flag to true
            report_debug_info("Return statement detected in function.");
        }
        // End of function block
        else if (is_function && (strncmp(line, "#", 1) == 0 || strncmp(line, "\n", 1) == 0))
        {
            is_function = false;
            report_debug_info("End of function block detected.");
        }
    }

    fclose(ml_fptr);  // Close the mini-language file
    return function_type;
}

// Translate function definitions from mini-language to C
// Handles the translation of function name, parameters, and body
void translate_function_definition(char *line, FILE *ml_extra, FILE *c_fptr, bool function_condition)
{
    char function_name[50];
    char parameters[line_length];
    char parameter_list[line_length] = "";
    char function_line[line_length];

    // Extract function name and parameters from the mini-language code
    sscanf(line, "function %s %[^\n]", function_name, parameters);

    if (sscanf(line, "function %s %[^\n]", function_name, parameters) < 1) {
        report_syntax_error("Invalid function definition.");
        return;
    }

    // Convert parameters into C-style parameters
    char *elements = strtok(parameters, " ");  // Split the parameters using " " (space) delimiter
    while (elements != NULL)  // Loop until there are no more elements/parameters
    {
        if (strlen(parameter_list) > 0) 
            strcat(parameter_list, ", ");
        sprintf(parameter_list + strlen(parameter_list), "double %s", elements);
        elements = strtok(NULL, " ");
    }

    // Write the function signature to the C file
    if (!function_condition)
        fprintf(c_fptr, "void %s(%s) {\n", function_name, parameter_list);
    else
        fprintf(c_fptr, "double %s(%s) {\n", function_name, parameter_list);

    // Translate the function body
    while (fgets(function_line, sizeof(function_line), ml_extra))
    {
        // Translate assignment statements, return statements, print statements, or ignore comments
        if (strstr(function_line, "<-"))
        {
            translate_assignment_statement(function_line, c_fptr);
        }
        else if (strstr(function_line, "return"))
        {
            translate_return_statement(function_line, c_fptr);
        }
        else if (strstr(function_line, "print"))
        {
            translate_print_statement(function_line, c_fptr);
        }
        else if (strstr(function_line, "#"))
            break;  // End of function body when a comment is detected
        else {
            report_syntax_error("Invalid function body statement.");
        }
    }

    // Close function definition
    fprintf(c_fptr, "}\n\n");
}

// Function to translate mini-language code to C code
// Handles both global and function-level statements
bool translate_ml_to_c(FILE *ml_extra, FILE *c_fptr, bool *function_return, bool main_start)
{
    char function_name[50];
    char arguments[line_length];
    char line[line_length];

    while (fgets(line, sizeof(line), ml_extra))
    {
        // Ignore comments or empty lines
        if (line[0] == '#' || line[0] == '\n') continue;

        // Handle function definitions
        else if (strncmp(line, "function", 8) == 0)
        {
            translate_function_definition(line, ml_extra, c_fptr, *function_return);
        }
        else
        {
                        // If the main function hasn't started yet, start it
            if (!main_start)
            {
                fprintf(c_fptr, "int main() {\n");  // Start the main function
                main_start = true;
            }

            // Handle print statements
            if (strncmp(line, "print ", 6) == 0)
            {
                translate_print_statement(line, c_fptr);
            }
            // Handle assignment statements
            else if (strstr(line, "<-"))
            {
                translate_assignment_statement(line, c_fptr);
            }
            // Handle function calls with parameters
            else if (sscanf(line, "%[^()](%[^)])", function_name, arguments) == 2)
            {
                fprintf(c_fptr, "   %s(%s);\n", function_name, arguments);  // Call the function with arguments
            }
            // Handle function calls without parameters
            else if (sscanf(line, "%[^()]", function_name) == 1)
            {
                fprintf(c_fptr, "   %s();\n", function_name);  // Call the function without arguments
            }
            else
            {
                report_syntax_error("Invalid statement outside function.");
            }
        }
    }

    // If the main function started, close it properly
    if (main_start)
    {
        fprintf(c_fptr, "   return 0;\n}\n");  // Ensure the main function ends with a return 0
    }

    return main_start;  // Return whether the main function was created or not
}

int main(int argc, char *argv[])
{
    // Ensure that exactly one argument is provided (the .ml file)
    if (argc != 2)
    {
        fprintf(stderr, "%s: program expected 1 argument, but instead received %i\n", argv[0], argc-1);
        exit(EXIT_FAILURE);
    }

    // Check if the provided file has the correct .ml extension
    if (!check_extension(argv[1]))
    {
        fprintf(stderr, "%s: Invalid file extension, expected a '.ml' file.\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Check if the function has a return type by analyzing the mini-language file
    FILE *ml_fptr = fopen(argv[1], "r");
    if (!ml_fptr)
    {
        perror("! Could not open input file");
        exit(EXIT_FAILURE);
    }
    bool function_return = function_type(ml_fptr);
    fclose(ml_fptr);

    // Generate unique filenames for the C file and executable based on process ID
    int pid = getpid();
    char c_filename[20], exec_filename[20];
    snprintf(c_filename, sizeof(c_filename), "ml-%d.c", pid);
    snprintf(exec_filename, sizeof(exec_filename), "ml-%d", pid);

    FILE *c_fptr = fopen(c_filename, "w");
    if (!c_fptr)
    {
        perror("! Could not create C output file");
        return EXIT_FAILURE;
    }

    // Write necessary C headers to the generated file
    fprintf(c_fptr, "#include <stdio.h>\n#include <stdlib.h>\n#include <math.h>\n\n");

    // Re-open the mini-language file for reading and create a temporary file
    ml_fptr = fopen(argv[1], "r");
    if (!ml_fptr)
    {
        perror("! Could not re-open input file");
        fclose(c_fptr);
        exit(EXIT_FAILURE);
    }

    // Process the mini-language file, extracting functions into a temporary file if needed
    FILE *ml_file = fopen("new.ml", "w");
    if (!ml_file)
    {
        perror("! Could not create temporary file");
        fclose(ml_fptr);
        fclose(c_fptr);
        exit(EXIT_FAILURE);
    }

    char line[line_length];
    bool function_exist = false;

    // First pass to handle functions and assignments
    while (fgets(line, sizeof(line), ml_fptr))
    {
        if (strncmp(line, "function", 8) == 0)
        {
            function_exist = true;
            fprintf(ml_file, "%s", line);
        }
        else if (strstr(line, "<-"))
        {
            if (!function_exist)
                translate_assignment_statement(line, c_fptr);  // Handle global assignments
            else
                fprintf(ml_file, "%s", line);  // Store function body in the temporary file
        }
        else
        {
            fprintf(ml_file, "%s", line);  // Store non-assignment statements in the temporary file
        }
    }

    fclose(ml_fptr);
    fclose(ml_file);

    // Second pass to process the temporary file
    FILE *ml_extra = fopen("new.ml", "r");
    if (!ml_extra)
    {
        perror("! Could not re-open temporary file");
        fclose(c_fptr);
        exit(EXIT_FAILURE);
    }

    // Translate the mini-language to C
    bool main_start = translate_ml_to_c(ml_extra, c_fptr, &function_return, false);
    fclose(ml_extra);

    // If no main function has been created, create a default one
    if (!main_start)
    {
        fprintf(c_fptr, "int main() {\n");
        fprintf(c_fptr, "   return 0;\n}\n");
    }

    fclose(c_fptr);

    // Delete the temporary file
    file_deletion("new.ml");

    // Compile the generated C file using gcc
    char compile_command[150];
    #ifdef _WIN32
        snprintf(compile_command, sizeof(compile_command), "gcc -std=c11 -mconsole -o %s %s", exec_filename, c_filename);
    #else
        snprintf(compile_command, sizeof(compile_command), "gcc -std=c11 -o %s %s", exec_filename, c_filename);
    #endif

    int compile_status = system(compile_command);
    if (compile_status != 0)
    {
        fprintf(stderr, "! Compilation failed.\n");
        unlink(c_filename);  // Clean up generated C file on failure
        return EXIT_FAILURE;
    }

    // Execute the compiled program
    char exec_command[200] = {0};
    snprintf(exec_command, sizeof(exec_command), "./%s", exec_filename);
    for (int i = 2; i < argc; i++) {
        strcat(exec_command, " ");
        strcat(exec_command, argv[i]);
    }
    int exec_status = system(exec_command);

    // Clean up generated files after execution
    unlink(c_filename);
    unlink(exec_filename);

    // Return the execution status
    return exec_status == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
