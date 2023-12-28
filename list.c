#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>


typedef struct _Node {
	char 				value[100];
	struct 				_Node* next;
	pthread_mutex_t* 	sync;
} Node;

typedef struct _Storage {
	Node 				*first;
} Storage;


int thread1 = 0;
int thread2 = 0;
int thread3 = 0;
int thread_change = 0;

pthread_mutex_t* changer_sync;
pthread_mutex_t* storage_sync;


void* _1_thread(void* arg) {
	Storage* storage = (Storage*)arg;
	
	while(1){
		pthread_mutex_lock(storage_sync);
		Node* cur = storage->first;
		pthread_mutex_unlock(storage_sync);
		
		pthread_mutex_lock(cur->sync);
		while(cur->next != NULL){
			pthread_mutex_lock(cur->next->sync);
			if(strlen(cur->value) < strlen(cur->next->value)){
				
			}
			Node* curLast = cur;
			cur = cur->next;
			pthread_mutex_unlock(curLast->sync);
		}
		pthread_mutex_unlock(cur->sync);
		++thread1;
	}
}

void* _2_thread(void* arg) {
	Storage* storage = (Storage*)arg;
	
	while(1){
		pthread_mutex_lock(storage_sync);
		Node* cur = storage->first;
		pthread_mutex_unlock(storage_sync);
		
		pthread_mutex_lock(cur->sync);
		while(cur->next != NULL){
			pthread_mutex_lock(cur->next->sync);
			if(strlen(cur->value) > strlen(cur->next->value)){
				
			}
			Node* curLast = cur;
			cur = cur->next;
			pthread_mutex_unlock(curLast->sync);
		}
		pthread_mutex_unlock(cur->sync);
		++thread2;
	}
}

void* _3_thread(void* arg) {
	Storage* storage = (Storage*)arg;
	
	while(1){
		pthread_mutex_lock(storage_sync);
		Node* cur = storage->first;
		pthread_mutex_unlock(storage_sync);
		
		pthread_mutex_lock(cur->sync);
		while(cur->next != NULL){
			pthread_mutex_lock(cur->next->sync);
			if(strlen(cur->value) == strlen(cur->next->value)){
				
			}
			Node* curLast = cur;
			cur = cur->next;
			pthread_mutex_unlock(curLast->sync);
		}
		pthread_mutex_unlock(cur->sync);
		++thread3;
	}
}

void* _change_thread(void* arg) {
	Storage* storage = (Storage*)arg;
	srand(time(NULL));
	
	while(1){
		pthread_mutex_lock(storage_sync);
		Node* cur = storage->first;
		
		pthread_mutex_lock(cur->sync);
		pthread_mutex_lock(cur->next->sync);
		if(rand() % 2){
			Node* c = cur;
			Node* cn = cur->next;
			Node* cnn = cur->next->next;
			
			storage->first = cn;
			cur = storage->first;
			cn->next = c;
			c->next = cnn;
			
			pthread_mutex_lock(changer_sync);
			++thread_change;
			pthread_mutex_unlock(changer_sync);
		}
		pthread_mutex_unlock(storage_sync);
		while(1){
			pthread_mutex_lock(cur->next->next->sync);
			if(rand() % 2){
				Node* cnnn = cur->next->next->next;
				Node* cn = cur->next;
				Node* cnn = cur->next->next;
				
				cur->next = cnn;
				cur->next->next = cn;
				cur->next->next->next = cnnn;
				
				pthread_mutex_lock(changer_sync);
				++thread_change;
				pthread_mutex_unlock(changer_sync);
			}
			Node* curLast = cur;
			cur = cur->next;
			pthread_mutex_unlock(curLast->sync);
			if(cur->next->next == NULL){
				break;
			}
		}
		pthread_mutex_unlock(cur->sync);
		pthread_mutex_unlock(cur->next->sync);
	}
}

void *qmonitor(void *arg) {
	printf("qmonitor: [%d %d %d]\n", getpid(), getppid(), gettid());

	while (1) {
		pthread_mutex_lock(changer_sync);
		printf("thread1: %d   thread2: %d   thread3: %d      thread_change: %d\n", thread1, thread2, thread3, thread_change);
		pthread_mutex_unlock(changer_sync);
		sleep(1);
	}

	return NULL;
}

int main(){
	Storage* storage = malloc(sizeof(Storage));
	int size = 1000000;
	int err;
	pthread_t tid;
	
	Node* first = malloc(sizeof(Node));
	first->value[0] = 'a';
	first->next = NULL;
	first->sync = malloc(sizeof(pthread_mutex_t));
	storage->first = first;
	
	srand(time(NULL));
	Node* cur = storage->first;
	//заполнение
	for(int i = 0; i < size - 1; ++i){
		Node* curNew = malloc(sizeof(Node));
		int len = rand() % 100;
		//int len = i + 2;
		for(int j = 0; j < len; ++j){
			curNew->value[j] = 'a';
		}
		curNew->sync = malloc(sizeof(pthread_mutex_t));
		curNew->next = NULL;
		
		cur->next = curNew;
		cur = cur->next;
	}
	changer_sync = malloc(sizeof(pthread_mutex_t));
	storage_sync = malloc(sizeof(pthread_mutex_t));

	//создание трех потоков
	err = pthread_create(&tid, NULL, _1_thread, storage);
	if (err) {
		printf("main: pthread_create() failed: %s\n", strerror(err));
		return -1;
	}
	err = pthread_create(&tid, NULL, _2_thread, storage);
	if (err) {
		printf("main: pthread_create() failed: %s\n", strerror(err));
		return -1;
	}
	err = pthread_create(&tid, NULL, _3_thread, storage);
	if (err) {
		printf("main: pthread_create() failed: %s\n", strerror(err));
		return -1;
	}
	
	//создание трех потоков проверки
	err = pthread_create(&tid, NULL, _change_thread, storage);
	if (err) {
		printf("main: pthread_create() failed: %s\n", strerror(err));
		return -1;
	}
	err = pthread_create(&tid, NULL, _change_thread, storage);
	if (err) {
		printf("main: pthread_create() failed: %s\n", strerror(err));
		return -1;
	}
	err = pthread_create(&tid, NULL, _change_thread, storage);
	if (err) {
		printf("main: pthread_create() failed: %s\n", strerror(err));
		return -1;
	}
	
	err = pthread_create(&tid, NULL, qmonitor, NULL);
	if (err) {
		printf("main: pthread_create() failed: %s\n", strerror(err));
		return -1;
	}
	
	pthread_exit(NULL);
	return 0;
}