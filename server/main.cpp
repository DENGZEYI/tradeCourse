#include "./common/common.h"
#include "changeCourse.h"
#include "threadpool.h"

#define MAX_EVENT_NUNBER 500
#define MAX_USER_NUMBER 1024

/**
 * creat epoll and add listenfd to it
 * return epollfd
 */
int creat_epoll(int listenfd){
    int epollfd = epoll_create( 5 );
    assert( epollfd != -1 );
    addfd( epollfd, listenfd, false );
    change_course::m_epollfd = epollfd;
    return epollfd;
}

/**
 * send error message to connect_fd
 */
void show_error( int connfd, const char* info )
{
    printf( "%s", info );
    send( connfd, info, strlen( info ), 0 );
    close( connfd );
}


int main(){
    //为每个用户分配一个change_course对象
    change_course* user=new change_course[MAX_USER_NUMBER];

    //创建线程池
    threadpool< change_course >* pool = NULL;
    try
    {
        pool = new threadpool< change_course >;
    }
    catch( ... )
    {
        return 1;
    }
    
    //忽略SIGPIPE信号，因为程序接收到SIGPIPE信号默认执行的是结束进程。
    //IGN is short for ignore
    addsig( SIGPIPE, SIG_IGN );
    int port = 7777;

    //创建监听socket
    int socket_listen = socket_create(port);
    assert(socket_listen > 0);
    std::cout<<"creat a listen socket"<<std::endl;

    epoll_event events[ MAX_EVENT_NUNBER ];
    int epollfd=creat_epoll(socket_listen);
    
    
    while(1){
        int number=epoll_wait(epollfd,events,MAX_EVENT_NUNBER,-1);
        if ( ( number < 0 ) && ( errno != EINTR ) )
        {
            printf( "epoll failure\n" );
            break;
        }
        for(int i=0;i<number;i++){
            int sockfd=events[i].data.fd;
            if(sockfd==socket_listen){
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof( client_address );
                int connfd = accept( socket_listen, ( struct sockaddr* )&client_address, &client_addrlength );
                if ( connfd < 0 )
                {
                    printf( "errno is: %d\n", errno );
                    continue;
                }
                if( change_course::m_user_count >= MAX_USER_NUMBER )
                {
                    show_error( connfd, "Internal server busy" );
                    continue;
                }
                //初始化客户连接
                user[connfd].init(connfd,client_address);
            }
            else if( events[i].events & ( EPOLLRDHUP | EPOLLHUP | EPOLLERR ) )
            {
                //如果有异常，直接关闭客户连接
                user[sockfd].close_conn();
            }
            else if(events[i].events&EPOLLIN){
                if(user[sockfd].read()){
                    pool->append(user+sockfd);
                }
                else{
                    user[sockfd].close_conn();
                }
            }
            else if(events[i].events&EPOLLOUT){
                if(!user[sockfd].write())
                    user[sockfd].close_conn();
            }
            else
            { }
        }
    }
    close(epollfd);
    close(socket_listen);
    delete[] user;
    delete pool;

    return 0;
}
