#include "client.h"

int data_sock;

int main(int argc, char* argv[]) 
{
	int retcode, s;
	char buffer[MAXBUFSIZE];
	struct addrinfo hints, *res, *rp;
    struct message request_message;

	if (argc != 3) {
		std::cout<<"usage: ./ftclient hostname port\n";
		exit(0);
	}

	char *host = argv[1];
	char *port = argv[2];

	// Get matching addresses
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
	s = getaddrinfo(host, port, &hints, &res);
	if (s != 0) {
		printf("getaddrinfo() error %s", gai_strerror(s));
		exit(1);
	}
	
	// Find an address to connect to & connect
	for (rp = res; rp != NULL; rp = rp->ai_next) {
		data_sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

		if (data_sock < 0)
			continue;

		if(connect(data_sock, res->ai_addr, res->ai_addrlen)==0) {
			break;
		} else {
			perror("connecting stream socket");
			exit(1);
		}
		close(data_sock);
	}
	freeaddrinfo(rp);
    	
    /* Get name and password and send to server */
	ftclient_login();
    while(true)
    {
        //loop until user type quit
		if(ftclient_read_command(buffer,sizeof buffer,&request_message)==false)
		{
			std::cout<<"Invalid command\n";
			continue;
		}
		if(send_message(&request_message)==false)
		{
			break;
		}
		if(read_result(buffer,sizeof buffer)==false)
		{
			break;
		}
    }
	close(data_sock);
	return 1;
}

/**
 * read reply from server
 * only recv() once 
 */
bool read_result(char *buf,int size)
{
	memset(buf,0,sizeof(buf));
	if(recv(data_sock,buf,size,0))
	{
		printf(buf);
		return true;
	}
	return false;
}

/**
 * Parse command in cstruct
 * return true on success, false on fail
 */ 
bool ftclient_read_command(char* buf, int size, struct message *request_message)
{
	memset(request_message->request_header, 0, sizeof(request_message->request_header));
	memset(request_message->request_line, 0, sizeof(request_message->request_line));
	
	printf("client> ");	// prompt for input		
	fflush(stdout); 	

	// wait for user to enter a command
	read_input(buf, size);
	std::string txt(buf);
	//分割请求行和请求头部的空格的下标
	int seperate_idx=txt.find_first_of(' ');
	std::string request_line=txt.substr(0,seperate_idx);
	if(request_line.compare("list")==0)
	{
		strcpy(request_message->request_line,"0");
	}
	else if(request_line.compare("post")==0)
	{
		strcpy(request_message->request_line,"1");
		strcpy(request_message->request_header,buf+seperate_idx+1);
	}
	else
	{
		return false;
	}
	return true;
}


void string_replace(std::string &st1, char c)
{
    int idx;
    while((idx=st1.find_first_of(c))!=-1)
    {
        st1.replace(idx,1,"\r\n");
    }
}

/**
 * 将message结构体构中的内容构造为http格式发送给客户端
 * return true on success, false on fail
 */
bool send_message(struct message *request_message)
{
	std::string request_line(request_message->request_line);
	std::string request_header(request_message->request_header);

	//如果是post请求，则在request_line最后加一个空格
	if(request_line=="1")
	{
		request_header.append(" ");
	}

	string_replace(request_header,' ');

	std::string request(request_line);
	request.append("\r\n");
	request.append(request_header);
	request.append("\r\n");

	if(send(data_sock,request.c_str(),request.size(),0)<0)
	{
		return false;
	}
	return true;
}

/** 
 * Read input from command line
 */
void read_input(char* buffer, int size)
{
	char *nl = NULL;
	memset(buffer, 0, size);

	if ( fgets(buffer, size, stdin) != NULL ) {
		nl = strchr(buffer, '\n');
		if (nl) *nl = '\0'; // truncate, ovewriting newline
	}
}

int send_login_message(char* user,char* password){
	char buffer[MAXBUFSIZE];
	int rc;

	sprintf(buffer, "%s %s\r\n", user,password);
	
	// Send command string to server
	rc = send(data_sock, buffer, (int)strlen(buffer), 0);	
	if (rc < 0) {
		perror("Error sending command to server");
		return -1;
	}
	return 0;
}

/**
 * Get login details from user and
 * send to server for authentication
 */
void ftclient_login()
{
	char user[256];
	memset(user, 0, 256);

	// Get username from user
	printf("Name: ");	
	fflush(stdout); 		
	read_input(user, 256);

	// Get password from user
	fflush(stdout);	
	char *pass = getpass("Password: ");	
	
	send_login_message(user,pass);

	// wait for response
	int retcode = read_reply();
	switch (retcode) {
		case 430:
			printf("Invalid username/password.\n");
			exit(0);
		case 230:
			printf("Successful login.\n");
			break;
		default:
			perror("error reading message from server");
			exit(1);		
			break;
	}
}

/**
 * Receive a response from server
 * Returns -1 on error, return code on success
 */
int read_reply()
{
	int retcode = 0;
	if (recv(data_sock, &retcode, sizeof retcode, 0) < 0) {
		perror("client: error reading message from server\n");
		return -1;
	}	
	return ntohl(retcode);
}
