#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <netdb.h>
#include <pthread.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/sysinfo.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

#include <set>
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <queue>

#define MAXTHREADS 3
#define KILO 1024
#define BUFF_LENGTH 1000
#define STRLEN 256
#define PROTO_PORT 60000
#define QLEN 1

#define MAX_CONTACTS 3

using namespace std;
int BankMoney = UINTMAX_MAX;
int clientSeq = 1;
int contacts = 0;
int active_socket[MAXTHREADS];
int thread_retval = 0;
int sd;
int endloop;

struct sysinfo sys_info;


char uptimeInfo[15];
unsigned long uptime;

typedef map<int, pthread_t> serverTid;
serverTid sTid,distTid;
pthread_t tid[MAXTHREADS];
pthread_mutex_t readClinetLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queueLock= PTHREAD_MUTEX_INITIALIZER;
int t_mutex;

typedef set<string> setOnlineClient;
setOnlineClient sli;

typedef map<int, pair<unsigned long, string>> registerInfo; //< ID, <register_time, name>>


typedef struct contact {
    int id = 0;
    char usrname[STRLEN];
    char inMsg[STRLEN];
    char contactname[STRLEN];
    int contactsd = 0;
    registerInfo regInfo;
    int money = 0;
    pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;
} contact;

contact onlinecontacts[MAX_CONTACTS];

typedef map<int, contact> clientMap;
clientMap clientDict;


typedef  deque<contact> serverList;
serverList borrowClientDeque,storeClientDeque;

unsigned long checkDeque(serverList cliList);

int getListHeadID(serverList cliList){
    return cliList.front().id;
}

void *serviceBorrow(void *c) {

    pthread_mutex_lock(&queueLock);

    contact usr = *(contact *) c;
    int n, msgMoney;
    int served = 0;
    char inbuf[BUFF_LENGTH], outbuf[BUFF_LENGTH];
    char msg[STRLEN], tmp[STRLEN];
    strcpy(msg, usr.inMsg);

    while (served == 0) {
        bzero(outbuf, BUFF_LENGTH);
        bzero(inbuf, BUFF_LENGTH);
        if (strstr(msg, "borrow")) {
            sscanf(msg, "%*s%s %d", tmp, &msgMoney);
            printf("[serviceBorrow()]>>> money:%d\n", msgMoney);
            if (msgMoney < 0 || msgMoney > UINTMAX_MAX) {
                sprintf(outbuf, "%s", "Invalid Money quantity\nPlease input valid one:\n");
            } else {
                sprintf(outbuf, "Borrow from bank: %d?[y/n]\n", msgMoney);
                write(usr.contactsd, outbuf, sizeof(outbuf));
//                if(!pthread_mutex_trylock(&readClinetLock))
//                    perror("serviceBorrow mutex LOCK failed!");
                n = read(usr.contactsd, inbuf, sizeof(inbuf));
//                if(!pthread_mutex_unlock(&readClinetLock))
//                    perror("serviceBorrow mutex UNLOCK failed!");

                printf("read>>>%d %s\n:", n, inbuf);
//                if(!strncmp(inbuf,"y",1)){
                if (strstr(inbuf, "y")) {
                    BankMoney = BankMoney - msgMoney;
                    usr.money = usr.money + msgMoney;
                    bzero(outbuf, BUFF_LENGTH);
                    sprintf(outbuf, "Borrow success!\nNow your account sum: %d, See you :-)\n", usr.money);
                } else if (!strcmp(inbuf, "n")) {
                    sprintf(outbuf, "There,May you can come next time,Bye.");
                }
                served = 1;
                printf("[>>>]Borrow Done.\n");
            }
        } else if (strstr(msg, "store")) {
            sprintf(outbuf, "In store ser");
            served = 1;
            printf("\n\n>>>>>>>>>>>>STORE\n\n");
        } else {
            sprintf(outbuf, "Invalid Input,EXIT :(");
            served = 1;
        }
    }
    printf("[>>>]SERVED==1 serviceBorrow() exit.\n\n");
    borrowClientDeque.pop_front(); // Serve done: pop the front Client in Queue.
    write(usr.contactsd, outbuf, sizeof(outbuf));

    pthread_mutex_unlock(&queueLock);

    pthread_exit(0);
    return (void *) NULL;
}

void *distServer(void *p) {
    printf("[>>>]Here is *DistServer(void *p)\n");
    contact usr = *(contact *) p;
    int curServerID;
    char inbuf[BUFF_LENGTH], outbuf[BUFF_LENGTH];
    char msg[STRLEN], tmp[STRLEN];
    strcpy(inbuf, usr.inMsg);

    int waitTime = INT32_MAX;

    int done = 0;


    while(!done) {
        if (strstr(inbuf, "borrow")) {
            borrowClientDeque.push_back(usr);

            checkDeque(borrowClientDeque);

//        waitTime = checkList(borrowClientList);
            curServerID = getListHeadID(borrowClientDeque);
            if (curServerID == usr.id) {
                if (0 != pthread_create(&distTid[usr.id], NULL, serviceBorrow, &usr)) {
                    perror("Thread:<serviceBorrow> creation Failed!");
                    distTid[usr.id] = (pthread_t) -1; // to be sure we don't have unknown values... cast
                    break;
                }
                pthread_join(distTid[usr.id], 0);
                done = 1;
            }else {
                sprintf(outbuf,"%s","Wait previous client finish\n");
                printf("[>>>]Wait previous client finish - -");
                write(usr.contactsd,outbuf,sizeof(outbuf));

                pthread_mutex_lock(&queueLock);

                sprintf(outbuf,"%s","You time\n");
                printf("[>>>]Wait previous client finish - -");
                write(usr.contactsd,outbuf,sizeof(outbuf));

                pthread_mutex_unlock(&queueLock);
                continue;
            }

        } else if (strstr(inbuf, "store")) {
            storeClientDeque.push_back(usr);
        }
    }


}


void chat(int sd2) {
    int n, i, served = 0;
    printf("CHAAAAAAAAAAAAT\n");

    char clientname[BUFF_LENGTH];
    char *var;
    char message[BUFF_LENGTH];
    char inbuf[BUFF_LENGTH];
    char outbuf[BUFF_LENGTH];

    while (served == 0) {

        for (i = 0; i < BUFF_LENGTH; i++) {
            inbuf[i] = 0;
            outbuf[i] = 0;
        }
        /*
        if(!pthread_mutex_unlock(&readClinetLock)){
            perror("service_CHAT mutex UNLOCK failed!");
        }
        */
        n = read(sd2, inbuf, sizeof(inbuf));
//        pthread_mutex_lock(&readClinetLock);
//        cout<<"read inbuf size: "<<n<<endl;

        if (!strcmp(inbuf, "q")) {
            sprintf(outbuf, "q");
            clientDict.erase(sd2);
            write(sd2, outbuf, sizeof(outbuf));
            served = 1;
        } else {
            strcpy(clientDict[sd2].inMsg,inbuf);
            //get clientSeq #ID
            if(!strcmp(inbuf,"y")){
                sprintf(outbuf, "In CHAT thread you input: y\n");
                write(sd2, outbuf, sizeof(outbuf));
            }
            else if(strstr(inbuf,"checkBdq")){
                checkDeque(borrowClientDeque);
            }
            else if (!strncmp(inbuf, "#reg", 4)) {
                /*
                for (i = 0; onlinecontacts[i].contactsd != sd2; i++);
                sprintf(outbuf, "Hi <%s>, your #ID is [%d]", onlinecontacts[i].contactname, clientSeq);
                */
                //get register time
                sysinfo(&sys_info);
//                onlinecontacts[i].regInfo[clientSeq].first = (unsigned long)sys_info.uptime;
                clientDict[sd2].regInfo[clientSeq].first = (unsigned long) sys_info.uptime;    // init clientInfo Dict
                clientDict[sd2].regInfo[clientSeq].second = clientDict[sd2].usrname;
                clientDict[sd2].id = clientSeq++;
                sprintf(outbuf, "Hi <%s>, your #ID is [%d]", clientDict[sd2].usrname, clientDict[sd2].id);
                write(sd2, outbuf, sizeof(outbuf));


                if(strstr(inbuf," borrow ") || strstr(inbuf," store")) {
//                    pthread_mutex_unlock(&readClinetLock);
//                    if (pthread_create(&sTid[clientDict[sd2].id], NULL, serviceBorrow, &clientDict[sd2]) != 0) {
                    if (pthread_create(&sTid[clientDict[sd2].id], NULL, distServer, &clientDict[sd2]) != 0) {
                        perror("Thread creation Failed!");
                        sTid[clientDict[sd2].id]= (pthread_t) -1; // to be sure we don't have unknown values... cast
                        continue;
                    }
                    pthread_join(sTid[clientDict[sd2].id],0);
                    printf("pthread_join(sTid[clientDict[sd2].id],0);\n");
                }

            } else if (!strcmp(inbuf, "p")) {
                /*
                sprintf(outbuf, "[%ld client%c online!]:", sli.size(), sli.size() > 1 ? 's' : ' ');
                for (auto k = sli.begin(); k != sli.end(); k++) {
                    printf("Online Clis:>>>%s\n", (*k).c_str());
                    sprintf(outbuf, "%s %s", outbuf, (*k).c_str());
                }
                */
                sprintf(outbuf, "[%ld client%c online!]:", clientDict.size(), clientDict.size() > 1 ? 's' : ' ');
                for (auto it = clientDict.begin(); it != clientDict.end(); it++) {
                    sprintf(outbuf, "%s %s", outbuf, it->second.usrname);
                }
                sprintf(outbuf, "%s%s", outbuf, "\n");
                write(sd2, outbuf, sizeof(outbuf));
            } else if (!strcmp(inbuf, "l")) {
                /** print client Queue Info **/
                sprintf(outbuf, "%s\n", "<ID>\t<Reg Time>\t<User>");
                registerInfo tmp;// = clientDict[sd2].regInfo;
                registerInfo sum;
                for (auto it = clientDict.begin(); it != clientDict.end(); it++)
                    sum.insert(it->second.regInfo.begin(), it->second.regInfo.end());
                for (auto it = sum.begin(); it != sum.end(); it++)
                    sprintf(outbuf, "%s% 2d\t% 7ld\t\t% 3s\n", outbuf, it->first, it->second.first,
                            it->second.second.c_str());
                /*
                for(auto it = clientDict.begin();it!=clientDict.end();it++) {
                    tmp = it->second.regInfo;
                    for (auto k = tmp.begin(); k != tmp.end(); k++) {
                        sprintf(outbuf, "%s% 2d\t% 4ld\t\t% 2s\n", outbuf, k->first, k->second.first,k->second.second.c_str());
                    }
                }
                */
                write(sd2, outbuf, sizeof(outbuf));
            }
                //send private msg
            else if (!strncmp(inbuf, "<", 1)) {
                var = strstr(inbuf, "<");
                i = 0;
                var++;
                while (*var != '>') {
                    clientname[i] = *var;
                    printf("%c", *var);
                    var++;
                    i++;
                }
                clientname[i] = '\0';

                var = strstr(inbuf, ">");
                i = 0;
                var++;
                while (*var != '\0') {
                    message[i] = *var;
                    printf("%c", *var);
                    var++;
                    i++;
                }
                message[i] = '\0';

                printf("\nMessage [%s] is for [%s]\n\n", message, clientname);

//                for (i = 0; onlinecontacts[i].contactsd != sd2; i++);
//                sprintf(outbuf, "<%s> wrote: [%s]", onlinecontacts[i].contactname, message);
                for (i = 0; onlinecontacts[i].contactsd != sd2; i++);
                sprintf(outbuf, "<%s> wrote: [%s]", clientDict[sd2].usrname, message);

                //strcpy(outbuf, message);

                i = 0;
                while (strcmp(onlinecontacts[i].contactname, clientname)) {
                    i++;
                }

                write(onlinecontacts[i].contactsd, outbuf, sizeof(outbuf));

            } else {
                sprintf(outbuf, "In BANK thread you input>>> %s\n",inbuf);
                write(sd2, outbuf, sizeof(outbuf));
                /*
                for (i = 0; onlinecontacts[i].contactsd != sd2; i++);
                sprintf(message, "<%s> wrote: [%s]", onlinecontacts[i].usrname, inbuf);
                strcpy(outbuf, message);
                for (i = 0; i < contacts; i++) {
                    if (onlinecontacts[i].contactsd != sd2)
                        write(onlinecontacts[i].contactsd, outbuf, sizeof(outbuf));
                }
                */
            }


        }
    }
    printf("\nchat has finished\n");

}

void interrupt_handler(int sig) {
    endloop = 1;
    close(sd);
    printf("Interrupt recieved: shutting down server!\n");
    return;
}

void *manage_connection(void *sdp) {

    char inbuf[BUFF_LENGTH];       /* buffer for incoming data  */
    char outbuf[BUFF_LENGTH];    /* buffer for outgoing data  */

    int i;
    int sd2 = *((int *) sdp);
    int j = ((int *) sdp) - active_socket;    /* use pointer arithmetic to get this thread's index in array */
    //int thiscontact = contacts;
    for (i = 0; i < BUFF_LENGTH; i++) {
        inbuf[i] = 0;
        outbuf[i] = 0;
    }

    //printf("\ndentro thread contacts [%d]\n\n", contacts);

//    for (i = 0; i < contacts; i++) {
    for (auto k = clientDict.begin(); k != clientDict.end(); k++) {

        //	printf(" dentro for thread%s - %d\n\n", onlinecontacts[i].contactname, onlinecontacts[i].contactsd);

//        sprintf(outbuf, "[%d]: [%s]\n", i, onlinecontacts[i].contactname);
        registerInfo tmp = k->second.regInfo;
        for (auto m = tmp.begin(); m != tmp.end(); m++) {
            sprintf(outbuf, "[%d]: [%s]: [%ld]", m->first, m->second.second.c_str(), m->second.first);
        }
        //printf("[%s]\n\n", buffer);
        write(sd2, outbuf, sizeof(outbuf));
    }

    sprintf(outbuf, "END");
    write(sd2, outbuf, sizeof(outbuf));

    printf("-(IN THREAD)- sent online contacts\n");

    printf("-(IN THREAD)- simulazione di chat\n");


    chat(sd2);

    tid[j] = (pthread_t) -1; // free thread array entry

    printf("-(IN THREAD)- close sd2\n");
    for (i = 0; onlinecontacts[i].contactsd != sd2; i++); // find sd2's name
    sli.erase(onlinecontacts[i].usrname); // erase name from set<char*>
    close(sd2);
    contacts--;
    //onlinecontacts[thiscontact].contactname = "removed";
    //onlinecontacts[thiscontact].contactsd = 0;
    return &thread_retval;

}

int main(int argc, char **argv) {

    if (argc < 2) {
        printf("\nUsage: ./servertcp 'IP' ['port'] \nUse 127.0.0.1 as IP if you want to test program on localhost, port number is optional!\n\n");
        return 0;
    }

    struct sockaddr_in sad;
    struct sockaddr_in cad;
    socklen_t alen;

    contact newcontact;

    int sd2, port, n, i, j = 0;
    char *var;
    char clientname[256];
    char busymsg[] = "BUSY";
    char buffer[BUFF_LENGTH];

    // init thread_t status
    for (i = 0; i < MAXTHREADS; i++) {
        tid[i] = (pthread_t) -1;
//        sTid[i] = (pthread_t) -1;
//        distTid[i] = (pthread_t ) -1;
        active_socket[i] = -1;
    }

    for (i = 0; i < BUFF_LENGTH; i++) {
        buffer[i] = 0;
    }

    if (argc == 3) {
        port = atoi(argv[2]);
        while (port < 0 || port > 64 * KILO) {
            printf("Bad port number, buond limits are (0,%d)\n\nEnter a new port number: ", 64 * KILO);
            scanf("%d", &port);
        }
    } else {
        port = PROTO_PORT;
    }

    memset((char *) &sad, 0, sizeof(sad));
    sad.sin_family = AF_INET;
    n = inet_aton(argv[1], &sad.sin_addr);
    sad.sin_port = htons((u_short) port);

    printf("Server IP address and service port: [%s]:[%d]\n", argv[1], port);
    printf("Server IP address and Service Port [%d]:[%d]\n\n", sad.sin_addr.s_addr, sad.sin_port);

    sd = socket(PF_INET, SOCK_STREAM, 0);
    if (sd < 0) {
        perror("Socket creation failed\n");
        exit(1);
    }
    printf("Socket created with sd: [%d]\n\n", sd);

    n = bind(sd, (struct sockaddr *) &sad, sizeof(sad));
    if (n == -1) {
        perror("Error in Bind\n");
        exit(1);
    }

    n = listen(sd, QLEN);
    if (n < 0) {
        perror("Listen failed\n");
        exit(1);
    }

    signal(SIGINT, interrupt_handler);

    printf("Server in the service loop\n");

    while (!endloop) {

        alen = sizeof(cad);

        printf("Server is waiting for a Client to serve...\n");

        sd2 = accept(sd, (struct sockaddr *) &cad, &alen);
        if (sd2 < 0) {
            if (endloop) break;
            perror("Accept failed\n");
            exit(1);
        }

        if (contacts < MAXTHREADS) {

            printf("Connection with Client: [%s]:[%hu] - sd:[%d]\n", inet_ntoa(cad.sin_addr), ntohs(cad.sin_port), sd2);
            printf("This server is serving [%d] client%c\n", contacts + 1, (contacts > 0) ? 's' : ' ');

            n = read(sd2, buffer, sizeof(buffer));
            printf("Message from client: [%s]\n", buffer);

            var = strstr(buffer, "<");
            i = 0;
            var++;
            while (*var != '>') {
                clientname[i] = *var;
                printf("%c", *var);
                var++;
                i++;
            }
            clientname[i] = '\0';

            printf("\nClient name is:  [%s]\n\n", clientname);

            strcpy(newcontact.usrname, clientname);
            string tmp = clientname;
            sli.insert(tmp);
            newcontact.contactsd = sd2;  //<-- init client parameter
            newcontact.money = 600;
//			newcontact.id=-1;
            onlinecontacts[contacts] = newcontact;
            clientDict[sd2] = newcontact;

            //look for the first empty slot in thread array
            for (i = 0; tid[i] != (pthread_t) -1; i++);

            /*the use of different variables for storing socket ids
            * avoids potential race conditions between the access
            * to the value of &sd2 in the new started thread and
            * the assignement by the connect() function call above.
            * By Murphy's laws it my happen that the thread reads
            * the variable pointed by its argument value after
            * accept has stored a new value in sd2, thus loosing the
            * previously opened socket.
            */


            //printf(" prima if pthread %d\n\n", contacts);
            active_socket[i] = sd2;

            if (pthread_create(&tid[i], NULL, manage_connection, &active_socket[i]) != 0) {
                perror("Thread creation");
                tid[i] = (pthread_t) -1; // to be sure we don't have unknown values... cast
                continue;
            }
            contacts++;

        } else {  //too many threads
            printf("Maximum threads active: closing connection\n");
            write(sd2, busymsg, strlen(busymsg) + 1);
            close(sd2);
        }

        printf("nuovo thread attivato\n");

    }
    printf("Server finished\n");
}

unsigned long checkDeque(serverList cliList) {
    contact usr;
    unsigned long t=0;
    cout<<"-------checkDeque---------"<<endl;
    cout<<"[Check List>>>]"<<endl;
    for(auto it = cliList.begin();it!=cliList.end();it++){
        printf("%d %s %ld\n", it->id, it->usrname, it->regInfo[it->id].first);
        t = t+it->regInfo[it->id].first;
    }
    cout<<"*******checkDeque*********"<<endl;
    return t;
}
