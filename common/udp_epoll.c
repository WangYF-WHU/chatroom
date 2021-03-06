/*************************************************************************
	> File Name: udp_epoll.c
	> Author: suyelu 
	> Mail: suyelu@126.com
	> Created Time: Thu 09 Jul 2020 04:40:38 PM CST
 ************************************************************************/

#include "head.h"
extern int port;
extern struct User *rteam;
extern struct User *bteam;
extern int repollfd, bepollfd;
extern pthread_mutex_t rmutex;
extern pthread_mutex_t bmutex;

void add_event_ptr(int epollfd, int fd, int events, struct User *user) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.ptr = (void *)user;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
}

void del_event(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
}

int find_sub(struct User *team){
	for(int i=0;i<MAX;++i){
		if(!team[i].online) return i;
	}
	return -1;
}

int udp_connect(struct sockaddr_in *client) { //32行 视频49分钟
    int sockfd;
    if ((sockfd = socket_create_udp(port)) < 0) {
        perror("socket_create_udp");
        return -1;
    }
    if (connect(sockfd, (struct sockaddr *)client, sizeof(struct sockaddr)) < 0) {
        return -1;
    }
    return sockfd;
}

int check_online(struct LogRequest *request) {
    for (int i = 0; i < MAX; i++) {
        if (rteam[i].online && !strcmp(request->name, rteam[i].name))
            return 1;
        if (bteam[i].online && !strcmp(request->name, bteam[i].name))
            return 1;
    }
    return 0;
}


int udp_accept(int fd, struct User *user) { //52行 视频48分钟 
    int new_fd, ret;
    struct sockaddr_in client;
    struct LogRequest request;
    struct LogResponse response;
    bzero(&request, sizeof(request));
    bzero(&response, sizeof(response));
    socklen_t len = sizeof(client);
    
    ret = recvfrom(fd, (void *)&request, sizeof(request), 0, (struct sockaddr *)&client, &len);
    
    if (ret != sizeof(request)) {
        response.type = 1;
        strcpy(response.msg, "Login failed with Data errors!");
        sendto(fd, (void *)&response, sizeof(response), 0, (struct sockaddr *)&client, len);
        return -1;
    }
    
	if (check_online(&request)) {
        response.type = 1;
        strcpy(response.msg, "You are Already Login!");
        sendto(fd, (void *)&response, sizeof(response), 0, (struct sockaddr *)&client, len);
        return -1;
    }

    if (request.team) {
        DBG(GREEN"Info"NONE" : "BLUE"%s login on %s:%d  <%s>\n", request.name, inet_ntoa(client.sin_addr), ntohs(client.sin_port), request.msg);
    } else {
        DBG(GREEN"Info"NONE" : "RED"%s login on %s:%d   <%s>\n", request.name, inet_ntoa(client.sin_addr), ntohs(client.sin_port), request.msg);
    }
	//strcpy(user->ip,inet_ntoa(client.sin_addr));
    strcpy(user->name, request.name);
    user->team = request.team;
    //user->score=0;
    //memset(user->test,0,sizeof(user->test));
    new_fd = udp_connect(&client);
    user->fd = new_fd;
    
    char cmd[512]={0};
    /*中间是跟老师本地的测评记录有关的 跳过*/
    
    response.type = 0;
    bzero(cmd,sizeof(cmd));
	sprintf(cmd,"%s Login Success. Enjoy yourself!\n",user->name); 
	strcpy(response.msg, cmd);
    send(new_fd,(void *)&response,sizeof(response),0);
	//sendto(fd, (void *)&response, sizeof(response), 0, (struct sockaddr *)&client, len);
    return new_fd;
}

/*int find_sub(struct User *team) {
    for (int i = 0; i < MAX; i++) {
        if (!team[i].online) return i;
    }
    return -1;
}*/

void add_to_sub_reactor(struct User *user){ //120行 视频52分半 
    struct User *team = (user->team ? bteam : rteam);
    DBG(YELLOW"Main Thread : "NONE"Add to sub_reactor\n");
    if(user->team)
    	pthread_mutex_lock(&bmutex);
    else
    	pthread_mutex_lock(&rmutex);
    	
    int sub = find_sub(team);
    if (sub < 0) {
        fprintf(stderr, "Full Team\n");
        return;
    }
    team[sub] = *user;
    team[sub].online = 1;
    team[sub].flag = 10;
    if(user->team)
    	pthread_mutex_unlock(&bmutex);
    else
    	pthread_mutex_unlock(&rmutex);
    
    DBG(L_RED"sub = %d, name = %s\n", sub, team[sub].name);
    if (user->team) 
        add_event_ptr(bepollfd, team[sub].fd, EPOLLIN | EPOLLET, &team[sub]);
    else 
        add_event_ptr(repollfd, team[sub].fd, EPOLLIN | EPOLLET, &team[sub]);
}
    
 /*   int sub;
    if (user->team == 0) {
    	if ((sub = find_sub(rteam)) < 0) {
    		perror("find_team()");
    		exit(1);
		}
		rteam[sub] = *user;
		add_event_ptr(repollfd, user->fd, (EPOLLIN | EPOLLET), user);
	} else {
		if ((sub = find_sub(bteam)) < 0) {
    		perror("find_team()");
    		exit(1);
		}
		bteam[sub] = *user;
		add_event_ptr(bepollfd, user->fd, (EPOLLIN | EPOLLET), user);
	}
}
*/
