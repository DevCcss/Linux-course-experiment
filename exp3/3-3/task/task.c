#include<stdio.h>
#include<pthread.h>
#include<semaphore.h>
#include<sys/ipc.h>
#include<string.h>
#define MSG_SIZE 512
//全局变量
int msqid;
size_t msgsize;
struct msg{//消息格式
    long mtype;
    char mtext[MSG_SIZE];
};
struct msg mymsg;
sem_t mutex,empty,full;

void *sender(void *arg){
    char buff[MSG_SIZE];
    int flag = 0;
    
    while(1){
        sem_wait(&empty);
        sem_wait(&mutex);
        if((msgrcv(msqid,&mymsg,MSG_SIZE,1,IPC_NOWAIT))>0)
            puts(mymsg.mtext);
        if(flag){
            printf("\033[31m\033[1mSender exit!\033[0m\n");
            return;
        }
        printf("\033[33m\033[1mSender input:\033[0m");
        gets(buff);
        if(!strcmp(buff,"exit"))
            flag=1;
        mymsg.mtype=1;
        strcpy(mymsg.mtext,buff);
        if(msgsnd(msqid,&mymsg,MSG_SIZE,0)<0)
            printf("\033[31m\033[1mSender:msgsend error!\033[0m\n");
        sem_post(&mutex);
        sem_post(&full);
    }
}
void *receiver(void *arg){
    char buff[MSG_SIZE];
    int flag = 0;
    while(1){
        sem_wait(&full);
        sem_wait(&mutex);
        if((msgrcv(msqid,&mymsg,MSG_SIZE,1,0))<0)
            printf("\033[31m\033[1mReceiver:msgsend error!\033[0m\n");
        else{
            if(strcmp(mymsg.mtext,"exit"))
                snprintf(buff,512,"\033[36m\033[01mReceiver reply \"%s\"\033[0m:over",mymsg.mtext);
            else{
                flag = 1;
                snprintf(buff,512,"\033[36m\033[01mReceiver reply \"%s\"\033[0m:over\n\033[31m\033[01mReceiver exit!\033[0m",mymsg.mtext);
            }
            strcpy(mymsg.mtext,buff);
            mymsg.mtype=1;
            msgsnd(msqid,&mymsg,MSG_SIZE,0);
        }
        sem_post(&mutex);
        sem_post(&empty);
        if(flag)
            return;
    }
}
int main()
{
    pthread_t Sender,Receiver;
//创建信号量
    sem_init(&mutex,0,1);
    sem_init(&full,0,0);
    sem_init(&empty,0,1);
//创建消息队列
    //这里很坑，除了表示创建的flag，读写权限也要加上
    if((msqid=msgget(IPC_PRIVATE,IPC_CREAT|0660))<0)
        printf("megget error\n");
//创建线程
    pthread_create(&Sender,NULL,sender,NULL);//创建sender线程，执行sender函数,不传入参数，不对线程属性进行定制
    pthread_create(&Receiver,NULL,receiver,NULL);
    sleep(1);
//阻塞调用线程
    pthread_join(Sender,NULL);
    pthread_join(Receiver,NULL);
    msgctl(msqid,IPC_RMID,NULL);
    sem_destroy(&mutex);
    sem_destroy(&empty);
    sem_destroy(&full);
    return 0;
}