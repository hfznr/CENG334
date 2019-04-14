#include <stdio.h> 
#include <sys/wait.h> // for wait() 
#include <unistd.h> // for fork() 
#include <stdlib.h> 
#include <string.h> 
#include <errno.h>

void deletePipes(int fd[][2], int fd2[][2], int fd3[][2], int n){
    for(int j=0 ; j<n ; j++){
        close(fd[j][0]);
        close(fd[j][1]);
        close(fd2[j][0]);
        close(fd2[j][1]);   
    }
    for(int j=0 ; j<n-1 ; j++){
        close(fd3[j][0]);
        close(fd3[j][1]);
    }
}

int main(int argc, char const *argv[]){
	int isReduce;
	for(isReduce=0 ; argv[isReduce] ; isReduce++){}

	int mapperCount = atoi(argv[1]);	
    int fd[mapperCount][2],fd2[mapperCount][2],fd3[mapperCount][2];
    pid_t childpid[mapperCount*2];
    int parentPid = getpid();
    char arg[100];

    char line[1000];
    char inputLines[6000][1000];
    int i=0;
    while(fgets(line, sizeof(line), stdin) && line[0] != '\n') {
        strcpy(inputLines[i], line);
        i++;
    }
    if(isReduce==3){
        for(int i=0 ; i<mapperCount ; i++){
            pipe(fd[i]);

            if((childpid[i]=fork())==-1){
                perror("fork");
                exit(1);
            }
            if(childpid[i]!=0){ //Parent
                close(fd[i][0]);
                int j=0;
                while(strcmp(inputLines[j],"")!=0){
                    if(j%mapperCount==i){
                        size_t ll = strlen(inputLines[j]);
                        write(fd[i][1], inputLines[j], ll);
                    }
                    j++;
                }
                close(fd[i][1]);
                waitpid(childpid[i],NULL,0);
            }else{ //Child
                close(fd[i][1]);
                dup2(fd[i][0],STDIN_FILENO);
                close(fd[i][0]);
                char *tmp = malloc(2+strlen(argv[2]));
                strcpy(tmp,"./");
                strcat(tmp,argv[2]);
                execlp(tmp,argv[2], (char*)0);
            }
        }    
    }else{
        for(int i=0; i<mapperCount ; i++){
            if(i!=mapperCount-1){
                pipe(fd[i]);
                pipe(fd2[i]);
                pipe(fd3[i]);    
                continue;
            }
            pipe(fd[i]);
            pipe(fd2[i]);
        }


        for(int i = 0; i<mapperCount ; i++){ //mapper
            if(fork()==0){
                dup2(fd[i][0],0);
                dup2(fd2[i][1],1);
                deletePipes(fd,fd2,fd3,mapperCount);                
                execl(argv[2],argv[2], (char *) NULL);                
            }
        }
        if(parentPid == getpid()){
            for(int i=0 ; i<mapperCount ; i++){ //reducer
                if(fork()==0){
                    if(i==0){
                        dup2(fd2[i][0],0);
                        dup2(fd3[i][1],1);
                        deletePipes(fd,fd2,fd3,mapperCount);
                        execl(argv[3],argv[3], (char *) NULL);    
                    }
                    else if(i!=mapperCount-1 && i!=0){
                        dup2(fd2[i][0],STDIN_FILENO);
                        dup2(fd3[i-1][0], STDERR_FILENO);
                        dup2(fd3[i][1],STDOUT_FILENO);
                        deletePipes(fd,fd2,fd3,mapperCount);
                        execl(argv[3],argv[3],(char *) NULL);

                    }   
                    else{
                        dup2(fd2[i][0],STDIN_FILENO);
                        dup2(fd3[i-1][0], STDERR_FILENO);
                        deletePipes(fd,fd2,fd3,mapperCount);
                        execl(argv[3],argv[3], (char *) NULL);
                    }    
                }
                
            }
        }
        

        if(parentPid == getpid()){
            int c;
            for(int j=0 ; j<mapperCount ; j++){
                if(j!=mapperCount-1){
                    close(fd[j][0]);
                    close(fd2[j][0]); 
                    close(fd2[j][1]);
                    close(fd3[j][0]); 
                    close(fd3[j][1]);
                    continue;    
                }
                close(fd[j][0]);
                close(fd2[j][0]);
                close(fd2[j][1]);
            }
            int x=0;
            while(strcmp(inputLines[x],"")!=0){
                size_t ll = strlen(inputLines[x]);
                write(fd[x%mapperCount][1], inputLines[x], ll);
                x++;
            }   
            for(int j=0 ; j<mapperCount ; j++) close(fd[j][1]);
            for(int j=0 ; j<2*mapperCount ; j++) wait(&c);
        }
    }

    return(0);
}