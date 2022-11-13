#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <pthread.h>

//Prototype
pid_t my_fork (void);
void print_pids(int fd, short unsigned int N, short unsigned int G);
void count_lines(short unsigned int G);
void print_threads(short unsigned int N);
void *print_me(void *id);

int main (int argc, char* argv[]) {
    int fd = open("out.txt",O_CREAT | O_TRUNC | O_WRONLY , 0666);
    short unsigned int N = atoi(argv[1]); //4
    short unsigned int G = atoi(argv[2]); //0
    print_pids(fd,N,G);
    count_lines(G);
    print_threads(N);
    close(fd);
    exit(EXIT_SUCCESS);
    return 0;
}

pid_t my_fork (void){
    //create a child process if possible and return its p_id
    int fork_id;
    fork_id = fork();
    if (fork_id<0) {
        perror("Cannot fork\n");
        exit(EXIT_FAILURE);
    } else {
        return fork_id;
    }
}

void print_pids(int fd, short unsigned int N, short unsigned int G){
    //create a family tree of process with G generation and N child for each process
    //the output of a process is write to Descriptor-File[fd]
    //the writing of process happens only affter all his children finish writing
    int father_of_all = getpid(); // the father process
    int g=0, n=0, pid;
    while(n<N){
        if (G!=0) pid = my_fork(); //for case of G=0
        if (pid==0){ //is child
            g++;
            if (g==G) { //is last generation
                dprintf(fd,"My pid is %d. My generation is %d\n",getpid(),g);
                break;
            } else {
                n=0;
            }
        } else { //is parent
            n++;
            wait(NULL);//wait for all child process to finish
            if (n==N) dprintf(fd,"My pid is %d. My generation is %d\n",getpid(),g);
        }
    }
    if (getpid() != father_of_all) exit(EXIT_SUCCESS);//all process ent but the father
}

void count_lines(short unsigned int G){
    //create G process, each g process count the number of g generation process
    int father_of_all = getpid();
    char command[40];
    int g, pid, fd = open("out.txt", O_RDONLY);
    for (g=0; g<G; g++){
        pid = my_fork();
        if (pid != 0){ //is parent
            wait(NULL);
            break;
        }
    }
    printf("Number of lines by process of generation %d is ",g);
    fflush(stdout);//flush the pront buffer for printing
    snprintf(command, sizeof(command), "grep -c \"generation is %d\" out.txt",g);
    system(command);
    if (getpid() != father_of_all) exit(EXIT_SUCCESS);//all process ent but the father
}

void *print_me(void *id){
    //print function for threads hendels
    int *i = (int *)id;
    printf("Hi. I'm thread number %d\n",*i);
    return NULL;
}

void print_threads(short unsigned int N){
    //create N treads each one print himself
    int i, vals[N];
    pthread_t tids[N];
    void *retval;
    for (i=0; i<N; i++){
        vals[i] = i;
        pthread_create (&(tids[i]), NULL, print_me, vals+i);
        pthread_join(tids[i], &retval);
    }
}