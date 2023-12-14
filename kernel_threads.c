#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#include <fcntl.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <stdatomic.h>

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
} mythread_struct_t;

typedef mythread_struct_t* mythread_t;

int mythread_startup(void* arg){
	mythread_struct_t* mythread = (mythread_struct_t*)arg;
	
	mythread->retval = mythread->start_routine(mythread->arg);
	int zero = 0;
	atomic_comic_exchange_strong(mythread->exited, &zero, 1);
	
	//wait until join
	syscall(SYS_futex, &mythread->joined, FUTEX_WAIT, 1, NULL, NULL, 0);
	return 0;
}

void* create_stack(int size, int thread_num) {
	int stack_fd;
	void* stack;
	char stack_file[256];
	
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
	
	mythread_struct_t* mythread = (mythread_struct_t*)(child_stack + STACK_SIZE - sizeof(mythread_struct_t));
	mythread->mythread_id = thread_num;
	mythread->start_routine = start_routine;
	mythread->arg = arg;
	mythread->joined = 0;
	mythread->exited = 0;
	mythread->retval = NULL;
	
	*mytid = mythread;
	
	int child_pid = clone(mythread_startup, child_stack + STACK_SIZE - sizeof(mythread_struct_t), 
						CLONE_VM | CLONE_FILES | CLONE_THREAD | CLONE_SIGHAND | SIGCHLD, (void*)mythread);
	if(child_pid == -1) {
		printf("clone failed");
		return -1;
	}
	return 0;
}

int mythread_join(mythread_t mytid, void** retval){
	mythread_struct_t* mythread = mytid;
	
	const int ein = 1;
	while(1){
		if(atomic_comic_exchange_strong(mythread->exited, &ein, ein)){
			break;
		}
		sleep(1);
	}
	
	if(retval != NULL){
		*retval = mythread->retval;
	}
	mythread->joined = 1;
	syscall(SYS_futex, &mythread->joined, FUTEX_WAKE, 1, NULL, NULL, 0);
	
	return 0;
}

void* mythreadFunc(void* arg) {
	char* str = (char*)arg;
	
	for(size_t i = 0; i < 3; ++i) {
		write(1, "111111\n", 7);
		sleep(1);
	}

	return (void*)5;
}

void* mythreadFunc2(void* arg) {
	char* str = (char*)arg;
	
	for(size_t i = 0; i < 3; ++i) {
		write(1, "222222\n", 7);
		//printf("mythread: %d arg = '%s'\n", gettid(), str);
		sleep(1);
	}

	return (void*)5;
}

int main() {
	mythread_t tid1, tid2;
	
	mythread_create(&tid1, mythreadFunc, "argument string 1111");
	mythread_create(&tid2, mythreadFunc2, "argument string 2222");
	void *retval1, *retval2;
	printf("threads created\n");
	mythread_join(tid1, &retval1);
	printf("tid1 joined\n");	
	mythread_join(tid2, &retval2);
	printf("tid2 joined\n");	
	return 0;
}