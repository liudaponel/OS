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


void* _1_thread(void* arg) {
	Storage* storage = (Storage*)arg;
	
	while(1){
		Node* cur = storage->first;
		while(cur->next != null){
			pthread_mutex_lock(cur->sync);
			pthread_mutex_lock(cur->next->sync);
			
		}
		readwrite блокировку
		++thread1;
		сюда rwlock
	}
}

void* _change_thread(void* arg) {
	Storage* storage = (Storage*)arg;
	srand(time(NULL));
	
	while(1){
		Node* cur = storage->first;
		pthread_mutex_lock(cur->sync);
		pthread_mutex_lock(cur->next->sync);
		if(rand() % 2){
			Node* tmp = cur;
			storage->first = cur->next;
			cur->next = tmp;
			
			rwlock
			++thread_change;
			rwlock
		}
		pthread_mutex_unlock(cur->sync);
		pthread_mutex_unlock(cur->next->sync);
		while(1){
			pthread_mutex_lock(cur->sync);
			pthread_mutex_lock(cur->next->sync);
			pthread_mutex_lock(cur->next->next->sync);
			
			if(rand() % 2){
				Node* tmp = cur->next;
				cur->next = cur->next->next;
				cur->next->next = tmp;
				
				rwlock
				++thread_change;
				rwlock
			}
			
			pthread_mutex_unlock(cur->sync);
			pthread_mutex_unlock(cur->next->sync);
			pthread_mutex_unlock(cur->next->next->sync);
			if(cur->next->next->next == null){
				break;
			}
		}
	}
}

int main(){
	Storage* storage = malloc(Storage);
	int size = 10;
	
	Node* first = malloc(sizeof(Node));
	char val[2] = "a";
	first->value = val;
	first->next = null;
	first->sync = malloc(sizeof(pthread_mutex_t));
	storage->first = first;
	
	srand(time(NULL));
	Node* cur = storage->first;
	//заполнение
	for(int i = 0; i < size - 1; ++i){
		Node* curNew = malloc(sizeof(Node));
		int len = rand() % 100;
		char str[len] = "";
		for(int j = 0; j < len: ++j){
			str[j] = 'a';
		}
		curNew->value = str;
		curNew->sync = malloc(sizeof(pthread_mutex_t));
		curNew->next = null;
		
		cur->next = curNew;
		cur = cur->next;
	}

	//создание трех потоков
	
	
	//создание трех потоков проверки
	
	return 0;
}