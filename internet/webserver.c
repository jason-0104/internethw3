/*
 * MIT License
 *
 * Copyright (c) 2018 Lewis Van Winkle
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "chap07.h"


const char *get_content_type(const char* path) {
    const char *last_dot = strrchr(path, '.');
    if (last_dot) {
        if (strcmp(last_dot, ".css") == 0) return "text/css";
        if (strcmp(last_dot, ".csv") == 0) return "text/csv";
        if (strcmp(last_dot, ".gif") == 0) return "image/gif";
        if (strcmp(last_dot, ".htm") == 0) return "text/html";
        if (strcmp(last_dot, ".html") == 0) return "text/html";
        if (strcmp(last_dot, ".ico") == 0) return "image/x-icon";
        if (strcmp(last_dot, ".jpeg") == 0) return "image/jpeg";
        if (strcmp(last_dot, ".jpg") == 0) return "image/jpeg";
        if (strcmp(last_dot, ".js") == 0) return "application/javascript";
        if (strcmp(last_dot, ".json") == 0) return "application/json";
        if (strcmp(last_dot, ".png") == 0) return "image/png";
        if (strcmp(last_dot, ".pdf") == 0) return "application/pdf";
        if (strcmp(last_dot, ".svg") == 0) return "image/svg+xml";
        if (strcmp(last_dot, ".txt") == 0) return "text/plain";
    }

    return "application/octet-stream";
}


SOCKET create_socket(const char* host, const char *port) {
    printf("Configuring local address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *bind_address;
    getaddrinfo(host, port, &hints, &bind_address);

    printf("Creating socket...\n");
    SOCKET socket_listen;
    socket_listen = socket(bind_address->ai_family,
            bind_address->ai_socktype, bind_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_listen)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        exit(1);
    }

    printf("Binding socket to local address...\n");
    if (bind(socket_listen,
                bind_address->ai_addr, bind_address->ai_addrlen)) {
        fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
        exit(1);
    }
    freeaddrinfo(bind_address);

    printf("Listening...\n");
    if (listen(socket_listen, 10) < 0) {
        fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
        exit(1);
    }

    return socket_listen;
}



#define MAX_REQUEST_SIZE 1000000

struct client_info {
    socklen_t address_length;
    struct sockaddr_storage address;
    SOCKET socket;
    char request[MAX_REQUEST_SIZE+1];
    long int received;
    struct client_info *next;
};

static struct client_info *clients = 0;

struct client_info *get_client(SOCKET s) {
    struct client_info *ci = clients;

    while(ci) {
        if (ci->socket == s)
            break;
        ci = ci->next;
    }

    if (ci) return ci;
    struct client_info *n =
        (struct client_info*) calloc(1, sizeof(struct client_info));

    if (!n) {
        fprintf(stderr, "Out of memory.\n");
        exit(1);
    }

    n->address_length = sizeof(n->address);
    n->next = clients;
    clients = n;
    return n;
}


void drop_client(struct client_info *client) {
    CLOSESOCKET(client->socket);

    struct client_info **p = &clients;
    
    while(*p) {
        if (*p == client) {
            *p = client->next;
            free(client);
            return;
        }
        p = &(*p)->next;
    }
    return;
    fprintf(stderr, "drop_client not found.\n");
    exit(1);
}


const char *get_client_address(struct client_info *ci) {
    static char address_buffer[100];
    getnameinfo((struct sockaddr*)&ci->address,
            ci->address_length,
            address_buffer, sizeof(address_buffer), 0, 0,
            NI_NUMERICHOST);
    return address_buffer;
}




fd_set wait_on_clients(SOCKET server) {
    fd_set reads;
    FD_ZERO(&reads);
    FD_SET(server, &reads);
    SOCKET max_socket = server;

    struct client_info *ci = clients;
    
    while(ci) {
        
        FD_SET(ci->socket, &reads);
        if (ci->socket > max_socket)
            max_socket = ci->socket;
        ci = ci->next;
    }

    if (select(max_socket+1, &reads, 0, 0, 0) < 0) {
        fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
        exit(1);
    }
    //int y = select(max_socket+1, &reads, 0, 0, 0);
    //printf("read %d\n",y);

    return reads;
}


void send_400(struct client_info *client) {
    const char *c400 = "HTTP/1.1 400 Bad Request\r\n"
        "Connection: close\r\n"
        "Content-Length: 11\r\n\r\nBad Request";
    send(client->socket, c400, strlen(c400), 0);
    drop_client(client);
}

void send_404(struct client_info *client,int signal) {
    if(signal==1){
        const char *c404 = "HTTP/1.1 404 No Found\r\n"
        "Connection: close\r\n"
        "Content-Length: 9\r\n\r\nNo path";
        send(client->socket, c404, strlen(c404), 0);
        drop_client(client);
    }
    else if(signal==2){
        const char *c404 = "HTTP/1.1 404 No Found\r\n"
        "Connection: close\r\n"
        "Content-Length: 9\r\n\r\ninvaild";
        send(client->socket, c404, strlen(c404), 0);
        drop_client(client);
    }
    else if(signal==3){
        const char *c404 = "HTTP/1.1 404 No Found\r\n"
        "Connection: close\r\n"
        "Content-Length: 9\r\n\r\ntoo large";
        send(client->socket, c404, strlen(c404), 0);
        if(client==NULL)
            printf("dddd\n");
        drop_client(client);
    }
    else if(signal==4){
        const char *c404 = "HTTP/1.1 404 No Found\r\n"
        "Connection: close\r\n"
        "Content-Length: 9\r\n\r\nno file";
        send(client->socket, c404, strlen(c404), 0);
        drop_client(client);
    }

    
}



void serve_resource(struct client_info *client, char *path) {
    printf("serve_resource %s %s\n", get_client_address(client), path);
    if (strcmp(path, "/") == 0) 
        path = "index.html";
    else{
        int i =0;
            if(path[i]=='/'){
                while(path[i]!=NULL){
                   path[i]=path[i+1];
                   i++; 
                }
                
            }
        
        
    }
    //printf("%s\n",path);
    if (strlen(path) > 100) {
        send_400(client);
        return;
    }

    if (strstr(path, "..")) {
        send_404(client,1);
        return;
    }

    char full_path[128];
    sprintf(full_path, "%s", path);
#if defined(_WIN32)      
    char *p = full_path;
    while (*p) {
        if (*p == '/') *p = '\\';
        ++p;
    }
#endif
    FILE *fp = fopen(path, "rb");
    
    if (!fp) {
        printf("404\n");
        send_404(client,4);
        return;
    }

    fseek(fp, 0L, SEEK_END);
    size_t cl = ftell(fp);
    rewind(fp);

    const char *ct = get_content_type(full_path);

#define BSIZE 1024
    char buffer[BSIZE];

    sprintf(buffer, "HTTP/1.1 200 OK\r\n");
    send(client->socket, buffer, strlen(buffer), 0);

    
    sprintf(buffer, "Connection: close\r\n");
    send(client->socket, buffer, strlen(buffer), 0);


    sprintf(buffer, "Content-Length: %u\r\n", cl);
    send(client->socket, buffer, strlen(buffer), 0);


    sprintf(buffer, "Content-Type: %s\r\n", ct);
    send(client->socket, buffer, strlen(buffer), 0);


    sprintf(buffer, "\r\n");
    send(client->socket, buffer, strlen(buffer), 0);

   
    int r = fread(buffer, 1, BSIZE, fp);
    printf("r=%d\n",r);
    while (r) {
        send(client->socket, buffer, r, 0);
        r = fread(buffer, 1, BSIZE, fp);
    }

    fclose(fp);
    drop_client(client);
}

void powerlow(FILE *fp){
    FILE *fp1,*fp2;
    fp1 = fopen (fp, "wb");
    fclose(fp1);
    return;
}
int main() {

#if defined(_WIN32)
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to initialize.\n");
        return 1;
    }
#endif

    SOCKET server = create_socket(0, "8080");

    while(1) {

        fd_set reads;
        reads = wait_on_clients(server);
        int d =FD_ISSET(server, &reads);
        if (FD_ISSET(server, &reads)) {
            struct client_info *client = get_client(-1);
           
            client->socket = accept(server,
                    (struct sockaddr*) &(client->address),
                    &(client->address_length));

            if (!ISVALIDSOCKET(client->socket)) {
                fprintf(stderr, "accept() failed. (%d)\n",
                        GETSOCKETERRNO());
                return 1;
            }

            printf("New connection from %s.\n",
                    get_client_address(client));
        }

        struct client_info *client = clients;
        while(client) {
            
            struct client_info *next = client->next;

            if (FD_ISSET(client->socket, &reads)) {
            
                if (MAX_REQUEST_SIZE == client->received) {
                    send_400(client);
                    client = next;
                    continue;
                }
               
                int r = recv(client->socket,
                        client->request + client->received,
                        MAX_REQUEST_SIZE - client->received, 0);
               
                if (r < 1) {
                    printf("Unexpected disconnect from %s.\n",
                            get_client_address(client));
                    drop_client(client);

                } else {
                    client->received += r;
                    client->request[client->received] = 0;
                    int len = strlen(client->request);
                    
                    
                    char *q = strstr(client->request, "\r\n\r\n");
                    if (q) {
                        //*q = 0;
                        
                        if (strncmp("GET /", client->request, 5)!=0 &&  strncmp("POST /", client->request, 6)!=0) {
                            //int len = strlen(client->request);
                            send_400(client);
                        
                        } 
                        else {
                            if(!strncmp("GET /", client->request, 5)){
                                char *path = client->request + 4;
                                char *end_path = strstr(path, " ");
                                if (!end_path) {
                                    send_400(client);
                                } else {
                                    *end_path = 0;
                                    serve_resource(client, path);
                                }
                            }
                            else if(!strncmp("POST /", client->request, 6)){
                            //printf("go to do something...\n");
                            //printf("re:%s\n",client->request);
                            //fwrite(client->request,100000,1,stderr);
                            FILE *fp ;
                            FILE *fp1 ;
                            int pow = 0;
                            if((strstr(client->request, "text/x-csrc"))!=NULL){
                               send_404(client,2);
                               break;
                                
                            }
                            else if((strstr(client->request, "png"))!=NULL){
                                fp = fopen ("save_client.png", "w");
                                //pow = 1;
                            }
                            else if((strstr(client->request, "jpg"))!=NULL){
                                fp = fopen ("save_client.jpg", "w");
                                pow = 1;
                            }
                            else if((strstr(client->request, "jpeg"))!=NULL){
                                fp = fopen ("save_client.jpeg", "w");
                                //pow = 1;
                            }
                            else if((strstr(client->request, "txt"))!=NULL){
                                fp = fopen ("save_client.txt", "w");
                            }
                            
                            else
                            {
                                send_404(client,2);
                                break;
                            }
                            
                            long long int i=0;
                            int k=0;
                            char p[100];
                            for(int j = 0 ;j<100;j++){
                                p[j]='\0';
                            }
                            //fwrite(client->request,100000,1,stderr);
                            //fclose(fp);
                            int count = 1;
                            while(1){
                                p[k] = client->request[i];
                                if(client->request[i]=='-'&&k==0){
                                    i++;
                                    p[k] = client->request[i];
                                    while(p[k]=='-'){
                                        count++;
                                        if(count>=20){
                                            //printf("%s\n",p);
                                            break;
                                        }
                                        k++;
                                        i++;
                                        p[k] = client->request[i]; 
                                    }
                                    while(1){
                                        i++;
                                        //printf("%c",client->request[i]) ;
                                        //char *p = strstr(client->request,"\r\n\r\n");
                                        /*if(p!=NULL){
                                            printf("ttt\n");
                                            //break;
                                        }*/
                                        int jud = 0;
                                        int con = 0;
                                        int gg = 0;
                                        while(1){
                                            if(client->request[i]=='\r'&&client->request[i+1]=='\n'&&client->request[i+2]=='\r'&&client->request[i+3]=='\n'){
                                                i+=4;
                                                char input[1000000];
                                                for(long int j = 0 ;j<1000000;j++)
                                                    input[j]='\0';
                                                long long int data=0;
                                                while(1){
                                                if(client->request[i]=='-'){
                                                    for(con=0;con<27;con++){
                                                        if(client->request[con+i]!='-'){
                                                            jud = 1;
                                                            break;
                                                        }
                                                    }
                                                }
                                                if(jud==0&&con==27)
                                                    break;
                                                jud=0;
                                                con=0;
                                                input[data] = client->request[i];
                                                i++;
                                                data++;
                                                if(data>65000){
                                                    send_404(client,3);
                                                    gg = 1;
                                                    break;
                                                }
                                            }
                                            if(gg)
                                                break;
                                            fwrite(input,1,data,fp);
                                            fclose(fp);
                                                break;
                                            }

                                            i++;
                                        }
                                        serve_resource(client,"form.html");
                                        break;
                                   }
                                }
                                if(count>=20)
                                    break;
                                if(p[k]=='\n'||p[k]==' '||p[k]==' '){
                                    //printf("%s\n",p);
                                    for(int j=0;j<100;j++)
                                        p[j]='\0';
                                    k=0;
                                    i++;
                                    continue;
                                }
                                
                                k++;
                                i++;
                            }
                            break;
                            }
                        }
                    } //if (q)
                }
            }

            client = next;
        }

    } //while(1)


    printf("\nClosing socket...\n");
    CLOSESOCKET(server);


#if defined(_WIN32)
    WSACleanup();
#endif

    printf("Finished.\n");
    return 0;
}

