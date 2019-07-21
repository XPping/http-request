#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <ctype.h>
#include <pthread.h>


#define MAX_LISTEN_NUMBER 5
#define BUFFER_SIZE 1024

// init request_head
#define INIT_REQUEST_HEAD(head) \
    head.method[0] = '\0';      \
    head.url[0] = '\0';         \
    head.port[0] = '\0';             \
    head.request[0] = '\0';
// init user_info
#define INIT_USER_INFO(info) \
    info.username[0] = '\0';      \
    info.passwd[0] = '\0';         
/*
 * Http head information
 */
typedef struct request_head{
    char method[5];     // GET, POST, or '\0'
    char url[16];       // ip, or '\0'
    char port[6];       // port, or '\0'
    char request[1024]; // the route after url:port/(..., this)
} request_head;

/*
 * user information 
 */
typedef struct user_info{
    char username[20];
    char passwd[20];
} user_info;

void client_request(int); // response to client request
int request_http_head(int , request_head *);

/*
 * parse the request of http head info
 * if bad request return -1, else return 1
 */
int request_http_head(int client_sock, request_head *head){
    char buf[BUFFER_SIZE];          // save a line, if characters of line >= BUFFER_SIZE then bad request 
    int buf_length = 0;             // line length
    int http_head_line_number = 0;  // http head lines, if it>=100 then bad request 
    ssize_t size = 0;
    char c;
    while(1){
        size = recv(client_sock, &c, 1, 0);
        if(size <= 0) return -1;
        if(c == '\r'){
            size = recv(client_sock, &c, 1, 0);
            if(size <= 0) break;
            if(c == '\n'){ // http lien end;
                if(buf_length == 0) break; // http head end;
                // http lien end, and parse
                http_head_line_number++;
                if(http_head_line_number >= 100) return -1; //bad request
                if(buf_length >= 3 && buf[0]=='G' && buf[1]=='E' && buf[2]=='T'){ // GET
                    strcpy(head->method, "GET");
                } else if (buf_length >= 4 && buf[0]=='P' && buf[1]=='O' && buf[2]=='S' && buf[3]=='T'){ // POST
                    strcpy(head->method, "POST");
                } else if (buf_length >=4 && buf[0]=='H' && buf[1]== 'o' && buf[2]=='s' && buf[3]=='t' && buf[4]==':'){ // Host:
                    int start = 5;
                    int end = 5;
                    // remove space
                    while(end<buf_length && isspace((int)buf[end])){
                        end++;
                    }
                    // url between [start, end)
                    start = end;
                    while(end<buf_length && buf[end] != ':'){
                        end++;
                    }
                    // copy url into head.url;
                    int i;
                    for(i=start; i<end; i++){
                        head->url[i-start] = buf[i];
                    }
                    head->url[i-start] = '\0';
                    // copy port into head.port
                    start = i+1;
                    for(; i<buf_length && (i-start)<5; i++)
                        head->port[i-start] = buf[i];
                    head->port[i-start] = '\0';
                } else if (buf_length >= 7 && buf[0]=='A' && buf[1]=='c' && buf[2]=='c' && buf[3]=='e' && buf[4]=='p' && buf[5]=='t' && buf[6]==':'){ // Accept:
                    // copy request into head.request
                    int i;
                    for(i=7; i<buf_length; i++){
                        head->request[i-7] = buf[i];
                    }
                    i++;
                    head->request[i] = '\0';
                }
                buf_length = 0; // the next line 
            } else { // go on read line
                buf[buf_length] = '\r';
                buf_length++;
                if(buf_length >= BUFFER_SIZE) return -1; // bad request;
                buf[buf_length] = c;
                if(buf_length >= BUFFER_SIZE) return -1; // bad request;
            }
        } else { // go on read line
            buf[buf_length] = c;
            buf_length++;
            if(buf_length >= BUFFER_SIZE) return -1; // bad request;
        }
    }
    return 1;
}

/*
 * html page one
 */
void html_page_one(int client)
{
    char buf[BUFFER_SIZE];
    // http head 
    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "Content-type:text/html\r\n\r\n");
    send(client, buf, strlen(buf), 0);
    // http body 
    strcpy(buf, "<html>\n<body>\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "<form action=\"\", method=\"POST\">");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "<table align=\"center\">");
    send(client, buf, strlen(buf), 0);
    
    strcpy(buf, "<tr><td height=\"30\" align=\"right\">UserName:");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "<td align=\"left\"><input type=\"text\" name=\"username\"/></td></tr>");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "<tr><td height=\"30\" align=\"right\">Password:");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "<td align=\"left\"><input type=\"text\" name=\"passwd\"/></td></tr>");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "<tr><td align=\"right\"><input type=\"submit\", name=\"submit\", value=\"submit\">");
    send(client, buf, strlen(buf), 0);
    
    strcpy(buf, "</table>\n</form>\n</body>\n</html>\n");
    send(client, buf, strlen(buf), 0);
}

/*
 * html page two
 */
void html_page_two(int client, user_info *info)
{
    char buf[BUFFER_SIZE];
    // http head 
    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "Content-type:text/html\r\n\r\n");
    send(client, buf, strlen(buf), 0);
    // http body 
    strcpy(buf, "<html>\n<body>\n");
    send(client, buf, strlen(buf), 0);
    
    strcpy(buf, "<table align=\"center\">");
    send(client, buf, strlen(buf), 0);
    
    strcpy(buf, "<tr><td height=\"30\" align=\"right\">UserName:");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "<td align=\"left\">");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, info->username);
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "</td></td></tr>");
    send(client, buf, strlen(buf), 0);
    
    strcpy(buf, "<tr><td height=\"30\" align=\"right\">Password:");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "<td align=\"left\">");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, info->passwd);
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "</td></td></tr>");
    send(client, buf, strlen(buf), 0);
    
    strcpy(buf, "</table>\n</body>\n</html>\n");
    send(client, buf, strlen(buf), 0);
}

/*
 * process the request of client by save the main info into struct head and 
 * body. 
 */
void client_request(int client_sock)
{
    
    struct request_head head;
    INIT_REQUEST_HEAD(head);
    struct user_info info;
    INIT_USER_INFO(info);
    
    int ret = request_http_head(client_sock, &head);
    if(ret == -1){
        printf("bad request\n");
    } else {
        printf("head.method: %s\n", head.method);
        printf("head.url: %s\n", head.url);
        printf("head.port: %s\n", head.port);
        printf("head.request: %s\n", head.request);
    }
            
    if(strcasecmp(head.method, "GET") == 0){
        html_page_one(client_sock);
    } else if(strcasecmp(head.method, "POST") == 0){
        char body[BUFFER_SIZE];
        int body_length;
        ret = recv(client_sock, body, BUFFER_SIZE, MSG_DONTWAIT);
        if(ret > 0){
            printf("body: %s\n", body);
            printf("body length: %d\n", ret);
            int i=0, j=0, start=0, cnt=0;
            while(i<ret && cnt<2){
                if(body[i] == '='){
                    if(cnt == 0){
                        j = i+1;
                        while(j<ret && body[j]!= '&'){
                            info.username[j-i-1] = body[j];
                            j++;
                        }
                        info.username[j-i-1] = '\0';
                        cnt++;
                        i = j;
                    } else if(cnt == 1){
                        j = i+1;
                        while(j<ret && body[j]!='&'){
                            info.passwd[j-i-1] = body[j];
                            j++;
                        }
                        info.passwd[j-i-1] = '\0';
                        cnt++;
                        i = j;
                    }
                }
                i++;
            }
            printf("%s, %s\n", info.username, info.passwd);
            html_page_two(client_sock, &info);
        }
    }

    printf("close client\n");
    close(client_sock);
    pthread_exit(0);
}

/*
 * build sock and listen local port, waiting for connect 
 */
int main(int argc, char *argv[])
{
    const char* ip = "127.0.0.1";
    const int port = 8080;

    // sock addr struct
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &addr.sin_addr);
    addr.sin_port = htons(port);

    // build sock, and build, and listen
    int server_sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(server_sock >= 0);
    int ret = bind(server_sock, (struct sockaddr*)&addr, sizeof(addr));
    assert(ret != -1);
    ret = listen(server_sock, MAX_LISTEN_NUMBER);
    assert(ret != -1);

    // client
    struct sockaddr_in client;
    socklen_t client_length = sizeof(client);
    // while accept
    pthread_t client_thread;
    while(server_sock != -1){
        int client_sock = accept(server_sock, (struct sockaddr*)&client, &client_length);
        if(client_sock < 0){
            if(server_sock != -1) // for signal set
                printf("error connect\n");
        } else {
            pid_t pid;
            pid = fork();
            if(pid == 0){
                // i'm child
                close(server_sock);
                client_request(client_sock);
                exit(0);
            } else {
                close(client_sock);
            }

            //if(pthread_create(&client_thread, NULL, (void *)client_request, client_sock) != 0){
            //    printf("Error thread for client: %d\n", client_sock);
            //}
        }
    }
    close(server_sock);
    return 0;
}
