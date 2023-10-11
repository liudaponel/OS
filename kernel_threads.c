#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#define _GNU_SOURCE
#include <linux/sched.h>
#include <sys/syscall.h>
#include <signal.h>

#include <pthread.h>
#include <errno.h>
#include <sys/types.h>

#define PAGE 4096
#define STACK_SIZE PAGE*8

typedef void  *(*start_routine_t)(void*);

typedef struct _mythread {
	int 				mythread_id;
	start_routine_t		start_routine;
	void* 				arg;
	void* 				retval;
	volatile int		joined;
	volatile int 		exited;
} mythread_t;


int mythread_startup(void* arg){
	printf("1234331243\n");
	mythread_t* mythread = (mythread_t*)arg;
	
	printf("in startup\n");
	mythread->retval = mythread->start_routine(mythread->arg);
	printf("1\n");
	mythread->exited = 1;
	
	//wait until join
	while(!mythread->joined) {
		sleep(1);
		printf("i wait join!!!!! \n");
	}
	return 0;
}

void* create_stack(int size, int thread_num) {
	int stack_fd;
	void* stack;
	char* stack_file = malloc(30);
	
	snprintf(stack_file, sizeof(stack_file), "stack-%d", thread_num);
	
	stack_fd = open(stack_file, O_RDWR | O_CREAT, 0660);
	ftruncate(stack_fd, 0);
	ftruncate(stack_fd, size);
	
	stack = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, stack_fd, 0);
	close(stack_fd);
	
	memset(stack, 0x7f, size);
	
	return stack;
}

int mythread_create(mythread_t* mytid, start_routine_t start_routine, void* arg) {
	static int thread_num = 0;
	
	thread_num++;
	
	void* child_stack = create_stack(STACK_SIZE, thread_num);
	
	mytid->mythread_id = thread_num;
	mytid->start_routine = start_routine;
	mytid->arg = arg;
	mytid->joined = 0;
	mytid->exited = 0;
	
	printf("start clone\n");
	int child_pid = clone(mythread_startup, child_stack + STACK_SIZE, 
				CLONE_VM|CLONE_FILES|CLONE_THREAD|CLONE_SIGHAND|SIGCHLD|
                CLONE_FS|CLONE_PARENT_SETTID|CLONE_CHILD_CLEARTID, (void*)mytid);
	if(child_pid == -1) {
		printf("clone failed");
		return -1;
	}
	sleep(2);
	printf("main: in create (%d)  %d\n", child_pid, gettid());
	
	return 0;
}

int mythread_join(mythread_t* mytid, void** retval){
	mythread_t* mythread = mytid;
	printf("mythread_join\n");
	
	while(!mytid->exited){
		printf("main: sleep\n");
		sleep(1);
	}
	printf("main: mythread finished\n");
	
	*retval = mythread->retval;
	mythread->joined = 1;
	
	printf("222222222222222222222\n");
	
	return 0;
}

void* mythreadFunc(void* arg) {
	char* str = (char*)arg;
	
	for (int i = 0; i < 5; ++i){
		printf("hello: '%s'\n", str);
		sleep(1);
	}

	return NULL;
}

int main() {
	mythread_t tid;
	
	mythread_create(&tid, &mythreadFunc, "argument string 1111");
	printf("main: after create\n");
	
	void* retval;
	mythread_join(&tid, &retval);
	return 0;
}