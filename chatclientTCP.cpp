/*
 * chatclientTCP.c
 *
 *  Created on: Feb 2013
 *      Author: Matteo Ruggero Ronchi
 *
 *  Copyright 2013 Matteo Ruggero Ronchi
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <pthread.h>


#define KILO 1024
#define BUFF_LENGTH 1024
#define PROTO_PORT 60000


int sd;
int served = 0;

void *manage_reading(void *) {

    int n, i;
    char inbuf[BUFF_LENGTH];

    while (served == 0) {

        for (i = 0; i < BUFF_LENGTH; i++) {
            inbuf[i] = 0;
        }

        n = read(sd, inbuf, sizeof(inbuf));

        if (!strcmp(inbuf, "q"))
            served = 1;
        else {
//            printf("Received message!!: %s\n", inbuf);
            printf("[>>>]\n%s\n", inbuf);
        }
    }
    return NULL;
}

int main(int argc, char **argv) {

    if (argc < 3) {
        printf("\nUsage: ./clienttcp 'wanted client host name' 'IP'\nUse 127.0.0.1 as IP if you want to test program on localhost!\n\n");
        return -1;
    }

    struct sockaddr_in sad;
    socklen_t alen;

    int i, n, port, oc = 0;
    char inbuf[BUFF_LENGTH], outbuf[BUFF_LENGTH], outchar;
    char *hostname;
    pthread_t tid;

    for (i = 0; i < BUFF_LENGTH; i++) {
        inbuf[i] = 0;
        outbuf[i] = 0;
    }

    if (argc == 4) {
        port = atoi(argv[3]);
        while (port < 0 || port > 64 * KILO) {
            printf("Bad port number, buond limits are (0,%d)\n\nEnter a new port number: ", 64 * KILO);
            scanf("%d", &port);
        }
    } else {
        port = PROTO_PORT;
    }


    memset((char *) &sad, 0, sizeof(sad));
    sad.sin_family = AF_INET;
    sad.sin_port = htons((u_short) port);
    n = inet_aton(argv[2], &sad.sin_addr);
    printf("\nServer to contact: [%s]:[%d]\n", argv[2], port);
    printf("Server to contact: [%d]:[%d]\n\n", sad.sin_addr.s_addr, sad.sin_port);

    hostname = argv[1];
    printf("Client's hostname is: [%s]\n\n", hostname);

    sd = socket(PF_INET, SOCK_STREAM, 0);

    printf("Connecting to [%d]:[%d]...\n\n", sad.sin_addr.s_addr, sad.sin_port);
//    sleep(0.5);
    connect(sd, (struct sockaddr *) &sad, sizeof(sad));
    printf("Connected to Server: [%d]:[%d]\n", sad.sin_addr.s_addr, sad.sin_port);

    sprintf(outbuf, "HELLO I AM <%s>", hostname);
    write(sd, outbuf, sizeof(outbuf));

    printf("waiting for Server status...\n\n");
//    sleep(0.5);

    n = read(sd, inbuf, sizeof(inbuf));
    if (strcmp(inbuf, "BUSY")) {

        putchar('\n');
        printf("\t**操作说明**\n");
        printf("\t\t- [\33[32;22ml\33[0m]               显示所有取号记录\n");
        printf("\t\t- [\33[32;22mp\33[0m]               显示银行中所有在线客户\n");
        printf("\t\t- [\33[32;22mq\33[0m]               退出\n");
        printf("\t\t- [\33[32;22m#reg\33[0m]            取号\n");
        printf("\t\t- [\33[32;22m#reg borrow <n>\33[0m] 取号 并预约数目为n的借款业务\n");
        printf("\t\t- [\33[32;22m#reg store <n>\33[0m]  取号 并预约数目为n的借款业务\n");
        printf("\t\t- <n>~[1,\33[32;22m%u\33[0m]     存取款数目n的有效范围 \n",UINT16_MAX);

        putchar('\n');
        printf("\t**当前在线**\n");
        while (strcmp(inbuf, "END")) {
            printf("\t\t- %s\n", inbuf);
            n = read(sd, inbuf, sizeof(inbuf));
            oc++;
        }
        putchar('\n');
        printf("\t点击回车进入操作\n\n");

        if (oc == 0) {
            reask:
            printf("No contacts online, do you wish to close connection? (Y/N)\n");
            scanf("%c", &outchar);
            switch (outchar) {
                case 'Y':
                    goto out;

                case 'y':
                    goto out;

                case 'N':
                    break;
                case 'n':
                    break;
                default:
                    printf("wrong character pressed\n");
                    while (getchar() != '\n');
                    goto reask;
                    break;
            }
        }

        if (pthread_create(&tid, NULL, manage_reading,NULL) != 0) {
            perror("Thread creation");
        }

        while (served == 0) {

            for (i = 0; i < BUFF_LENGTH; i++) {
                outbuf[i] = 0;
            }

            while (getchar() != '\n');        //secondo client in poi prima stampa a vuoto...
            scanf("%[^\n]s", outbuf);
            printf("[<<<] %s\n",outbuf);
            write(sd, outbuf, sizeof(outbuf));
            if (!strcmp(outbuf, "q"))
                served = 1;
        }


    } else {
        printf("\nServer Busy, closing connection\n");
        close(sd);
    }

    out:
    printf("Closing connection...\n");

//    sleep(0.5);

    printf("Bye!\n");
//    sleep(0.5);

    close(sd);
    printf("\n\nClient finished\n\n");
}
