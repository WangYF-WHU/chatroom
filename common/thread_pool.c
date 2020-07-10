/*************************************************************************
	> File Name: thread_pool.c
	> Author: suyelu 
	> Mail: suyelu@126.com
	> Created Time: Thu 09 Jul 2020 02:50:28 PM CST
 ************************************************************************/

#include "head.h"

extern int repollfd, bepollfd;
extern struct User *bteam, *rteam;
extern pthread_mutex_t bmutex, rmutex; 

void send_all(struct ChatMsg *msg) { //14行 视频29分钟 
	for(int i = 0;i < MAX; ++i) {
		if(bteam[i].online) send(bteam[i].fd, (void *)msg, sizeof(struct ChatMsg), 0);
		if(rteam[i].online) send(rteam[i].fd, (void *)msg, sizeof(struct ChatMsg), 0);
	}
}


void send_to(char *to, struct ChatMsg *msg,int fd) { //22行 视频35min 
	int flag=0;
	for(int i=0;i<MAX;++i){
		if(rteam[i].online&&(!strcmp(to,rteam[i].name)||!strcmp(to,rteam[i].real_name))){
			send(rteam[i].fd,msg,sizeof(struct ChatMsg),0);
			flag=1;
			break;
		}
		if(bteam[i].online&&(!strcmp(to,bteam[i].name)||!strcmp(to,bteam[i].real_name))){
			send(bteam[i].fd,msg,sizeof(struct ChatMsg),0);
			flag=1;
			break;
		}
	}
	if(!flag){
		memset(msg->msg,0,sizeof(msg->msg));
		sprintf(msg->msg,"用户 %s 不在线，或用户名错误！ ",to);
		msg->type=CHAT_SYS;
		send(fd,msg,sizeof(struct ChatMsg),0);
	}
}

void do_work(struct User *user){ //45行 视频25min 
	//
	//收到一条信息，并打印。 
	struct ChatMsg r_msg;
    struct ChatMsg msg;
    
    bzero(&msg,sizeof(msg));
    bzero(&r_msg,sizeof(r_msg));
    recv(user->fd, (void *)&msg, sizeof(msg), 0);
    if (msg.type & CHAT_WALL) {
    	if(!user->test[4]){
    		printf("<%s> ~ %s \n", user->name, msg.msg);
    		r_msg.type = CHAT_SYS;
    		user->test[4] = 1;
    		user->score += 90;
    		sprintf(r_msg.msg,"%s<%s>Great! You Have Got %d for test003 and test004!\n",user->real_name,user->id,user->score);
			send(user->fd,(void *)&r_msg,sizeof(r_msg),0);
			bzero(r_msg.msg,sizeof(r_msg.msg));
			sprintf(r_msg.msg,"你的好友 "RED"%s"NONE" 上线了，快打个招呼吧！",user->real_name);
			r_msg.type=CHAT_SYS;
			send_all(&r_msg);
			store_score1(4,user->id,user->real_name,user->ip,user->score);
		}
        printf("<%s> $ %s \n", user->name, msg.msg);
        strcpy(msg.name,user->real_name);
        send_all(&msg);
    } else if (msg.type & CHAT_MSG) { //70行 视频30分钟 
    	char to[20]={0};
    	int i=1;
    	for(;i<=21;i++){
    		if(msg.msg[i]==' ') break;
    	}
    	if(msg.msg[i]!=' ' || msg.msg[0]!='@'){
			memset(&r_msg,0,sizeof(r_msg));
			r_msg.type = CHAT_SYS;
			sprintf(r_msg.msg,"私聊格式错误！");
			send(user->fd,(void *)&r_msg,sizeof(r_msg),0);	
		} else{
			msg.type = CHAT_MSG;
			strcpy(msg.name,user->real_name);
			strncpy(to,msg.msg+1,i-1);
			sent_to(to,&msg,user->fd);
		}
		
	} else if (msg.type & CHAT_FIN) { //88行 视频37分钟 
		bzro(msg.msg,sizeof(msg.msg));
		msg.type=CHAT_SYS;
		sprintf(msg.msg,"注意：我们的好朋友 %s 要下线了！\n",user->real_name);
		strcpy(msg.name,user->real_name);
		send_all(&msg);
		if(user->team)
			pthread_mutex_lock(&bmutex);
		else
			pthread_mutex_lock(&rmutex);
		user->score = 0;
        user->online = 0;
        int epollfd = user->team ? bepollfd : repollfd;
        del_event(epollfd, user->fd);
        if(user->team)
			pthread_mutex_unlock(&bmutex);
		else
			pthread_mutex_unlock(&rmutex);
        printf(GREEN"Server Info"NONE" : %s logout!\n", user->name);
        close(user->fd);
    }
}

void task_queue_init(struct task_queue *taskQueue, int sum, int epollfd) {
    taskQueue->sum = sum;
    taskQueue->epollfd = epollfd;
    taskQueue->team = calloc(sum, sizeof(void *));
    taskQueue->head = taskQueue->tail = 0;
    pthread_mutex_init(&taskQueue->mutex, NULL);
    pthread_cond_init(&taskQueue->cond, NULL);
}

void task_queue_push(struct task_queue *taskQueue, struct User *user) {
    pthread_mutex_lock(&taskQueue->mutex);
    taskQueue->team[taskQueue->tail] = user;
    DBG(L_GREEN"Thread Pool"NONE" : Task push %s\n", user->name);
    if (++taskQueue->tail == taskQueue->sum) {
        DBG(L_GREEN"Thread Pool"NONE" : Task Queue End\n");
        taskQueue->tail = 0;
    }
    pthread_cond_signal(&taskQueue->cond);
    pthread_mutex_unlock(&taskQueue->mutex);
}


struct User *task_queue_pop(struct task_queue *taskQueue) {
    pthread_mutex_lock(&taskQueue->mutex);
    while (taskQueue->tail == taskQueue->head) {
        DBG(L_GREEN"Thread Pool"NONE" : Task Queue Empty, Waiting For Task\n");
        pthread_cond_wait(&taskQueue->cond, &taskQueue->mutex);
    }
    struct User *user = taskQueue->team[taskQueue->head];
    DBG(L_GREEN"Thread Pool"NONE" : Task Pop %s\n", user->name);
    if (++taskQueue->head == taskQueue->sum) {
        DBG(L_GREEN"Thread Pool"NONE" : Task Queue End\n");
        taskQueue->head = 0;
    }
    pthread_mutex_unlock(&taskQueue->mutex);
    return user;
}

void *thread_run(void *arg) { //150行 
    pthread_detach(pthread_self());
    struct task_queue *taskQueue = (struct task_queue *)arg;
    while (1) {
        struct User *user = task_queue_pop(taskQueue);
        do_work(user);
    }
}

