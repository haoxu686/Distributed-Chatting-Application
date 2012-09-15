#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "Model.h"
#include "Timer.h"

using namespace std;

int command;
string name;
string contactIp;
uint16_t contactPort;

int parseArguments(char *argv[], int argc) {
    if (argc < 2) {
        cout << "Insufficient argument" << endl;
        return -1;
    }
    name.assign(argv[1]);
    command =  1;
    if (argc == 3) {
        command = 2;
        char *p = argv[2];
        while (*p != '\0') {
            if (*p == ':') {
                break;
            }
            p++;
        }
        if (*p == '\0') {
            cout << "Malformed address" << endl;
            return -1;
        }
        *p = '\0';
        contactPort = atoi(p+1);
        if (contactPort == 0) {
            cout << "Malformed address: port" << endl;
            return -1;
        }
        struct sockaddr_in contactAddr;
        bzero((char *) &contactAddr, sizeof(contactAddr));
        int result = inet_pton(AF_INET, argv[2], &(contactAddr.sin_addr));
        if (result == -1) {
           cout << "Malformed address: ip" << endl;
           return -1;
        }
        contactIp.assign(argv[2]);
    }
    return 0;
}

Model *model;
Timer *timer;

void * doInput(void *arg) {
	string input;
	char buffer[1000];
	while (true) {
		cin.getline(buffer, 1000);
		input.assign(buffer);
	    if (input.compare("EXIT") == 0 || input.compare("exit") == 0) {
	    	break;
	    }
	    model->chat(input);
	}
	return NULL;
}

void * doListenMessage(void *arg) {
	model->doListenMessage();
	return NULL;
}

void * doSend(void *arg) {
	model->doSend();
	return NULL;
}

void doTimer(int signum) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	int64_t a = (int64_t)tv.tv_sec*1000000+tv.tv_usec;
	timer->doTimer();
	gettimeofday(&tv, NULL);
	int64_t b = (int64_t)tv.tv_sec*1000000+tv.tv_usec;
	//cout << b-a << endl;
	alarm(1);
}

int main(int argc, char *argv[]) {
    int result = parseArguments(argv, argc);
    if (result == -1) {
        return 0;
    }

    timer = new Timer();
    if (command == 1) {
        model = new Model(name, timer);
    } else if (command == 2) {
    	model = new Model(name, contactIp, htons(contactPort), timer);
    }

    pthread_t tids[3];
    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&tids[0], &thread_attr, doListenMessage, (void *)(0));
    pthread_create(&tids[1], &thread_attr, doSend, (void *)(1));
    pthread_create(&tids[2], &thread_attr, doInput, (void *)(2));

    struct sigaction sa;
    sa.sa_handler = doTimer;
    sigaction(SIGALRM, &sa, NULL);
    alarm(1);
    pthread_join(tids[2], NULL);
    alarm(0);
    delete timer;
    delete model;
    return 0;
}


