#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#define BACKLOG 4 			// how many pending connections queue will hold
#define MAX_LENGTH 16384

inline void _FilesDirs(char *dir, int new_fd);

int main(int argc, char **argv)
{
	FILE* fp;
	pid_t pid;
	char _files[MAX_LENGTH],*cwd,buf[MAX_LENGTH],buf1[MAX_LENGTH],buf2[16384];
	int sin_size,l,status,fd,i,cnt,t;
	int sockfd, new_fd,numbytes,MYPORT; // listen on sock_fd, new connection on new_fd
	MYPORT=atoi(argv[1]);

	struct sockaddr_in my_addr; // my address information
	struct sockaddr_in their_addr; // connector’s address information

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
	
	my_addr.sin_family = AF_INET; // host byte order
	my_addr.sin_port = htons(MYPORT); // short, network byte order
	my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
	memset(&(my_addr.sin_zero), '\0', 8); // zero the rest of the struct
	
	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr))== -1) {
		perror("bind");
		exit(1);
	}
	
	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}
	
	while(1) { // main accept() loop
		sin_size = sizeof(struct sockaddr_in);
		if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr,
		&sin_size)) == -1) {
		perror("accept");
		continue;
	}
	
	printf("Server: got connection from %s\n",inet_ntoa(their_addr.sin_addr));
	
	cwd=getcwd(_files,sizeof(_files));
	strcpy(buf2,"Current Directory: ");
	strcat(buf2,cwd);
	send(new_fd,buf2,strlen(buf2),0);
	
	while(1)
	{
		bzero(buf,sizeof(buf));		//empty the arrays
		bzero(buf1,sizeof(buf1));
		bzero(_files,sizeof(_files));
		
		recv(new_fd,buf,MAX_LENGTH-1,0);	//receive the command string
		l=strlen(buf);
		
		for(i=0,cnt=0;;i++)			//loop to seperate the command
		{
			if(buf[i]!=32)
			{
				buf1[i]=buf[i];
				cnt++;
			}
			else
				break;
		}
		l=strlen(buf);
		buf1[i]='\0';
		cnt++;
		
		for(i=cnt;i<l;i++)			//loop to get the next argument
		{
			if(buf[i]!=32)
			{
				buf[i-cnt]=buf[i];
			}
			else
				break;
		}
		buf[i-cnt]='\0';
		
		if(strcmp(buf1,"quit")==0)		//for quit
		{
			close(new_fd);
			exit(0);
		}
		
		if(strcmp(buf1,"ls")==0 || strcmp(buf1,"lsl")==0 || strcmp(buf1,"rrmdir")==0)	//for ls ,lsl, rrmdir commands
		{
			pid=fork();
			if(pid<0)
				printf("Forking Error!!!\n");
			else if(pid>0)
				{
					waitpid(pid,&status,0);
				}
				else
				{
					if(strcmp(buf1,"rrmdir")==0)
					{
						printf("Received rrmdir command...\n");
						fd=open("temp.txt",O_RDWR|O_CREAT|O_TRUNC,0755);
						dup2(fd,1);
						t=execlp("rm","rm","-r",buf,NULL);
					}
					else
					{
						if(strcmp(buf1,"ls")==0)
						{
							printf("Received ls command...\n");
							fd=open("temp.txt",O_RDWR|O_CREAT|O_TRUNC,0755);
							dup2(fd,1);
							if(buf[0]==NULL)
								t=execlp("ls","ls",NULL);
							else
								t=execlp("ls","ls",buf,NULL);
						}
						else
						{
							printf("Received lsl command...\n");
							fd=open("temp.txt",O_RDWR|O_CREAT|O_TRUNC,0755);
							dup2(fd,1);
							if(buf[0]==NULL)
								t=execlp("ls","ls","-l",NULL);
							else
								t=execlp("ls","ls","-l",buf,NULL);
						}
					}					
				}
			if(t==-1)
				continue;
			close(fd);
			int size;
			char len[100];
			fp=fopen("temp.txt","r");
			fseek(fp,0,SEEK_END);
			size=ftell(fp);
			sprintf(len,"%d",size);
			send(new_fd,len,strlen(len),0);
			recv(new_fd,len,sizeof(len),0);
			fseek(fp,0,SEEK_SET);
			while(1)
			{
				ssize_t bytes=fread(_files,1,MAX_LENGTH,fp);
				if(bytes<=0)
					break;
				send(new_fd,_files,bytes,0);
			}
			fclose(fp);
		}
		else if(strcmp(buf1,"cd")==0)		//to change directory
			{
				printf("Received cd command...\n");
				if(chdir(buf)==-1)
					strcpy(buf2,"Directory not found: ");
				else
				{
					cwd=getcwd(_files,9999);
					strcpy(buf2,"Directory changed to : ");
				}
				strcat(buf2,cwd);
				send(new_fd,buf2,strlen(buf2),0);
			}
		else if(strcmp(buf1,"pwd")==0)		//to get the current directory
			{
				printf("Received pwd command...\n");
				cwd=getcwd(_files,9999);
				strcpy(buf2,"Current Directory: ");
				strcat(buf2,cwd);
				send(new_fd,buf2,strlen(buf2),0);
			}
		else if(strcmp(buf1,"md")==0)		//to make directory
			{
				printf("Received md command...\n");
				if(mkdir(buf,S_IRWXU |S_IRGRP | S_IXGRP |S_IROTH | S_IXOTH)==-1)
					strcpy(buf2,"Directory already exists: ");
				else
					strcpy(buf2,"Created directory: ");
				strcat(buf2,buf);
				send(new_fd,buf2,strlen(buf2),0);
			}
		else if(strcmp(buf1,"rmdir")==0)		//to remove empty directory
			{
				printf("Received rmdir command...\n");
				if(rmdir(buf)==-1)
					strcpy(buf2,"Directory does not exists or directory contains data..\nStill to remove directory type: rrmdir");
				else
				{
					strcpy(buf2,"Deleted directory: ");
					strcat(buf2,buf);
				}
				send(new_fd,buf2,strlen(buf2),0);
			}
		else if(strcmp(buf1,"get")==0)			//to transfer file to client
			{
				printf("Received get command...\n");
				long size;
				int c=0;
				char len[100];
				fp=fopen(buf,"r");
				fseek(fp,0,SEEK_END);
				size=ftell(fp);
				sprintf(len,"%d",size);
				send(new_fd,len,strlen(len),0);
				recv(new_fd,len,sizeof(len),0);
				fseek(fp,0,SEEK_SET);
				while(1)
				{
					ssize_t bytes=fread(buf2, 1, sizeof(buf2), fp);
					if (bytes <=0) break;
					if (send(new_fd, buf2, bytes, 0) != (size_t)bytes)
					{
						perror("send");
						break;
					}
				}
				fclose(fp);
				printf("Read %i bytes from file, sending them to network...\n",size);
				printf("File: %s Successfully sent\n",buf);
			}
			else if(strcmp(buf1,"upload")==0)		//to load file from client
				{
					printf("Received upload command...\n");
					strcpy(_files,"serv_recv_");
					strcat(_files,buf);
					fp = fopen(_files,"w");
					int byte,len=0;
					ssize_t bytes;
					bzero(buf2,sizeof(buf2));
					recv(new_fd,buf2,sizeof(buf2),0);
					send(new_fd,"OK",2,0);
					byte=atoi(buf2);
					while(1)
					{
					  bytes=recv(new_fd, buf2, sizeof(buf2), 0);
					  if (bytes < 0) perror("recv");  // network error?
					  if (bytes <= 0) break;   // sender closed connection, must be end of file

					  if (fwrite(buf2, 1, bytes, fp) != (size_t) bytes)
					  {
					     perror("fwrite");
					     break;
					  }
					  len+=bytes;printf("l==%d\n",len);
					  if(len==byte)
					  	break;
					}
					fclose(fp);
					printf("Received %i bytes from network, writing them to file...\n", byte);
					printf("File: %s received successfully from client..\n",_files);
					continue;
				}
				else
				{
					send(new_fd,"Invalid Command!!!\n",sizeof(buf2),0);
					printf("Command not found..\n");
				}
			}
		close(new_fd); // parent doesn’t need this
	}
	return 0;
}
