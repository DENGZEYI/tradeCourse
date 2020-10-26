#ifndef CHANGECOURSE_H
#define CHANGECOURSE_H

#include "./common/common.h"


#define MAXSIZE 512
#define READ_BUFFER_SIZE 2048
#define WRITE_BUFFER_SIZE 2048 

class change_course{
public:
    //为所有的socket上的事件都被注册到epoll中，所以将epoll设置为静态成员变量
    static int m_epollfd;
    //统计用户总数
    static int m_user_count;
    
    /**
     * 自定义协议
     */
    enum METHOD {LIST=0,POST};
private:
    int m_control_socket;
    //存储被服务对象的用户名
    char m_user_name[MAXSIZE];
    //存储服务对象的addr
    sockaddr_in m_addr;
    char m_read_buf[READ_BUFFER_SIZE];
    int m_read_indx;
    char m_write_buf[WRITE_BUFFER_SIZE];
    int m_write_indx;
    //我们将采用writev来执行写操作，所以定义下面两个成员，其中m_iv_count表示被写到内存块的数量
    struct iovec m_iv[1];
    int m_iv_count;
    //MYSQL结构体指针
    MYSQL * conn_ptr;

public:
    change_course();
    ~change_course();
    void init(int control_socket,const sockaddr_in& addr);
    void init();
    /**
     * Send resposne code on sockfd
     * Returns -1 on error, 0 on success
     */
    int send_response(int socket_fd,int rc);
    int ftserve_login(int control_socket);
    int recv_data(int sockfd, char* buf, int bufsize);
    int ftserve_check_user(char*user, char*pass);
    void close_conn();
    //循环读取客户端上的数据，直到无数据可读性或者对方关闭
    bool read();
    /**
     * 服务器处理客户端登陆流程
     * 返回值：登陆成功返回true，否则返回false
     */
    bool process_login();
    void process();
    /**判断m_read_buf中是否为一个完整的请求
     * 返回值：完整返回true，否则返回flase
     * */
    bool completeness_request_message();
    /**
     * 获取请求报文的请求行
     */
    METHOD parse_requestline();
    /**
     * 根据读取到的报文内容来写应答报文
     */
    bool process_write(METHOD ret);
    /**
     * 往应用层写缓冲区中写入待发送的数据
     */
    bool add_response( const char* format, ... );
    /**
     * 将应用层缓冲区的数据写到内核缓冲区
     */
    bool write();
    /**
     * iovec指向应用层输出缓冲区
     */
    void set_iovec();

    /**
     * get data from mariadb and store it in m_write_buf
     * return true on success, false on fail
     */
    bool list_action();

    /**
     * connect to mariadb 
     * return true on scuccess, false on fial
     */
    bool connect_db();

    /**
     * update a data to mariadb
     * return true on success, false on fail
     */
    bool post_action();
};


#endif