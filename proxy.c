#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>
#include <limits.h>
#include <netdb.h>

#define PORT 80
#define BUFF_SIZE 8196
#define THREADS_COUNT 10

pthread_cond_t* cv;
pthread_mutex_t *mutex;
int threads;


void FoundHeaderHost(char* request, char* host){
    char* header_host;
    header_host = strstr(request, "Host: ");
    if (!header_host){
        printf("Host: header not found\n");
        return;
    }
    
    char* header_host_end = strstr(header_host, "\r\n");
    int len = header_host_end - header_host - 6;
    strncpy(host, header_host + 6, len);
    host[len] = '\0';
}

int ConnectToServer(char* host) {
    int sock_server;
    struct sockaddr_in addr;

    struct hostent* host_info = gethostbyname(host);
    sock_server = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr = *((struct in_addr *)host_info->h_addr);

    if (connect(sock_server, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Connect to server error");
        return 0;
    }
    
    return sock_server;
}

void* ClientFunc(void* socket_client) {
    int sock_client = *(int*)socket_client;
    char buff[BUFF_SIZE + 1];
    int sock_server;
    int count_rnrn = 0;

    //первый запрос с хостом сервера
    int size = 0;
    char host[HOST_NAME_MAX + 1];
    while (1) {
        bytes = recv(client_sock, buff, BUFF_SIZE, 0);
        if (bytes <= 0) {
            break;
        }

        if (count_rnrn != 4 && !sock_server) {
            FoundHeaderHost(buff, host);
            if(!host){
                close(sock_client);
                return 0;
            }
            sock_server = ConnectToServer(host);
            if (!sock_server) {
                close(sock_client);
                return NULL;
            }
        }

        // Переотправляем запрос на сервер
        send(sock_server, buff, size, 0);
        
        if (buff[BUFF_SIZE - 3] == '\r' && buff[BUFF_SIZE - 2] == '\n' && buff[BUFF_SIZE - 1] == '\r' && buff[BUFF_SIZE] == '\n'){
            count_rnrn = 4;
        }
        else if (buff[BUFF_SIZE - 2] == '\r' && buff[BUFF_SIZE - 1] == '\n' && buff[BUFF_SIZE] == '\r') {
            count_rnrn = 3;
        }
        else if (buff[BUFF_SIZE - 1] == '\r' && buff[BUFF_SIZE] == '\n') {
            count_rnrn = 2;
        }
        else if (buff[BUFF_SIZE] == '\r') {
            count_rnrn = 1;
        }
        else count_rnrn = 0;
        
        
        if (buff[0] == '\n' && buff[1] == '\r' && buff[2] == '\n' && count_rnrn == 1) {
            count_rnrn = 4;
        }
        else if (buff[0] == '\r' && buff[1] == '\n' && count_rnrn == 2) {
            count_rnrn = 4;
        }
        else if (buff[0] == '\n' && count_rnrn == 3) {
            count_rnrn = 4;
        }
        else count_rnrn = 0;
    }

    // Получаем ответ
    while (bytes > 0) {
        bytes = recv(sock_server, buff, BUFF_SIZE, 0);
        send(sock_client, buff, bytes, 0);
    }

    // Закрытие соединений
    close(sock_client);
    close(sock_server);
    
    pthread_mutex_lock(mutex);
    --threads;
    pthread_cond_signal(cv); 
    pthread_mutex_unlock(mutex);
    return NULL;
}

int main(){
    int sock_proxy, sock_client;
    struct sockaddr_in addr;
    cv = malloc(sizeof(pthread_cond_t));
    pthread_cond_init(cv, NULL);
    mutex = malloc(sizeof(pthread_mutex_t));

    sock_proxy = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    int opt = 1;
    if (setsockopt(sock_proxy, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt error");
        return 0;
    }
    
    if (bind(sock_proxy, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind error");
        return 0;
    }
    
    if (listen(sock_proxy, 5) < 0) {
        perror("listen error");
        return 0;
    }

    printf("HTTP proxy started\n");
    
    pthread_attr_t attr;
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    int len = sizeof(addr);
    while (1) {
        sock_client = accept(sock_proxy, (struct sockaddr*)&addr, &len);
        if (sock_client < 0) {
            perror("accept error");
            continue;
        }

        pthread_t tid;
        pthread_mutex_lock(mutex);
        
		while (threads > THREADS_COUNT){
            pthread_cond_wait(cv, mutex);
        }
		
        if (pthread_create(&tid, &attr, ClientFunc, &sock_client) != 0) {
            perror("pthread_create error");
            continue;
        } else{
            ++threads;
        }
        pthread_mutex_unlock(mutex);
    }
    
    return 0;
}
