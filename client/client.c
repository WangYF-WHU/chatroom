/*************************************************************************
	> File Name: client.c
	> Author: suyelu 
	> Mail: suyelu@126.com
	> Created Time: Wed 08 Jul 2020 04:32:12 PM CST
 ************************************************************************/

#include "head.h"

int server_port = 0;
char server_ip[20] = {0};
char *conf = "./football.conf";
int sockfd = -1;

/*update*/
WINDOW *message_win, *message_sub,  *info_win, *info_sub, *input_win, *input_sub;
int msgnum = 0;

void logout(int signum) {
    struct ChatMsg msg;
    msg.type = CHAT_FIN;
    send(sockfd, (void *)&msg, sizeof(msg), 0);
    close(sockfd);
    DBG(RED"Bye"NONE);
    exit(0);
}

/*void *print(void* arg) {
    struct ChatMsg msg;
    fd_set rfds;
    bzero(&msg, sizeof(msg));
    while (1) {
        FD_ZERO(&rfds);
        FD_SET(sockfd, &rfds);
        int retval = select(sockfd + 1, &rfds, NULL, NULL, NULL);
        if (retval < 0) {
            perror("select() in print");
            exit(1);
        } else if (retval) {
            if (recv(sockfd, &msg, sizeof(msg), 0) >0) {
                printf("%s : %s\n", msg.name, msg.msg);
            }
        } else {
            DBG(RED"Error"NONE"The Server is out of service!\n");
            exit(1);
        }
    }
}*/


int main(int argc, char **argv) {
	
	/*update*/
	setlocale(LC_ALL,"");
	
    int opt;
    struct LogRequest request;
    struct LogResponse response;
    
    bzero(&request, sizeof(request));
    bzero(&response, sizeof(response));

    while ((opt = getopt(argc, argv, "h:p:t:m:n:")) != -1) {
        switch (opt) {
            case 't':
                request.team = atoi(optarg);
                break;
            case 'h':
                strcpy(server_ip, optarg);
                break;
            case 'p':
                server_port = atoi(optarg);
                break;
            case 'm':
                strcpy(request.msg, optarg);
                break;
            case 'n':
                strcpy(request.name, optarg);
                break;
            default:
                fprintf(stderr, "Usage : %s [-hptmn]!\n", argv[0]);
                exit(1);
        }
    }
    

    if (!server_port) server_port = atoi(get_conf_value(conf, "SERVERPORT"));
    if (!request.team) request.team = atoi(get_conf_value(conf, "TEAM"));
    if (!strlen(server_ip)) strcpy(server_ip, get_conf_value(conf, "SERVERIP"));
    if (!strlen(request.name)) strcpy(request.name, get_conf_value(conf, "NAME"));
    if (!strlen(request.msg)) strcpy(request.msg, get_conf_value(conf, "LOGMSG"));


    DBG("<"GREEN"Conf Show"NONE"> : server_ip = %s, port = %d, team = %s, name = %s\n%s\n",\
        server_ip, server_port, request.team ? "BLUE": "RED", request.name, request.msg);

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(server_port);
    server.sin_addr.s_addr = inet_addr(server_ip);

    socklen_t len = sizeof(server);
	
	/*update*/
	init_ui();
	
	//if ((sockfd = socket_create_udp(5555)) < 0) {
    if ((sockfd = socket_udp()) < 0) {
        perror("socket_udp()");
        exit(1);
    }
    
    sendto(sockfd, (void *)&request, sizeof(request), 0, (struct sockaddr *)&server, len);
    
    /*update*/
    struct ChatMsg tmp;
    
    fd_set set;
    FD_ZERO(&set);
    FD_SET(sockfd, &set);
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    int retval = select(sockfd + 1, &set, NULL, NULL, &tv);
    if (retval < 0) {
        perror("select()");
        exit(1);
    } else if (retval) {
        int ret = recvfrom(sockfd, (void *)&response, sizeof(response), 0, (struct sockaddr *)&server, &len);
        if (ret != sizeof(response) || response.type) {
            DBG(RED"Error"NONE"The Game Server refused you login\n\tThis maybe helpful %s\n", response.msg);
            exit(1);
        }
    } else {
        DBG(RED"Error"NONE"The Game Server is out of service!\n");
        exit(1);
    }

    DBG(GREEN"Server"NONE" : %s\n", response.msg);

	/*update*/
	strcpy(tmp.msg, response.msg);
    show_message(message_sub, &tmp, 1);

    connect(sockfd, (struct sockaddr *)&server, len);

    pthread_t recv_t;
    if (pthread_create(&recv_t, NULL, do_recv, NULL) < 0) {
        perror("pthread_create()");
        exit(1);
    }

    signal(SIGINT, logout); 
    
    /*update*/
    noecho();
    cbreak();
    while(1){
    	send_chat();
    }
    
    /*
    while (1) {
        struct ChatMsg msg;
        msg.type = CHAT_WALL;
        printf(RED"Please Input:  \n"NONE);
        scanf("%[^\n]S", msg.msg);
        getchar();
        if (strlen(msg.msg)) {
            if (msg.msg[0] == '@') msg.type = CHAT_MSG;
            if (msg.msg[0] == '#') msg.type = CHAT_FUNC;
            send(sockfd, (void *)&msg, sizeof(msg), 0);
        }
    }
	*/
    //pthread_join(recv_t, NULL);

    return 0;
}
