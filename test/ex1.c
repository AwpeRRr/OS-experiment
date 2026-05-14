#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(){
        int pipefd[2];
        char cachee[256];

        FILE *file;

        pid_t pid = fork();
        if (pid < 0) {
                perror("failed");
                exit(1);
        }
        else if (pid == 0){
                FILE * file = fopen("test1.txt", "r");
                if (file != NULL){
                        printf("Done\n");
                        while (fgets(cachee, sizeof(cachee), file) != NULL){
                                printf("%s", cachee);
                        }
                fclose(file);
                }
                else{
                        printf("Fail\n");
                }

                exit(0);

        }
        else if (pid > 0){
                printf("father is running\n");
                wait(NULL);
                printf("father stop running\n");
        }
        return 0;
}
