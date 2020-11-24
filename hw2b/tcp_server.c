#include "info.h"
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include<fcntl.h>
struct information{
    SOCKET id;
    unsigned int in;
    char name[1024];
    char password[1024];
    int state;
    int oppnent;
    int oppfind;
    char board[9];
    char bit[100];
    char sysmbol[2];
    int turn;
    int filled[9];
    int win;
    int counter;
    int ingame;
    char more[4];
    int win1;
    int total;
    int watching;
    int watchingguy[10];
    int w;
    int chatid;
    int login;
};
int turn=0;
void playgame(SOCKET fd1,SOCKET fd2);
struct information user[100];
int num = 0;
int main() {

#if defined(_WIN32)
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to initialize.\n");
        return 1;
    }
#endif


    printf("Configuring local address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *bind_address;
    getaddrinfo(0, "8080", &hints, &bind_address);


    printf("Creating socket...\n");
    SOCKET socket_listen;
    socket_listen = socket(bind_address->ai_family,
            bind_address->ai_socktype, bind_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_listen)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }


    printf("Binding socket to local address...\n");
    if (bind(socket_listen,
                bind_address->ai_addr, bind_address->ai_addrlen)) {
        fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    freeaddrinfo(bind_address);


    printf("Listening...\n");
    if (listen(socket_listen, 10) < 0) {
        fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    fd_set master;
    FD_ZERO(&master);
    FD_SET(socket_listen, &master);
    SOCKET max_socket = socket_listen;
    
    printf("Waiting for connections...\n");
    for(int i=0;i<30;i++){
        user[i].id=-1;
        user[i].in=-1;
        user[i].login=-1;
        user[i].ingame=0;;
    }

    while(1) {
        fd_set reads;
        reads = master;
        if (select(max_socket+1, &reads, 0, 0, 0) < 0) {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
            return 1;
        }

        SOCKET i;
        for(i = 1; i <= max_socket; ++i) {
            if (FD_ISSET(i, &reads)){

                if (i == socket_listen) {
                    struct sockaddr_storage client_address;
                    socklen_t client_len = sizeof(client_address);
                    SOCKET socket_client = accept(socket_listen,
                            (struct sockaddr*) &client_address,
                            &client_len);
                    if (!ISVALIDSOCKET(socket_client)) {
                        fprintf(stderr, "accept() failed. (%d)\n",
                                GETSOCKETERRNO());
                        return 1;
                    }
                    
                    FD_SET(socket_client, &master);
                    if (socket_client > max_socket)
                        max_socket = socket_client;

                    char address_buffer[100];
                    getnameinfo((struct sockaddr*)&client_address,
                            client_len,
                            address_buffer, sizeof(address_buffer), 0, 0,
                            NI_NUMERICHOST);
                    printf("New connection from %s\n", address_buffer);

                } 
                else{
                    char read[1024];
                    memset(read,0,1024);
                    int bytes_received = recv(i, read, 1024, 0);
                     int w=0;
                            for(int j=0;j<30;j++){
                                if(i==user[j].id)
                                    w = j;
                            }
                        if (bytes_received < 1) {
                            FD_CLR(i, &master);
                            CLOSESOCKET(i);
                            continue;
                        }
                        else if(strcmp(read,"who\n")==0&&user[w].login==1){
                            char all[1024];
                            memset(all,'\0',1024);
                            for(int j=0;j<30;j++){
                                if(user[j].id!=-1&&user[j].login==1)
                                    strcat(all,user[j].name);
                            }
                            send(i,all, 1024, 0);
                        }
                        else if(strcmp(read,"gamelist\n")==0&&user[w].login==1){
                            char all[1024];
                            memset(all,'\0',1024);
                            for(int j=0;j<30;j++){
                                if(user[j].ingame==1){
                                    strcat(all,user[j].name);
                                }
                                    
                            }
                            send(i,"below are people who are playing\n", 1024, 0);
                            usleep(1);
                            send(i,all, 1024, 0);
                            usleep(1);
                        }
                        else if(strcmp(read,"invite\n")==0&&user[w].login==1){
                            send(i, "please choose your oppnent\n", 50, 0);
                            usleep(1);
                            for(int j=0;j<30;j++){
                                if(i==user[j].id)
                                    user[j].state = -1;
                            }
                        }
                        else if(strcmp(read,"help\n")==0&&user[w].login==1){
                            char all[1024];
                            memset(all,'\0',1024);
                            send(i, "\nUsage: \n join {name}: log in with {name}\n gamelist: who is playing now\n who: Check who is online \n winrate {player}: Check {player}'s winrate \n invite: invite someone to play \n Watching: Watching someone's game which is played now\n log out: log out your account\n Chat: Choose to chat with someone \n", 300, 0);
                            usleep(1);

                        }
                        else if(strstr(read,"winrate")&&user[w].login==1){
                            int jud=0;
                            char name[30];
                            memset(name,'\0',30);
                            int k=8;
                            while(read[k]!='\0'){
                                name[k-8]=read[k];
                                k++;
                            }
                            printf("Chenking winrate\n");
                            for(int j=0;j<30;j++){
                                if(!strcmp(user[j].name,name)){
                                    jud=1;
                                    float s;
                                    if(user[j].total==0){
                                        s = 0;
                                    }
                                    else{
                                        s = (float)user[j].win1/(float)user[j].total;
                                    }
                                    s*=100;
                                    char rate[70];
                                    memset(rate,'\0',70);
                                    sprintf(rate, "This person's winning rate is %.2f percent\n", s);
                                    send(i, rate, 70, 0);
                                    usleep(1);

                                    break;
                                }
                            }
                            if(jud==0){
                                send(i, "Person isn't exist", 40, 0);
                                usleep(1);
                            }
                        }
                        else if(strcmp(read,"log out\n")==0&&user[w].login==1){
                            send(i, "Assure to leave?Please type y/n\n", 50, 0);
                            usleep(1);
                            for(int j=0;j<30;j++){
                                if(i==user[j].id)
                                    user[j].state = 2;
                            }
                        }
                        else if(strstr(read,"create")){
                            char name[30];
                            memset(name,'\0',30);
                            int jud=0;
                            int k=7;
                            if(read[k-1]=='\n'){
                                send(i, "Please type your name\n", 50, 0);
                                continue;
                            }
                            while(read[k]!='\0'){
                                name[k-7]=read[k];
                                k++;
                            }
                            printf("Someone want to create new account\n");
                            int go = i;
                            for(int j=0;j<30;j++){
                                if(!strcmp(user[j].name,name)){
                                    send(go, "The name have been used\n", 50, 0);
                                    usleep(1);
                                    printf("Create fail...\n");
                                    jud=1;
                                    break;
                                }
                            }
                            if(jud)
                                continue;
                            for(int j=0;j<30;j++){
                                if(user[j].id==-1)
                                    num=j;
                            }
                            strcpy(user[num].name,name);
                            user[num].id = i;
                            user[num].in=1;
                            user[num].state=9;
                            user[num].total=0;
                            user[num].win=0;
                            user[num].watching=0;
                            user[num].chatid=-1;
                            for(int j=0;j<10;j++){
                                    user[num].watchingguy[j]=-1;
                            }            
                            send(i, "Please set your password\n", 50, 0);
                            usleep(1);
                        }
                        else if(strstr(read,"join")){
                            int jud=0;
                            char name[30];
                            memset(name,'\0',30);
                            int k=5;
                            if(read[k-1]=='\n'){
                                send(i, "Please type your name\n", 50, 0);
                                continue;
                            }
                            while(read[k]!='\0'){
                                name[k-5]=read[k];
                                k++;
                            }
                            printf("Someone is log in\n");
                            int go = i;
                            for(int j=0;j<30;j++){
                                if(!strcmp(user[j].name,name)){
                                send(user[j].id, "Please type password\n", 50, 0);
                                usleep(1);
                                user[j].state=11;
                                jud =1;
                                continue;                           
                                }
                            }
                            if(!jud){
                                send(i, "No this user...\n", 50, 0);                          
                                usleep(1);
                            }
                                
                            continue;
                        }
                        else if(strcmp(read,"Watching\n")==0&&user[w].login==1){
                            send(i, "Who do you want to watch?\n", 50, 0);
                            for(int j=0;j<30;j++){
                                if(i==user[j].id)
                                    user[j].state = 4;
                            }  
                        }
                        else if(strcmp(read,"Chat\n")==0&&user[w].login==1){
                            send(i, "Who do you want to chat?\n", 50, 0);
                            for(int j=0;j<30;j++){
                                if(i==user[j].id)
                                    user[j].state = 6;
                            }  
                        }
                        else{
                            int w=0;
                            for(int j=0;j<30;j++){
                                if(i==user[j].id)
                                    w = j;
                            }
                            if(user[w].state==1&&user[w].login==1){
                                send(i, "\nUsage: \n join {name}\n gamelist\n who\n winrate {player} \n invite \n Watching \n log out \n Chat \n help \n", 100, 0);
                                usleep(1);
                                continue;
                            } 
                            else if(user[w].state==11){
                                char a[1024];
                                memset(a,'\0',1024);
                                strcpy(a,read);
                                if(!strcmp(user[w].password,a)){
                                    user[w].state=1;
                                    user[w].login=1;
                                    send(user[w].id, "log in success Welcome!\n", 50, 0);
                                    usleep(1);
                                }
                                else{
                                    send(user[w].id,"Wrong ,please type again\n", 100, 0);    
                                    usleep(1);
                                }

                            }
                            else if(user[w].state==10){
                                char a[1024];
                                memset(a,'\0',1024);
                                strcpy(a,read);
                                if(!strcmp(user[w].password,a)){
                                    send(user[w].id,"Setting success ,Please log in first\n", 100, 0);    
                                    usleep(1);
                                }
                                else{
                                    send(user[w].id,"Wrong ,please type again\n", 100, 0);    
                                    usleep(1);
                                }
                            }
                            else if(user[w].state==9){
                                memset(user[w].password,'\0',1024);
                                strcpy(user[w].password,read);
                                user[w].state=10;
                                send(user[w].id,"Please check again\n", 100, 0);    
                                usleep(1);
                            }
                            else if(user[w].state==8){
                                char a[70];
                                memset(a,'\0',70);
                                strcpy(a,read);
                                if(strcmp(a,"l\n")==0){
                                    user[user[w].chatid].state=1;
                                    user[w].state=1;
                                    send(user[user[w].chatid].id,"Your friend leaves...You can do another thing\n", 100, 0);    
                                    usleep(1);
                                    send(user[w].id,"You leave \n", 50, 0);  
                                    usleep(1);
                                    user[user[w].chatid].chatid=-1;
                                    user[w].chatid=-1;

                                }
                                else{
                                    send(user[user[w].chatid].id,a, 50, 0);
                                }

                            }
                            else if(user[w].state==7){
                                char a[70];
                                memset(a,'\0',70);
                                strcpy(a,read);
                                if(strcmp(a,"y\n")==0){
                                    user[user[w].chatid].state=8;
                                    user[w].state=8;
                                    send(user[user[w].chatid].id,"The person agree.Now you can chat.If want leave, type l\n", 100, 0);    
                                    usleep(1);
                                    send(user[w].id,"Now you can chat.If want leave, type l\n", 100, 0);
                                    usleep(1);   
                                }
                                else{
                                    user[user[w].chatid].state=1;
                                    user[w].state=1;
                                    user[w].chatid=-1;
                                    user[user[w].chatid].chatid=-1;
                                    user[w].state=1;
                                    send(user[user[w].chatid].id,"The person reject your require...\n", 50, 0);    
                                    usleep(1);
                                    send(user[w].id,"You Reject\n", 50, 0); 
                                    usleep(1);

                                }
                                continue; 
                            }
                            else if(user[w].state==6){
                                char a[70];
                                memset(a,'\0',70);
                                strcpy(a,read);
                                int ext=0;
                                int y=0;
                                for(int j=0;j<30;j++){
                                    if(!strcmp(a,user[j].name)){
                                        ext=1;
                                        y=j;
                                        break;
                                    }
                                }
                                if(ext){
                                    if(user[y].state==1){
                                        char s[100];
                                        memset(s,'\0',100);
                                        //strcat(s,user[w].name);
                                        int u=0;
                                        while(user[w].name[u]!='\n'){
                                            s[u]=user[w].name[u];
                                            u++;
                                        }
                                        strcat(s," want to chat with you, do you agree? Please type y/n.\n");
                                        send(user[y].id, s, 100, 0);                                   
                                        usleep(1);
                                        send(user[w].id, "Waitng...\n", 100, 0);
                                        usleep(1);
                                        user[y].state=7;
                                        user[w].state=0;
                                        user[y].chatid=w;
                                        user[w].chatid=y;
                                    }
                                    else{
                                        send(user[w].id, "The person is busy now\n", 50, 0);
                                        usleep(1);
                                        continue;
                                    }
                                }
                                else{
                                    send(user[w].id, "Can't find the person...\n", 50, 0);
                                    usleep(1);
                                    continue;
                                }
                            }

                            else if(user[w].state==5){
                                char a[70];
                                memset(a,'\0',70);
                                strcpy(a,read);
                                if(!strcmp(a,"l\n")){
                                    send(user[w].id, "Leave success\n", 50, 0);
                                    usleep(1);
                                    user[w].watching=0;
                                    user[w].state=1;
                                    for(int j=0;j<10;j++){
                                        if(user[user[w].watching].watchingguy[j]==user[w].id){
                                            user[user[w].watching].watchingguy[j]=-1;
                                            break;
                                        }
                                    }
                                }
                                
                            }

                            else if(user[w].state==4){
                                char a[70];
                                memset(a,'\0',70);
                                strcpy(a,read);
                                int jud=0;
                                int full=0;
                                for(int j=0;j<30;j++){
                                    if(!strcmp(a,user[j].name)&&user[j].ingame==1){
                                        jud=1;
                                        for(int k=0;k<10;k++){
                                            if( user[j].watchingguy[k]==-1){
                                                send(user[w].id, "Watching Now .If want leave,please type l\n", 50, 0);
                                                usleep(1);
                                                
                                                full=1;
                                                user[w].watching=j;
                                                user[j].watchingguy[k]=w;
                                                user[w].state=5;
                                                char a[100];
                                                memset(a,'\0',100);
                                                strcpy(a,"This person's side is ");
                                                strcat(a,user[j].sysmbol);
                                                send(user[w].id,a, 50, 0);
                                                usleep(1);
                                                break;
                                            }
                                        }
                                        
                                    }
                                }
                                if(!jud){
                                    send(user[w].id, "The person isn't ingame or doesn't exist\n", 50, 0);
                                    usleep(1);
                                    user[w].state=1;
                                    continue;
                                }
                                if(!full){
                                    send(user[w].id, "Watching full\n", 50, 0);
                                    usleep(1);
                                    user[w].state=1;
                                    usleep(1);
                                }
                            }
                            else if(user[w].state==2){
                                char a[70];
                                memset(a,'\0',70);
                                strcpy(a,read);
                                if(!strcmp(a,"y\n")){
                                    send( user[w].id, "Log out success!Goodbye!\n", 50, 0);
                                    usleep(10);
                                    send( user[w].id, "To send data, enter text followed by enter.!\n", 50, 0);
                                    usleep(1);
                                    user[w].state=1;
                                    //user[w].id=-1;
                                    user[w].login=-1;
                                    user[w].in=-1;
                                    //memset(user[w].name,'\0',1024);
                                    memset(user[w].board,'\0',9);
                                    memset(user[w].bit,'\0',100);
                                    memset(user[w].sysmbol,'\0',2);
                                    memset(user[w].more,'\0',4);
                                    user[w].oppnent=0;
                                    user[w].oppfind=0;
                                    user[w].turn=0;
                                    for(int j=0;j<9;j++)
                                        user[w].filled[j]=0;
                                    //user[w].win=0;
                                    user[w].counter=0;
                                    user[w].ingame=0;
                                    //user[w].win1=0;
                                    //user[w].total=0;
                                    
                                }
                                else{
                                    send(i, "Continue playing\n", 50, 0);
                                    usleep(1);
                                    user[w].state=1;
                                    continue;
                                }
                            
                                continue;
                            }
                            else if(user[w].state==-1){
                                send(user[w].id, "please wait...\n", 50, 0);
                                usleep(1);
                                int jud = 0;
                                int opp=0;
                                char get[30];
                                memset(get,'\0',30);
                                char name[30];
                                memset(name,'\0',30);
                                strcpy(name,read);
                                for(int j=0;j<30;j++){
                                    if(i==user[j].id)
                                        user[j].state = -3;
                                }
                                user[w].oppfind=0; 
                                int sel=0;
                                int in=0;
                                for(int j=0;j<30;j++){               
                                    if(!strcmp(user[j].name,name)&&user[j].login==1){
                                        if(user[j].ingame){
                                            send(user[w].id, "He/She is playing.Please find another person\n", 50, 0);
                                            user[w].state= 1;
                                            in=1;
                                            break;
                                        }
                                        if((user[j].id==i)){
                                            sel = 1;
                                            send(user[j].id, "It's you.Please reinvite. \n", 50, 0);
                                            user[j].state = 1;
                                            usleep(1);
                                            break;
                                        }    
                                        user[w].oppnent=j;
                                        user[j].oppnent=w;   
                                        user[w].ingame=1;
                                        user[user[w].oppnent].ingame=1;
                                        user[w].oppfind=1;
                                        user[user[w].oppnent].oppfind=1;                  
                                        //opp = user[j].id;
                                        send(user[j].id, "Do you want to play?\n", 50, 0);
                                        usleep(1);  
                                        user[user[w].oppnent].state=-4;
                                        user[w].state=-4;
                                        memset( user[w].more,'\0',4);
                                        memset( user[user[w].oppnent].more,'\0',4);
                                        break;
                                    }
                                }   
                                
                                    if(in)
                                        continue;
                                    if( user[w].oppfind==0&&sel==0){
                                        send(user[w].id, "We can't find the oppnent\n", 50, 0);
                                        user[w].state = 1;
                                        continue;
                                    }
                            }
                            else if(user[w].state==-4){
                                char a[30];
                                memset(a,'\0',30);
                                strcpy(a,read);
                                if(!strcmp(a,"y\n")){
                                    send(user[user[w].oppnent].id, "Your oppnent aggree your require.\n", 50, 0); 
                                    user[w].total++;
                                    user[user[w].oppnent].total++;
                                    usleep(3);
                                    printf("\nSTARTING GAME\n");
                                    SOCKET fd1 = user[w].id;
                                    SOCKET fd2 = user[user[w].oppnent].id;
                                    for(int k=0;k<9;k++){
                                        user[w].board[k]=(k+1)+'0';
                                        user[user[w].oppnent].board[k]=(k+1)+'0';

                                    }                                                    
                                    int a;
                                    srand(time(NULL));
                                    a=(rand()%2);
                                    if(a==1)
                                        turn=0;
                                    else
                                        turn=1;
                                    if(!turn){
                                        send(fd1, "choose O or X\n", 50, 0);
                                        usleep(1);
                                        send(fd2, "Waiting your oppnent choosing\n", 50, 0);
                                        usleep(1);
                                        user[w].state=-7;
                                        user[user[w].oppnent].state=0;
                                    }
                                    else{
                                        send(fd2, "choose O or X\n", 50, 0);
                                        usleep(1);
                                        send(fd1, "Waiting your oppnent choosing\n", 50, 0);
                                        usleep(1);
                                        user[w].state=0;
                                        user[user[w].oppnent].state=-7;
                                    }
                                }
                                    else{
                                        send(user[user[w].oppnent].id, "Your oppnent reject...\n", 50, 0); 
                                        usleep(1);
                                        send(user[w].id, "You reject\n", 50, 0);
                                        usleep(1);
                                        user[w].ingame=0;
                                        user[user[w].oppnent].ingame=0;                              
                                        user[w].state=1;
                                        user[user[w].oppnent].state=1;
                                        user[w].oppfind=0;    
                                        user[user[w].oppnent].oppfind=0; 
                                        user[user[w].oppnent].oppnent = -1;
                                        user[w].oppnent =-1;
                                        continue;
                                    }
                                }
                                else if(user[w].state==0){
                                    send(user[w].id, "Please wait!\n", 30, 0);
                                    usleep(1);
                                }
                                else if(user[w].state==-7){
                                    char a[70];
                                    memset(a,'0',70);
                                    strcpy(a,read);
                                    if(a[0]=='X'){
                                            strcpy(user[w].sysmbol ,"X\n");
                                            strcpy(user[user[w].oppnent].sysmbol ,"O\n");
                                            send(user[user[w].oppnent].id, "Your are O\n", 20, 0);
                                            usleep(1);
                                        }
                                    else{
                                            strcpy(user[w].sysmbol ,"O\n");
                                            strcpy(user[user[w].oppnent].sysmbol ,"X\n");
                                            send(user[user[w].oppnent].id, "Your are X\n", 20, 0);
                                            usleep(1);
                                        }
                                    for(int j=0;j<9;j++){
                                        user[w].filled[j]=0;
                                        user[user[w].oppnent].filled[j]=0;
                                    }
                                    send(user[w].id, "You first\n", 20, 0);
                                    usleep(1);
                                    user[w].turn=1;
                                    user[w].state=-8;
                                    user[user[w].oppnent].state=-8;
                                    user[user[w].oppnent].turn=0;
                                    user[w].win=0;
                                    user[user[w].oppnent].win=0;
                                    user[w].counter=0;
                                    user[user[w].oppnent].counter=0;
                                    memset(user[w].bit,'\0',100);
                                    memset(user[user[w].oppnent].bit,'\0',100);
                                    int cou3 = -1;
                                    int loop = 0;
                                    while (1)
                                        {
                                            if(cou3==9)
                                                break;
                                            else if(cou3 ==-1)
                                            {
                                                user[w].bit[loop++] = '\n';
                                                user[w].bit[loop++] = '\n';
                                                user[w].bit[loop++] = ' ';
                                                cou3++;
                                                continue;
                                            }
                                            else if(cou3%3!=2){
                                                user[w].bit[loop++] = user[w].board[cou3];
                                                user[w].bit[loop++] = ' ';
                                                user[w].bit[loop++] = '|';
                                                user[w].bit[loop++] = ' ';
                                                cou3++;
                                            }
                                            else if(cou3==8){
                                                user[w].bit[loop++] = user[w].board[cou3];
                                                user[w].bit[loop++] = '\n';
                                                user[w].bit[loop++] = '\0';
                                                break;
                                            }
                                            else if(cou3%3==2){
                                                user[w].bit[loop++] = user[w].board[cou3];
                                                user[w].bit[loop++] = '\n';
                                                user[w].bit[loop++] = '-';
                                                user[w].bit[loop++] = '-';
                                                user[w].bit[loop++] = '-';
                                                user[w].bit[loop++] = '+';
                                                user[w].bit[loop++] = '-';
                                                user[w].bit[loop++] = '-';
                                                user[w].bit[loop++] = '-';
                                                user[w].bit[loop++] = '+';
                                                user[w].bit[loop++] = '-';
                                                user[w].bit[loop++] = '-';
                                                user[w].bit[loop++] = '-';
                                                user[w].bit[loop++] = '\n';
                                                user[w].bit[loop++] = ' ';
                                                cou3++;
                                            }
                                        }
                                        usleep(1);
                                        send(user[w].id, user[w].bit, 100, 0);
                                        usleep(1);
                                        send(user[user[w].oppnent].id,user[w].bit, 100, 0);
                                        usleep(1);

                                }
                                else if(user[w].state==-8){
                                    int over = 0; 
                                    //user[user[w].oppnent].state=0;
                                    if(user[w].turn==1){
                                        usleep(1);
                                        int win=0;
                                        char a[70];
                                        memset(a,'0',70);
                                        strcpy(a,read);
                                        int l=0;
                                        int wrong=1;
                                        for(int j=0;j<9;j++){
                                            if((a[0]-'0')-1==j&&a[1]=='\n'){
                                            wrong =0;
                                            if(user[w].filled[j]==1){
                                                l=1;
                                                break;
                                            }
                                            user[user[w].oppnent].counter++;
                                            user[w].counter++;
                                            user[w].board[j]=user[w].sysmbol[0];
                                            user[user[w].oppnent].board[j]=user[w].sysmbol[0];
                                            user[w].filled[j]=1;
                                            user[user[w].oppnent].filled[j]=1;
                                            break;
                                            }
                                        }
                                        if(l==1){
                                            send(user[w].id,"This fill has been selected.Please choose another field\n", 70, 0);
                                            usleep(1);
                                            continue;
                                        }
                                        if(wrong==1){
                                            send(user[w].id,"Wrong type. Please retype\n", 70, 0);
                                            usleep(1);
                                            continue;
                                        }
                                        //user[w].state = 0;
                                        user[user[w].oppnent].state=-8;
                                        user[w].turn = 0;
                                        user[user[w].oppnent].turn = 1;
                                    }
                                    else{
                                        send(user[w].id,"Please wait your oppnent\n", 40, 0);
                                        usleep(1);
                                        continue;
                                    }

                                    int cou3 = -1;
                                    int loop=0;
                                    while (1)
                                        {
                                            if(cou3==9)
                                                break;
                                            else if(cou3 ==-1)
                                            {
                                                user[w].bit[loop++] = '\n';
                                                user[w].bit[loop++] = '\n';
                                                user[w].bit[loop++] = ' ';
                                                cou3++;
                                                continue;
                                            }
                                            else if(cou3%3!=2){
                                                user[w].bit[loop++] = user[w].board[cou3];
                                                user[w].bit[loop++] = ' ';
                                                user[w].bit[loop++] = '|';
                                                user[w].bit[loop++] = ' ';
                                                cou3++;
                                            }
                                            else if(cou3==8){
                                                user[w].bit[loop++] = user[w].board[cou3];
                                                user[w].bit[loop++] = '\n';
                                                user[w].bit[loop++] = '\0';
                                                break;
                                            }
                                            else if(cou3%3==2){
                                                user[w].bit[loop++] = user[w].board[cou3];
                                                user[w].bit[loop++] = '\n';
                                                user[w].bit[loop++] = '-';
                                                user[w].bit[loop++] = '-';
                                                user[w].bit[loop++] = '-';
                                                user[w].bit[loop++] = '+';
                                                user[w].bit[loop++] = '-';
                                                user[w].bit[loop++] = '-';
                                                user[w].bit[loop++] = '-';
                                                user[w].bit[loop++] = '+';
                                                user[w].bit[loop++] = '-';
                                                user[w].bit[loop++] = '-';
                                                user[w].bit[loop++] = '-';
                                                user[w].bit[loop++] = '\n';
                                                user[w].bit[loop++] = ' ';
                                                cou3++;
                                            }
                                        }
                                        strcpy(user[user[w].oppnent].bit,user[w].bit);
                                        send(user[w].id, user[w].bit, 200, 0);
                                        usleep(1);
                                        send(user[user[w].oppnent].id,user[user[w].oppnent].bit, 200, 0);
                                        usleep(1);
                                        for(int j=0;j<10;j++){
                                            if(user[w].watchingguy[j]!=-1){
                                                if(!user[user[w].watchingguy[j]].watching)
                                                    user[w].watchingguy[j]=-1;
                                                else{
                                                    send(user[user[w].watchingguy[j]].id, user[w].bit, 200, 0);
                                                    usleep(1);
                                                }
                                                
                                            }
                                            if(user[user[w].oppnent].watchingguy[j]!=-1&&user[user[user[w].oppnent].watchingguy[j]].watching!=0){
                                                if(!user[user[user[w].oppnent].watchingguy[j]].watching)
                                                    user[user[w].oppnent].watchingguy[j]=-1;
                                                else{
                                                    send(user[user[user[w].oppnent].watchingguy[j]].id, user[w].bit, 200, 0);
                                                    usleep(1);
                                                }
                                            }
                                        }
                                            if(user[w].board[0]==user[w].board[1]&&user[w].board[1]==user[w].board[2])
                                                user[w].win=1;
                                            else if(user[w].board[3]==user[w].board[4]&&user[w].board[3]==user[w].board[5])
                                                user[w].win=1;
                                            else if(user[w].board[6]==user[w].board[7]&&user[w].board[6]==user[w].board[8])
                                                user[w].win=1;
                                            else if(user[w].board[0]==user[w].board[3]&&user[w].board[0]==user[w].board[6])
                                                user[w].win=1;
                                            else if(user[w].board[1]==user[w].board[4]&&user[w].board[4]==user[w].board[7])
                                                user[w].win=1;
                                            else if(user[w].board[2]==user[w].board[5]&&user[w].board[5]==user[w].board[8])
                                                user[w].win=1;
                                            else if(user[w].board[0]==user[w].board[4]&&user[w].board[4]==user[w].board[8])
                                                user[w].win=1;
                                            else if(user[w].board[2]==user[w].board[4]&&user[w].board[4]==user[w].board[6])
                                                user[w].win=1;
                                            if(user[w].win==1){
                                                user[w].win1++;
                                                send(user[w].id,"You Win!\n", 30, 0);
                                                usleep(1);
                                                send(user[user[w].oppnent].id,"You Lose...\n", 30, 0);
                                                usleep(1);
                                                over=1;
                                            }
                                            if(user[w].counter==9&&user[w].win==0){
                                                send(user[w].id,"Tie game\n", 30, 0);
                                                usleep(1);
                                                send(user[user[w].oppnent].id,"Tie game\n", 30, 0);
                                                usleep(1);
                                                over=1;
                                            }
                                            if(over==1){
                                                send(user[w].id,"Want to play again? type y/n\n", 30, 0);
                                                usleep(1);
                                                send(user[user[w].oppnent].id,"Want to play again? type y/n\n", 30, 0);
                                                usleep(1);
                                                usleep(1);
                                                for(int j=0;j<10;j++){
                                                    if(user[w].watchingguy[j]!=-1&& user[user[w].watchingguy[j]].watching!=0){
                                                        send(user[user[w].watchingguy[j]].id, "Game is over\n", 30, 0);
                                                        usleep(1);
                                                        user[user[w].watchingguy[j]].state=1;
                                                        user[user[w].watchingguy[j]].watching=0;
                                                    }
                                                    if(user[user[w].oppnent].watchingguy[j]!=-1&&user[user[user[w].oppnent].watchingguy[j]].watching!=0){
                                                        send(user[user[user[w].oppnent].watchingguy[j]].id, "Game is over\n", 30, 0);
                                                        usleep(1);
                                                        user[user[user[w].oppnent].watchingguy[j]].state=1;
                                                        user[user[user[w].oppnent].watchingguy[j]].watching=0;
                                                    }
                                            
                                                }
                                                user[w].counter=0;
                                                user[user[w].oppnent].counter=0;
                                                user[w].win=0;
                                                user[user[w].oppnent].win=0;
                                                user[w].state=-9;
                                                user[user[w].oppnent].state=-9;
                                                memset(user[w].bit,'\0',100);
                                                memset(user[user[w].oppnent].bit,'\0',100);
                                                //user[w].oppnent = -2;
                                                //user[user[w].oppnent].oppnent = -2;
                                                for(int k=0;k<9;k++){
                                                user[w].board[k]=(k+1)+'0';
                                                user[user[w].oppnent].board[k]=(k+1)+'0';
                                                for(int k=0;k<9;k++){
                                                    user[w].filled[k]=0;
                                                    user[user[w].oppnent].filled[k]=0;
                                                }
                                                
                                            } 
                                            continue;                
                                        }
                                        usleep(1);
                                        send(user[user[w].oppnent].id,"Now is your turn\n", 20, 0);
                                        usleep(1);
                                continue;
                                }
                                else if(user[w].state==-9){
                                strcpy(user[w].more,read);
                                    if(strcmp(user[user[w].oppnent].more,"y\n")!=0&&strcmp(user[w].more,"y\n")==0){
                                        user[w].state=0;
                                        send(user[w].id,"OK please wait....\n", 30, 0);
                                        usleep(1);
                                        continue;
                                    }
                                    else if(strcmp(user[user[w].oppnent].more,"y\n")==0&&strcmp(user[w].more,"y\n")==0){
                                        user[w].total++;
                                        user[user[w].oppnent].total++;
                                        usleep(1);
                                        int cou3 = -1;
                                        int loop=0;
                                        while (1)
                                        {
                                            if(cou3==9)
                                                break;
                                            else if(cou3 ==-1)
                                            {
                                                user[w].bit[loop++] = '\n';
                                                user[w].bit[loop++] = '\n';
                                                user[w].bit[loop++] = ' ';
                                                cou3++;
                                                continue;
                                            }
                                            else if(cou3%3!=2){
                                                user[w].bit[loop++] = user[w].board[cou3];
                                                user[w].bit[loop++] = ' ';
                                                user[w].bit[loop++] = '|';
                                                user[w].bit[loop++] = ' ';
                                                cou3++;
                                            }
                                            else if(cou3==8){
                                                user[w].bit[loop++] = user[w].board[cou3];
                                                user[w].bit[loop++] = '\n';
                                                user[w].bit[loop++] = '\0';
                                                break;
                                            }
                                            else if(cou3%3==2){
                                                user[w].bit[loop++] = user[w].board[cou3];
                                                user[w].bit[loop++] = '\n';
                                                user[w].bit[loop++] = '-';
                                                user[w].bit[loop++] = '-';
                                                user[w].bit[loop++] = '-';
                                                user[w].bit[loop++] = '+';
                                                user[w].bit[loop++] = '-';
                                                user[w].bit[loop++] = '-';
                                                user[w].bit[loop++] = '-';
                                                user[w].bit[loop++] = '+';
                                                user[w].bit[loop++] = '-';
                                                user[w].bit[loop++] = '-';
                                                user[w].bit[loop++] = '-';
                                                user[w].bit[loop++] = '\n';
                                                user[w].bit[loop++] = ' ';
                                                cou3++;
                                            }
                                        }
                                        //printf("%s",user[w].bit);
                                        strcpy(user[user[w].oppnent].bit,user[w].bit);
                                        send(user[w].id, user[w].bit, 100, 0);
                                        usleep(1);
                                        send(user[user[w].oppnent].id,user[w].bit, 100, 0);
                                        usleep(1);
                                        user[w].state=-8;
                                        user[user[w].oppnent].state=-8;
                                        int a;
                                        srand(time(NULL));
                                        a=(rand()%2);
                                        if(a==1){
                                            user[w].turn=0;
                                            user[user[w].oppnent].turn=1;
                                            send(user[w].id,"Game start:Now is your oppnent's turn\n", 50, 0);
                                            usleep(1);
                                            send(user[user[w].oppnent].id,"Game start:Now is your turn\n",50 , 0);
                                            usleep(1);
                                        }
                                        else{
                                            user[w].turn=1;
                                            user[user[w].oppnent].turn=0;
                                            send(user[w].id,"Game start:Now is your turn\n", 50, 0);
                                            usleep(1);
                                            send(user[user[w].oppnent].id,"Game start:Now is your oppnent's turn\n", 50, 0);
                                            usleep(1);
                                        }
                                        memset(user[w].more,'\0',4);
                                        memset( user[user[w].oppnent].more,'\0',4);

                                        
                                    }
                                    else if(strcmp(user[user[w].oppnent].more,"n\n")==0||strcmp(user[w].more,"n\n")==0){
                                        send(user[w].id,"Game over.Someone leaves...\n", 100, 0);
                                        usleep(1);
                                        send(user[w].id,"You can do another thing\n", 100, 0);
                                        usleep(1);
                                        send(user[user[w].oppnent].id,"Game over.Someone leaves...\n", 100, 0);
                                        usleep(1);
                                        send(user[user[w].oppnent].id,"You can do another thing\n", 100, 0);
                                        usleep(1);
                                        printf("gameover\n");
                                        user[w].state=1;
                                        user[user[w].oppnent].state=1;
                                        user[w].oppfind=0;    
                                        user[user[w].oppnent].oppfind=0; 
                                        user[w].turn=0;
                                        user[user[w].oppnent].turn=0;
                                        user[w].ingame=0;
                                        user[user[w].oppnent].ingame=0;
                                        memset( user[w].more,'\0',4);
                                        memset( user[user[w].oppnent].more,'\0',4);
                                        memset( user[w].sysmbol,'\0',2);
                                        memset( user[user[w].oppnent].sysmbol,'\0',2);
                                    }


                                }
                            
                            }
               
                       
                }
            }
        
        
        } 
    } 



    printf("Closing listening socket...\n");
    CLOSESOCKET(socket_listen);

#if defined(_WIN32)
    WSACleanup();
#endif


    printf("Finished.\n");

    return 0;
}
