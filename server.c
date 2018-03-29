/*
	多客户端通信
	文件传输

	初步实现单服务完成文件及文字传输

	此版本中客户端与服务端不兼容（尚待解决）
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#define SERVPORT 3333
#define BACKLOG 10
#define MAXDATASIZE 1024

//USER Datasets
struct user
{
	char IP[16];
	int NUM;
	int FD;
}User[10000];

//Pac Datasets
typedef struct{
    char type;
    char data[512];
}m_package;

char msg[30];
struct sockaddr_in client_sockaddr;

/*客户端连接处理函数 init_socket、init_lis*/
int init_socket()
{
	int sockfd;
	if((sockfd = socket(AF_INET,SOCK_STREAM,0))==-1)
	{
		perror("socket");
		exit(1);
	}
	printf("socket success!,sockfd=%d\n",sockfd);
	struct sockaddr_in server_sockaddr;
	memset(&server_sockaddr, 0, sizeof(server_sockaddr));
	server_sockaddr.sin_family=AF_INET;    /* Internet地址族 */
	server_sockaddr.sin_port=htons(SERVPORT);
	server_sockaddr.sin_addr.s_addr=INADDR_ANY;    /* IP地址 */
	bzero(&(server_sockaddr.sin_zero),8);                //memset(&(server_sockaddr.sin_zero),0,8);  置字节字符串s的前n个字节为零
	if(bind(sockfd,(struct sockaddr *)&server_sockaddr,sizeof(struct sockaddr))==-1)
	{        //bind  return 0表示成功， -1表示不成功
		perror("bind");
		exit(1);
	}
	printf("bind success!\n");
	if(listen(sockfd,BACKLOG)==-1)
	{
		perror("listen");
		exit(1);
	}
	return sockfd;
}

int init_lis(int sockfd)
{
	
	int client_fd;
	printf("listening....\n");
	int sin_size = sizeof(client_sockaddr);
	if((client_fd=accept(sockfd,(struct sockaddr *)&client_sockaddr,&sin_size))==-1)
	{
		perror("accept");
		exit(1);
	}

	int flag = arr_op(client_fd);

	if(flag >= 0)
	{
		User[flag].FD = client_fd;
		sprintf(User[flag].IP, "%s", inet_ntoa(client_sockaddr.sin_addr));
		User[flag].NUM = flag + 2017;
		sprintf(msg, "Your user id is : %d\n", User[flag].NUM);
	}
	
	send(client_fd, msg, strlen(msg), 0);
	printf("receive a client：%s\n", User[flag].IP);
	printf("client_fd is :%d\n", User[flag].FD);
	return client_fd;
}

/*判断当前申请链接的客户端是否已存在编号*/
int arr_op(int fd)
{
	for (int i = 0; i < 1024; i++)
	{
		if(strcmp(User[i].IP, inet_ntoa(client_sockaddr.sin_addr)) == 0)
		{
			User[i].FD = fd;
			sprintf(msg, "Your user id is : %d\n", User[i].NUM);
			break;
		}
		else
		{
			if(User[i].FD == 0 && User[i].NUM == 0)
			{
				return i;
			}
		}
	}
	return -1;
}

void *init_acc(int client_socket)
{
	char buf[MAXDATASIZE], buff[MAXDATASIZE], me[MAXDATASIZE], ta[MAXDATASIZE], file_name[20];
	int recvbytes, sendbytes;
	int left, right, diff;
	
	int size = 0, fd, count = 0;
	m_package pac;
	long total = 0, cur = 0;
	
	while(1)
	{
		left = 0;
		right = 0;
		diff = 0;
		memset(buf, 0, 1024);
		memset(buff, 0, 1024);
		memset(me, 0, 1024);
		memset(ta, 0, 1024);
		memset(file_name, 0, 20);

		if((recvbytes=recv(client_socket,buf,MAXDATASIZE,0))==-1)
		{
			perror("recv");
			exit(1);
		}
		printf("received a connection :%s\n",buf);

		/*获取发送数据中两次空格所在数据位*/
		for(int i = 0;i < 1024;i++)
		{
			if(left == 0 && buf[i] == 32)
			{
				left = i;
				continue;
			}
			else if(left != 0 && buf[i] == 32)
			{
				right = i;
				continue;
			}
			else if(buf[i] == 0)
			{
				buf[i] = '\n';
				break;
			}
		}

		/*Get the input user number*/
		diff  = right - left;
		int client_num, client_socket_d;
		if(diff == 5)
			client_num = ((int)buf[left+1]-'0')*1000 + ((int)buf[left+2]-'0')*100 + ((int)buf[left+3]-'0')*10 + (int)buf[right-1]-'0';
		else if(diff == 6)
			client_num = ((int)buf[left+1]-'0')*10000 + ((int)buf[left+2]-'0')*1000 + ((int)buf[left+3]-'0')*100 + ((int)buf[left+4]-'0')*10 + (int)buf[right-1]-'0';
			
		/*判断发送目标端用户是否存在*/
		for (int i = 0; i < 1024; i++)
		{
			if(User[i].NUM != 0)
			{
				if(client_num == User[i].NUM)
				{
					break;
				}
			}
			else if(User[i].NUM == 0)
			{
				goto _clo;
			}
		}
		
		/*获取编号对应的句柄号*/
		for (int i = 0; i < 1024; ++i)
		{
			if(User[i].NUM == client_num)
			{
				client_socket_d = User[i].FD;
				break;
			}
		}
		
		/*获取有效文本*//*buff为有效文本；send 2016 xxxxx；buff = "xxxxx"*/
		for(int i = 0;i < 1024;i++)
		{
			if(buf[right+i] != 0 && buf[5] != 0)
			{
				buff[i] = buf[right+i];
			}
			else if(buf[right+i] == 0)
			{
				break;
			}
		}

		/*判断有效文本是否为文件名,格式:xx/xx/xx*/
		for(int i = 0;i < strlen(buff);i++)
		{
			if(buff[i] == '/')
			{
				strcpy(file_name, strrchr(buff, '/') + 1);
			}
			if(i == strlen(buff) - 1)
			{
				file_name[0] = 'a';
			}
		}
		

		/**/
		if(file_name[0] == 'a')//判断为非文本文件
		{
			/*字符串处理并发送纯文字文本*/
			if(buff[0] != 0)
			{
			
				int ID;
				for(int i = 0;i < 1024;i++)
				{
					if(User[i].FD == client_socket)
					{
						ID = User[i].NUM;
						break;
					}
				}
				sprintf(me, "me : %s", buff);
				sprintf(ta, "%d : %s", ID, buff);
				if((sendbytes=send(client_socket_d,ta,strlen(ta),0))==-1)
				{
					perror("send");
					exit(1);
				}
				if((sendbytes=send(client_socket,me,strlen(me),0))==-1)
				{
					perror("send");

					exit(1);
				}
			}
			else
			{
				if(strncmp(buf, "end", 3) == 0)
				{

					break;
				}
			}
		}
		else if(file_name[0] != 'a')//判断为文本文件
		{
			
			while(1)
			{
				memset(&pac, 0, sizeof(pac));
				size = read(client_socket, &pac, sizeof(pac));
				printf("%c\n", pac.type);
				write(client_socket, "over!", 10);
				if(size > 0)
				{
					//pac.type = 1;
					//printf("%s\n", pac.type);

					/**使用数组的格式来接受第一个包：文件名*/
					/*
						printf("%s\n", pac.data);
						fd = open(pac.data, O_CREAT|O_WRONLY, 0777);
						if(-1 == fd)
						{
							printf("open file error!\n");
							continue;
						}
						count = total = cur = 0;
						printf("%s\n", pac.data);
					*/

					//type为1接受文件名
				    if (pac.type == 1)
					{
						printf("%s\n", pac.data);
						fd = open(pac.data, O_CREAT|O_WRONLY, 0777);
						if(-1 == fd)
						{
							printf("open file error!\n");
							continue;
						}
						count = total = cur = 0;
						printf("%s\n", pac.data);
				    }
				    //type为2接受文件内容
				    else if (pac.type == 2)
					{
						cur += write(fd, pac.data, strlen(pac.data));
						if(count++ % 5000 == 0)
						{
							printf("recv from client <> : %.01lf\\%\n", cur * 100.0 / total);
							count = 0;
						}
				    }
				    //type为3接受文件结束标志
				    else if (pac.type == 3)
					{
						printf("recv from client <> : 100.0\\%\n");
						printf("recv success\n");
						close(fd);
				    }
				    //type为4接受文件大小
				    else if(pac.type == 4)//文件长度
					{
						total = strtol(pac.data, NULL, 10);
						printf("%ld\n", total);
				    }
				}
				else
				{
				    printf("client disconnected\n");
				    close(client_socket);
				    break;
				}
			}
			
		}
		
	}
	
	close(client_socket);

	_clo:
		close(client_socket);
}

int main(int argc, char *argv[])
{
	char xx[100] = {"please input layout : send target data\n"};
	int listen_socket = init_socket();
	while(1)
	{
		int client_socket = init_lis(listen_socket);
		send(client_socket,xx,strlen(xx),0);
		pthread_t TH;
		pthread_create(&TH, NULL, init_acc, (void *)client_socket);
	}
	close(listen_socket);
}
