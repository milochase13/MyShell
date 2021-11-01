#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

/*
 * Helper function for debugging
 */

void myPrint(char *msg)
{
    write(STDOUT_FILENO, msg, strlen(msg));
}

/*
 * Helper function for redirect search functionality
 */
int redirect_search(char* argv){
    int i = 0;
    while(argv[i]){
        if(argv[i] == 62){
            if(argv[i+1] && argv[i+1] == 43){
                argv[i+1] = ' ';
                return -1;
            }
            return 1;
        }
        i++;
    }
    return 0;
}

/*
 * Helper function to check for white lines
 */
int white_line(char* pinput){
    for (int i =0; i < strlen(pinput); i++){
        if(pinput[i] != ' ' && pinput[i] != '\n' && pinput[i] != '\t'){
            return 0;
        }
    }
    return 1;
}

/*
 * Helper function to check for empty words
 */
int empty_word(char* word){
    if(strlen(word) == 0){
        return 1;
    }
    for(int i = 0; i < strlen(word); i++){
        if((word[i] != ' ') && (word[i] != '\n')&& (word[i] != '\t')&& (word[i] != ';')){
            return 0;
        }
    }
    return 1;
}

/*
 * Helper function to divide command line input into array of single commands
 */
char** divider(char* argv, int* out){
    int count = 0;
    char** divided = (char**)malloc(sizeof(char*)*514);
    char* save;
    char* word = strtok_r(argv, ";\n", &save);
    int i = 0;
    while(word != NULL){
        if(!empty_word(word)){
            divided[i] = word;
            i++;
            count++;
        }
        word = strtok_r(NULL, ";\n", &save);
    }
    
    *out = count;
    return divided;
}

/*
 * Helper function to fix formatting of commands
 */
char** formatter(char* argv){
    char** formatted = (char**)malloc(sizeof(char*)*514);
    char* save;
    char* word = strtok_r(argv, " \t\n", &save);
    int i = 0;
    while(word != NULL){
        formatted[i] = word;
        i++;
        word = strtok_r(NULL, " \t\n", &save);
    }
    formatted[i] = NULL;
    return formatted;
}

/*
 * Handler function for batch files
 */
char** batch_handler(char* file){
    int fd = open(file, O_RDWR);
    char* buffer = (char*)malloc(sizeof(char) * 2000);
    int ammount = read(fd, buffer, 2000);
    char** lines = (char**)malloc(sizeof(char*) * ammount);
    char* save;
    char* line = strtok_r(buffer, "\n", &save);
    int i = 0;
    while(line != NULL){
        lines[i] = line;
        i++;
        line = strtok_r(NULL, "\n", &save);
    }
    return lines;
}

/*
 * Helper function for formatting a command to prime it for redirect functionality
 */
char* clean_redirect(char* str){
    char* out = (char*)malloc(sizeof(char)*strlen(str));
    out = str;
    while(str[0]){
        if(str[0] == '>'){
            out = str+1;
        }
        str++;
    }
    return out;
}

char* clean_redirect2(char* str){
    char* out = (char*)malloc(sizeof(char)*strlen(str));
    int i = 0;
    while(str[i]){
        if(str[i] == '>'){
            str[i] = '\0';
        }
        i++;
    }
    strcpy(out,str);
    return out;
}

/*
 * Helper function to return command before redirect symbol
 */
char* get_redirect_str(char* str){
    int len = strlen(str);
    char* out = (char*)malloc(sizeof(char)*len);
    for(int i = 0; i<len; i++){
        if(!(str[i] == '>')){
            out[i] = str[i];
        }
        else{
            return out;
        }
    }
    return out;
}

void sigexit(){
    exit(0);
}

/*
 * Helper function for parsing a command into an array of string which can be fed into sys.
 */
char** parse(char* input){
    char cpy[514];
    strcpy(cpy, input);
    char** divided = (char**)malloc(sizeof(char*)*514);
    char* save;
    char* word = strtok_r(cpy, " \t;>\n", &save);
    int i = 0;
    int count = 0;
    while(word != NULL){
        if(!empty_word(word)){
            divided[i] = word;
            i++;
            count++;
        }
        word = strtok_r(NULL, " \t;>\n", &save);
    }
    divided[i] = NULL;
    return divided;
}

/*
 * Helper function to get array of commands
 */
char** get_coms(int size, char* line){
    char cpy[514];
    strcpy(cpy, line);
    char** divided = (char**)malloc(sizeof(char*)*(size+1));
    char* save;
    char* word = strtok_r(cpy, ";\n", &save);
    int i = 0;
    while(word != NULL){
        if(!empty_word(word)){
            divided[i] = word;
            i++;
        }
        word = strtok_r(NULL, ";\n", &save);
    }
    divided[i] = NULL;
    return divided;
}


/*
 * Helper function to properly format an array of commands
 */
char** clean(char* line, int* size){
    char* cpy = (char*)malloc(sizeof(char) * 514);
    strcpy(cpy, line);
    char** divided2 = (char**)malloc(sizeof(char*) * 512);
    char* save;
    char* word = strtok_r(cpy, " \t\n>+", &save);
    if(!word){
        return NULL;
    }
    int i = 0;
    while(word != NULL){
        if(!empty_word(word)){
            divided2[i] = word;
            i++;
        }
        word = strtok_r(NULL, " \t\n>+", &save);
    }
    divided2[i] = NULL;
    *size = i - 1;
    return divided2;
}

/*
 * Helper function to keep track of command line parsing status
 */
void word_status(char* input, int* red, int* adv, int* num){
    int before = 0;
    int state = 0;
    int after = 0;
    int first = 0;
    for(int i = 0; i < strlen(input); i++){
        if(input[i] == ';'){
            if(before){
                if(first){
                    state = 1;
                }
                (*num)++;
                state = 0;
                before = 0;
            }
            after = 1;
        }
        else if (input[i] != ' ' && input[i] != '\t' && input[i] != '\n'){
            first = 1;
            before = 1;
            if(state){
                (*num)++;
                state = 0;
                before = 0;
            }
            after = 0;
        }
        if(input[i] == '>'){
            if(input[i+1]&&(input[i+1] == '+')){
                (*adv)++;
            }
            else{
                (*red)++;
            }
        }
    }
    if(after){
        (*num)--;
    }
}

/*
 * Handler for redirection functionality
 */
void redirection_handler(const char* com, char** words, int size){
    const char* file = words[size];
    int fd = open(file, O_RDWR | O_CREAT | O_EXCL, 00600);
        if(fd == -1){
            char error_message[30] = "An error has occurred\n";
            write(STDOUT_FILENO, error_message, strlen(error_message));
            exit(0);
        }
    dup2(fd, STDOUT_FILENO);
    close(fd);
    char** words2 = realloc(words, size+1);
    words2[size] = NULL;
    const char* com2 = (const char*)com;
    int err = execvp(com2, words);
    if(err < 0){
        char error_message[30] = "An error has occurred\n";
        write(STDOUT_FILENO, error_message, strlen(error_message));
        exit(0);
    }
}

/*
 * Handler for advanced redirection functionality
 */
void advanced_handler(const char* com, char** words, int size){
    const char* com2 = (const char*)com;
    const char* file = words[size];
    int fd = open(file, O_RDWR | O_CREAT | O_EXCL | O_TRUNC, 00600);
    if(fd == -1){
        char buffer [200000];
        int fd2 = open(file, O_APPEND | O_RDWR);
        int ammount = read(fd2, buffer, 200000);
        int fdtemp = open(file, O_RDWR | O_CREAT | O_TRUNC, 00600);
        if(errno == 2){
            char error_message[30] = "An error has occurred\n";
            write(STDOUT_FILENO, error_message, strlen(error_message));
            exit(0);
        }
        close(fdtemp);
        dup2(fd2, STDOUT_FILENO);
        int pid2 = fork();
        int status2; //??
        if (pid2 == 0){
            lseek(fd2, 0, SEEK_SET);
            words[size] = NULL;
            execvp(com2, words);
            exit(0);
        }
        else{
            waitpid(pid2, &status2, 0);
        }
        int fd3 = open(file, O_APPEND | O_RDWR); 
        write(fd3, buffer, ammount);
        close(fd);
        close(fd2);
        close(fd3);
        exit(0);
    }
    else{
        dup2(fd, STDOUT_FILENO);
        close(fd);
        char** words2 = realloc(words, size+1);
        words2[size] = NULL;
        const char* com2 = (const char*)com;
        int err = execvp(com2, words);
        if(err < 0){
            char error_message[30] = "An error has occurred\n";
            write(STDOUT_FILENO, error_message, strlen(error_message));
            exit(0);
        }
    }
}

int only_red(char* str){
    for(int i = 0; i < strlen(str); i++){
        if((str[i] != '>') && (str[i] != '+') && (str[i] != ' ') && (str[i] != '\t') && (str[i] != '\n')){
            return 1;
        }
    }
    return 0;
}

/*
 * Execute main process
 */
int main(int argc, char *argv[]) 
{
    char cmd_buff[20000];
    char *pinput;
    int batch = 0;
    FILE * fl;
    char* get;
    char ln [1500];
    
    // Check for edge cases
    
    if(argc > 2){
        char error_message[30] = "An error has occurred\n";
        write(STDOUT_FILENO, error_message, strlen(error_message));
        exit(0);
    }

    if(argv[1]){
        batch = 1;
        fl = fopen(argv[1], "r");
        if(!fl){
            char error_message[30] = "An error has occurred\n";
            write(STDOUT_FILENO, error_message, strlen(error_message));
            exit(0);
        }
    }

    int start = 1;
    
    // Enter execution loop
    
    while (1) {
        
        // If batch
        
        if(batch == 1){
            
            // Read line and parse
            
            get = fgets(ln, 1500, fl);
            if(start && !get){
                exit(0);
            }
            start = 0;
            while(white_line(get)){
                get = fgets(ln, 1500, fl);
            }
            if(get){
                pinput = get;
                if(white_line(pinput)){
                    break;
                }
                else{
                    write(STDOUT_FILENO, pinput, strlen(pinput));
                }
            }
            else{
                break;
                exit(0);
            }
        }
        
        // Otherwise, prepare for another command
        
        else{
            myPrint("myshell> ");
            pinput = fgets(cmd_buff, 20000, stdin);
        }
        if (!pinput) {
            exit(0);
        }
        
        if(strlen(pinput) < 514){
            int red = 0;
            int adv = 0;
            int num = 1;
            int repeat = 0;
            word_status(pinput, &red, &adv, &num);
            char** commands = get_coms(num,pinput);
            for(int i = 0; i < num; i++){
                
                // Builtin commands
                
                int size = 0;
                char** words = clean(commands[i], &size);
                if(!words){
                    char error_message[30] = "An error has occurred\n";
                    write(STDOUT_FILENO, error_message, strlen(error_message));
                    repeat = 1;
                    char* temp[1];
                    temp[0] = "temp";
                    words = temp;
                }
                
                // Custom commands
                
                if(strcmp(words[0], "exit") == 0){
                    if(words[1]){
                        char error_message[30] = "An error has occurred\n";
                        write(STDOUT_FILENO, error_message, strlen(error_message));
                        repeat = 1;
                    }
                    else{
                        exit(0);
                    }
                }
                else if(strcmp(words[0], "cd") == 0){
                    int err = 0;
                    if(words[2]){
                        char error_message[30] = "An error has occurred\n";
                        write(STDOUT_FILENO, error_message, strlen(error_message));
                        repeat = 1;
                    }
                    else{
                        if(words[1]){
                            err = chdir(words[1]);
                            repeat = 1;
                        }
                        else{
                            err = chdir(getenv("HOME"));
                            repeat = 1;
                        }
                        if(err){
                            char error_message[30] = "An error has occurred\n";
                            write(STDOUT_FILENO, error_message, strlen(error_message));
                            repeat = 1;
                        }
                    }
                }
                else if(strcmp(words[0], "pwd") == 0){
                    char buff[512];
                    if(!words[1]){
                        getcwd(buff,512);
                        myPrint(buff);
                        myPrint("\n");
                        repeat = 1;
                    }
                    else{
                        char error_message[30] = "An error has occurred\n";
                        write(STDOUT_FILENO, error_message, strlen(error_message));
                        repeat = 1;
                    }        
                }
                
                //Non builtin Stage
                
                if(repeat == 0){
                    int pid = fork();
                    int status;
                    if (pid == 0){
                        if(red > num || adv > num || commands[i][strlen(commands[i])-1] == '+' || commands[i][strlen(commands[i])-1] == '>') {
                            char error_message[30] = "An error has occurred\n";
                            write(STDOUT_FILENO, error_message, strlen(error_message));
                            exit(0);
                        }
                        if(adv){
                            advanced_handler(words[0], words, size);
                        }
                        else if(red){
                            redirection_handler(words[0], words, size);
                        }
                        else{
                            int error = execvp(words[0], words);
                            if(error == -1){
                                char error_message[30] = "An error has occurred\n";
                                write(STDOUT_FILENO, error_message, strlen(error_message));
                            }
                            exit(0);
                        }
                    }
                    else{
                        waitpid(pid, &status, 0);
                    }
                }
                repeat = 0;
                if(strcmp(words[0], "temp")){
                    free(words);
                }
            }
        }
        else{
            char error_message[30] = "An error has occurred\n";
            write(STDOUT_FILENO, error_message, strlen(error_message));
        }
    }
}
