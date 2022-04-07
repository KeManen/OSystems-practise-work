#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <fcntl.h>
#include <time.h>
#include "wish.h"

void errorHandle(){
    char* errMsg = "An error has occurred\n";
    write(STDERR_FILENO, errMsg, strlen(errMsg));
    exit(1);
}

int readInput(char* buffer, FILE* fp){
    size_t bufsize = BUFSIZE*sizeof(char);
    return getline(&buffer, &bufsize, fp);
}

//parses the input with strsep returns the amount of tokens
Node_t* parseInput(char* buffer, Node_t *tokens){
    int i;
    char* token;
    //remove the newline character
    buffer[strlen(buffer)-1] = '\0';
    for(i = 0; (token = strsep(&buffer, " ")) != NULL; i++){
        tokens = addToList(tokens, token);
    }

    return tokens;
}

int changeDirectory(Node_t* tokens){
    if(getValue(tokens, 1) == NULL){
        printf("No directory specified.\n");
        return 1;
    }

    if(chdir(getValue(tokens, 1)) == -1){
        printf("Directory not found.\n");
        return 1;
    }

    return 0;
}

int setPath(Node_t* tokens, Node_t* path){
    if(getValue(tokens, 1) == NULL){
        printf("No path specified.\n");
        return 1;
    }

    //clear the path
    freeList(path->next);
    //Copy from tokens to path
    for(Node_t* tmp = tokens->next;tmp != NULL; tmp = tmp->next){
        addToList(path, tmp->value);
    }

    return 0;
}


//TODO Nice to have: handle uppers and lowers
int handleShellCommands(Node_t* tokens, Node_t* path){
    if(strcmp(getValue(tokens, 0), "exit") == 0){
        exitShell();
    } else if(strcmp(getValue(tokens, 0), "help") == 0){
        helpText();
        return(0);
    } else if(strcmp(getValue(tokens, 0), "cd") == 0){
        changeDirectory(tokens);
        return(0);
    } else if(strcmp(getValue(tokens, 0), "path") == 0){
        setPath(tokens, path);
        return(0);
    }
    return -1;
}

int checkPath(Node_t* tokens, Node_t* path){
    char checkPath[PATHLENGTH];
    for(Node_t* tmp = path; tmp != NULL ;tmp = tmp->next){
        strcpy(checkPath, tmp->value);
        strcat(checkPath, "/");
        strcat(checkPath, tokens->value);
        if(access(checkPath, F_OK)==0){
            return(0);
        }
    }
    printf("Command not in path\n");
    return(-1);
}

int handleExternalCommands(Node_t* tokens, Node_t* path, int* bgFlag){
    int pid;
    char* argv[PATHLENGTH];
    getArray(tokens, argv);

    //fork
    switch(pid = fork()){
        case -1:
            errorHandle();
        case 0:
            if(checkPath(tokens, path) == 0) if(execvp(argv[0], argv) == -1) errorHandle();
            break;
        default:
            if(bgFlag == 0) if(wait(&pid) == -1) errorHandle();
            break;
    }

    return(0);
}

//shell 
int main(){
    char inputBuffer[BUFSIZE]; 
    Node_t* tokens = NULL;  
    Node_t* path = NULL; 
    int iaFlag = 1;
    int bgFlag = 0;

    //clear arrays
    memset(inputBuffer, 0, BUFSIZE*sizeof(char));

    path = addToList(path, DEFFAULTPATH);

    signal(SIGALRM, autoLogout);

    welcomeText();

    //shell loop
    while(1){
        //alarm(LOGOUTITME); //autologout
        
        if(iaFlag != 0) printf(PROMPT);

        readInput(inputBuffer, stdin);
        tokens = parseInput(inputBuffer, tokens);

        if(handleShellCommands(tokens, path) != 0){
            handleExternalCommands(tokens, path, &bgFlag);
        }

        //clearing buffers
        memset(inputBuffer, 0, BUFSIZE*sizeof(char));
        tokens = freeList(tokens);
    }
}