#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define MAXDATASIZE 10000 // max number of bytes we can get at once

void _own(char*);

int main(int argc, char *argv[])
{
	FILE* fp;
	int sockfd, numbytes,PORT,i,l,cnt=0;
	char buf[MAXDATASIZE],buf1[99999],_files[9999],buffer[16384];
	struct hostent *he;
	struct sockaddr_in their_addr; // connector’s address information
	if (argc != 3) {
		fprintf(stderr,"usage: client hostname\n");
		exit(1);
	}
	if ((he=gethostbyname(argv[1])) == NULL) { // get the host info
		perror("gethostbyname");
		exit(1);
	}
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
	PORT=atoi(argv[2]);
	their_addr.sin_family = AF_INET; // host byte order
	their_addr.sin_port = htons(PORT); // short, network byte order
	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(their_addr.sin_zero),'\0', 8); // zero the rest of the struct
	
	if (connect(sockfd, (struct sockaddr *)&their_addr,sizeof(struct sockaddr)) == -1) {
		perror("connect");
		exit(1);
	}

	numbytes=recv(sockfd,buf,MAXDATASIZE-1,0);
	buf[numbytes] = '\0';
	printf(">--------- %s ----------\n",buf);
	
	while(1)
	{
		bzero(buffer,16384);
		printf("> ");
		gets(buf);		//inputs the command string
		if(buf[0]==0)
			continue;
		if(buf[0]=='o' && buf[1]=='w' && buf[2]=='n')		//pass the string to function _own for the client manager
		{
			_own(buf);
			continue;
		}
		send(sockfd,buf,strlen(buf),0);			//sends the command to server
		l=strlen(buf);
		
		for(i=0,cnt=0;;i++)			//get the command
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
		
		for(i=cnt;i<l;i++)			//get the next arguement
		{
			if(buf[i]!=32)
			{
				buf[i-cnt]=buf[i];
			}
			else
				break;
		}
		buf[i-cnt]='\0';
		if(strcmp(buf1,"quit")==0)		//to quit
		{
			close(sockfd);
			exit(0);
		}
		if(strcmp(buf1,"get")==0)		//to get the file from server
		{
			strcpy(_files,"recv_");
			strcat(_files,buf);
			FILE* fp2 = fopen(_files,"a");
			ssize_t bytes;
			int byte,len=0;
			bzero(buffer,sizeof(buffer));
			recv(sockfd,buffer,sizeof(buffer),0);
			send(sockfd,"OK",2,0);
			byte=atoi(buffer);
			while(1)
			{
				bytes=recv(sockfd, buffer, sizeof(buffer), 0);
		 		if (bytes < 0) perror("recv");  // network error?
				if (bytes <= 0) break;   // sender closed connection, must be end of file
		  
				if (fwrite(buffer, 1, bytes, fp2) != (size_t) bytes)
				{
					perror("fwrite");
		      			break;
				}
				len+=bytes;
				if(len==byte)
				 	break;
			}
			fclose(fp2);
			printf("Received %i bytes from network, writing them to file...\n", byte);
			printf("File: %s received successfully..\n",_files);
			continue;
		}
		else if(strcmp(buf1,"upload")==0)		//to upload file to server
			{
				long size;
				int c=0;
				char len[100];
				FILE *fp1=fopen(buf,"r");
				fseek(fp1,0,SEEK_END);
				size=ftell(fp1);
				sprintf(len,"%d",size);
				send(sockfd,len,strlen(len),0);
				recv(sockfd,len,sizeof(len),0);
				fseek(fp1,0,SEEK_SET);
				while(1)
				{
					ssize_t bytes=fread(buffer, 1, sizeof(buffer), fp1);
					if (bytes <= 0) break;
					
					if (send(sockfd, buffer, bytes, 0) != (size_t)bytes)
					{
						perror("send");
						break;
					}
				}
				printf("Read %i bytes from file, sending them to network...\n",size);
				printf("File: %s successfully sent to Server\n",buf);
			}
			else if(strcmp(buf1,"ls")==0 || strcmp(buf1,"lsl")==0)		//to get the directory/file lists from server
				{
					int byte,len=0;
					bzero(buffer,sizeof(buffer));
					recv(sockfd,buffer,sizeof(buffer),0);
					send(sockfd,"OK",2,0);
					byte=atoi(buffer);
					while(1)
					{
						ssize_t bytes=recv(sockfd, buffer, sizeof(buffer), 0);
						if(bytes<=0)
							break;
						printf("%s\n",buffer);
						len+=bytes;
						if(len==byte)
							break;
					}
				}
				else
				{
					if ((numbytes=recv(sockfd, buffer, 99998, 0)) == -1) {
					perror("recv");
					exit(1);
					}
					buffer[numbytes] = '\0';
					printf("%s\n",buffer);
				}
	}
	close(sockfd);
	return 0;
}

void _own(char *buf)		//function to execute the command on the client side
{
	char buf1[5000],*cwd,_files[99999];
	int i,j,k,l,cnt,t,status;
	pid_t pid;
	
	l=strlen(buf);
	for(i=4,j=0,cnt=4;;i++,j++)
	{
		if(buf[i]!=32)
		{
			buf1[j]=buf[i];
			cnt++;
		}
		else
			break;
	}
	l=strlen(buf);
	buf1[j]='\0';
	cnt++;
	for(i=cnt;i<l;i++)
	{
		if(buf[i]!=32)
		{
			buf[i-cnt]=buf[i];
		}
		else
			break;
	}
	buf[i-cnt]='\0';
	printf("%s %s\n",buf1,buf);
	if(strcmp(buf1,"ls")==0 || strcmp(buf1,"lsl")==0 || strcmp(buf1,"rrmdir")==0)
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
					t=execlp("rm","rm","-r",buf,NULL);
				else
				{
					if(strcmp(buf1,"ls")==0)
					{
						if(buf[0]==NULL)
							t=execlp("ls","ls",NULL);
						else
							t=execlp("ls","ls",buf,NULL);
					}
					else
					{
						if(buf[0]==NULL)
							t=execlp("ls","ls","-l",NULL);
						else
							t=execlp("ls","ls","-l",buf,NULL);
					}
				}					
			}
	}
	else if(strcmp(buf1,"cd")==0)
		{
			if(chdir(buf)==-1)
				printf("Directory not found: %s\n",buf);
			else
			{
				cwd=getcwd(_files,9999);
				printf("Directory changed to : &s\n",cwd);
			}
		}
	else if(strcmp(buf1,"pwd")==0)
		{
			cwd=getcwd(_files,9999);
			printf("Current Directory: %s\n",cwd);
		}
	else if(strcmp(buf1,"md")==0)
		{
			if(mkdir(buf,S_IRWXU |S_IRGRP | S_IXGRP |S_IROTH | S_IXOTH)==-1)
				printf("Directory already exists: %s\n",buf);
			else
				printf("Created directory: %s\n",buf);
		}
	else if(strcmp(buf1,"rmdir")==0)
		{
			if(rmdir(buf)==-1)
				printf("Directory does not exists or directory contains data..\nStill to remove directory type: rrmdir");
			else
			{
				printf("Deleted directory: %s\n",buf);
			}
		}
		else
		{
			printf("Command not found..\n");
		}
}
