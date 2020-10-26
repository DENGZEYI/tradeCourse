#include "changeCourse.h"

void modfd( int epollfd, int fd, int ev )
{
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl( epollfd, EPOLL_CTL_MOD, fd, &event );
}

void string_replace(std::string &st1, std::string ori, int ori_length,std::string dst)
{
    int idx;
    while((idx=st1.find_first_of(ori))!=-1)
    {
        st1.replace(idx,ori_length,dst);
    }
}


int change_course::m_epollfd=-1;
int change_course::m_user_count=0;

void change_course::init(int control_socket,const sockaddr_in& addr){
	
	m_control_socket = control_socket;
    m_addr = addr;
	memset(m_user_name,'\0',sizeof(m_user_name));

	//get SO_ERROR option and store it in error
    int error = 0;
    socklen_t len = sizeof( error );
    getsockopt( m_control_socket, SOL_SOCKET, SO_ERROR, &error, &len );
    
	//set SO_REUSEADDR
	int reuse = 1;
    setsockopt( m_control_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof( reuse ) );
    addfd( m_epollfd, m_control_socket, true );

	m_user_count++;

	init();
}

void change_course::init(){
	//初始化应用层缓冲区
	m_read_indx=0;
	memset(m_read_buf,'\0',READ_BUFFER_SIZE);
	m_write_indx=0;
	memset(m_write_buf,'\0',WRITE_BUFFER_SIZE);
}

/**
 * 构造函数
 */
change_course::change_course(){
}

/**
 * 析构函数
 */
change_course::~change_course(){
}


/**
 * Send resposne code on sockfd
 * Returns -1 on error, 0 on success
 */
int change_course::send_response(int sockfd, int rc)
{
	int conv = htonl(rc);
	if (send(sockfd, &conv, sizeof conv, 0) < 0 ) {
		perror("error sending...\n");
		return -1;
	}
	return 0;
}

bool change_course::process_login(){
    printf("receive a connection from socket:%d\n",m_control_socket);
	// Authenticate user
	if (ftserve_login(m_control_socket) == 1) {
		send_response(m_control_socket, 230);
	}
    else {
		//登陆失败
		send_response(m_control_socket, 430);
		//清空应用层缓冲区
		init();
		return false;
	}
	//清空应用层缓冲区
	init();
	return true;
}


/** 
 * Log in connected client
 * return 1 on success, 0 on error
 */
int change_course::ftserve_login(int control_socket)
{	
	char user[MAXSIZE];
	char pass[MAXSIZE];	
	memset(user, '\0', MAXSIZE);
	memset(pass, '\0', MAXSIZE);
	int length = strlen(m_read_buf);
	//当login_message格式完整时
	if(m_read_buf[length-2]=='\r' && m_read_buf[length-1]=='\n'){
		int i;
		int n;
		for(i=0,n=0;i<length;i++){
			if(m_read_buf[i] == ' '){
				break;
			}
			else{
				user[n]=m_read_buf[i];
				n++;
			}		
		}
		i++;
		for(n=0;i<length-2;i++,n++){
			pass[n]=m_read_buf[i];
		}
		return (ftserve_check_user(user, pass));	
	}
	else
	{
		return 0;
	}
}

/**
 * Receive data on sockfd
 * Returns -1 on error, number of bytes received 
 * on success
 */
int change_course::recv_data(int sockfd, char* buf, int bufsize){
	size_t num_bytes;
	memset(buf, 0, bufsize);
	num_bytes = recv(sockfd, buf, bufsize, 0);
	if (num_bytes < 0) {
		return -1;
	}
	return num_bytes;
}

/**
 * Authenticate a user's credentials
 * Return 1 if authenticated, 0 if not
 */
int change_course::ftserve_check_user(char*user, char*pass)
{
	char username[MAXSIZE];
	char password[MAXSIZE];
	char *pch;
	char buf[MAXSIZE];
	char *line = NULL;
	size_t num_read;									
	size_t len = 0;
	FILE* fd;
	int auth = 0;
	
	fd = fopen("./auth_file/.auth", "r");
	if (fd == NULL) {
		perror("file not found");
		exit(1);
	}	

	while ((num_read = getline(&line, &len, fd)) != -1) {
		memset(buf, 0, MAXSIZE);
		strcpy(buf, line);
		
		pch = strtok (buf," ");
		strcpy(username, pch);

		if (pch != NULL) {
			pch = strtok (NULL, " ");
			strcpy(password, pch);
		}

		// remove end of line and whitespace
		trimstr(password, (int)strlen(password));

		if ((strcmp(user,username)==0) && (strcmp(pass,password)==0)) {
			auth = 1;
			break;
		}		
	}
	free(line);	
	fclose(fd);
    //如果登陆成功，则保存登陆者的用户名
    if(auth==1)
        strcpy(m_user_name,user);
	return auth;
}

void change_course::close_conn(){
	removefd(m_epollfd,m_control_socket);
	m_control_socket=-1;
	m_user_count--;
}

bool change_course::read(){
    if( m_read_indx >= READ_BUFFER_SIZE )
    {
        return false;
    }

    int bytes_read = 0;
    while( true )
    {
        bytes_read = recv( m_control_socket, m_read_buf + m_read_indx, READ_BUFFER_SIZE - m_read_indx, 0 );
        if ( bytes_read == -1 )
        {
            if( errno == EAGAIN || errno == EWOULDBLOCK )
            {
                break;
            }
            return false;
        }
        //recv()返回是0时，表示对端关闭
        else if ( bytes_read == 0 )
        {
            return false;
        }

        m_read_indx += bytes_read;
    }
    return true;
}

void change_course::process(){
	if(m_user_name[0]=='\0')
		//如果还没有登陆则执行登陆流程
		if(!process_login())
			//如果登陆失败则关闭连接
			close_conn();
	//已经登陆则不用执行登陆流程
	if(completeness_request_message()){
		change_course::METHOD read_ret=parse_requestline();
		bool write_ret=process_write(read_ret);
		if(!write_ret){
			close_conn();
			return;
		}
		modfd(m_epollfd,m_control_socket,EPOLLOUT);
	}
	else{
		modfd(m_epollfd,m_control_socket,EPOLLIN);
	}
}

bool change_course::completeness_request_message(){
	int length=strlen(m_read_buf);
	if(length>=4 && m_read_buf[length-1]=='\n' && m_read_buf[length-2]=='\r' && m_read_buf[length-3]=='\n' && m_read_buf[length-4]== '\r'){
		return true;
	}
	return false;
}

change_course::METHOD change_course::parse_requestline(){
	std::string text(m_read_buf);
	std::string request_method=text.substr(0,text.find_first_of("\r\n"));
	int method = atoi(request_method.c_str());
	switch(method)
	{
		case 0:
			return LIST;
		case 1:
			return POST;
		default:
			return LIST;
	}
}

void change_course::set_iovec(){
	m_iv[ 0 ].iov_base = m_write_buf;
    m_iv[ 0 ].iov_len = m_write_indx;
    m_iv_count = 1;
}

bool change_course::process_write(METHOD ret){
	switch(ret)
	{
		case LIST:
			if(list_action())
			{
				set_iovec();
				return true;
			}
			else
			{
				return false;
			}
		case POST:
			if(post_action())
			{
				add_response("recved a POST messsage\n");
				set_iovec();
				return true;
			}
			break;
	}
	return false;
}

bool change_course::add_response(const char* format,...){
	if(m_write_indx>WRITE_BUFFER_SIZE)
		return false;
	va_list arg_list;
    va_start( arg_list, format );
    int len = vsnprintf( m_write_buf + m_write_indx, WRITE_BUFFER_SIZE - 1 - m_write_indx, format, arg_list );
    if( len >= ( WRITE_BUFFER_SIZE - 1 - m_write_indx ) )
    {
		va_end(arg_list);
        return false;
    }
    m_write_indx += len;
    va_end( arg_list );
    return true;
}

bool change_course::write(){
	int temp = 0;
    int bytes_have_send = 0;
    int bytes_to_send = m_iv->iov_len;
	//当发送完数据后
    if ( bytes_to_send == 0 )
    {
        modfd( m_epollfd, m_control_socket, EPOLLIN );
		//清空应用层输入缓存和输出缓存
        init();
        return true;
    }
	while(true){
		if(bytes_to_send<=0)
		{
			modfd(m_epollfd,m_control_socket,EPOLLIN);
			init();
			return true;
		}
		temp=writev(m_control_socket,m_iv,m_iv_count);
		if(temp<=-1){
			if( errno == EAGAIN || errno == EWOULDBLOCK)
            {
				//如果写缓冲区没有空间，则等待下一轮EPOLLOUT时间，虽然在此期间，服务器无法立即接收到客户的下一个请求，但这可以保证连接的完整性
                modfd( m_epollfd, m_control_socket, EPOLLOUT );
                return true;
            }
			return false;
		}
		bytes_to_send-=temp;
		bytes_have_send+=temp;
	}
}


/**
 * get data from mariadb and store it in m_write_buf
 * return true one success, false on fail
 */
bool change_course::list_action()
{
	if(connect_db()==false)
	{
		return false;
	}
	else
	{
		std::string data;
		MYSQL_RES *res;
    	MYSQL_ROW row;

    	mysql_query(conn_ptr,"select * from info");
    	res=mysql_store_result(conn_ptr);
    	unsigned rlen = mysql_num_fields(res);
    
    	while ((row = mysql_fetch_row(res))) {
        	for (unsigned i = 0; i < rlen; ++i)
			{
				if(row[i]==NULL)
				{
					data.append("null");
					data.append("\t");
				}
				else
				{				
					data.append(row[i]);
					data.append("\t");
				}
			}
        	data.append("\n");
    	}
		if(data.size()+1<WRITE_BUFFER_SIZE)
		{
			m_write_indx=data.size();
			strcpy(m_write_buf,data.c_str());
		}
		mysql_close(conn_ptr);
		return true;
	}
}

/**
 * connect to mariadb 
 * return true on scuccess, false on fial
 */
bool change_course::connect_db()
{
	conn_ptr = mysql_init( NULL );	/* 连接初始化 */
	if ( !conn_ptr )
	{
		return(false);
	}
	conn_ptr = mysql_real_connect( conn_ptr, "localhost", "root", "123456", "changeCourse", 0, NULL, 0 );	/* 建立实际连接 */
	/* 参数分别为：初始化的连接句柄指针，主机名（或者IP），用户名，密码，数据库名，0，NULL，0）后面三个参数在默认安装mysql>的情况下不用改 */
	if ( conn_ptr )
	{
		return true;
	}
	else
	{
		return false;
	}
}

/**
* update a data to mariadb
 * return true on success, false on fail
 */
bool change_course::post_action()
{
	if(connect_db()==false)
	{
		return false;
	}
	std::string tmp(m_read_buf);
	int start_idx=tmp.find_first_of("\r\n",0)+2;
	int end_idx=tmp.find_last_of("\r\n")-4;
	tmp=tmp.substr(start_idx,end_idx-start_idx+1);
	string_replace(tmp,"\r\n",2,",");

	std::string query("insert into info (src_name,src_id,dst_name,dst_id,telephone_number) values (");
	query.append(tmp);
	query.append(")");

	if(mysql_query(conn_ptr,query.c_str()))
	{
		mysql_close(conn_ptr);
		return false;
	}
	mysql_close(conn_ptr);
	return true;
}
