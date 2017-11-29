# include <stdio.h>
# include <sys/types.h> /* sockets */
# include <sys/socket.h> /* sockets */
# include <netinet/in.h> /* internet sockets */
# include <unistd.h> /* read , write , close */
# include <netdb.h> /* ge th os tb ya dd r */
# include <stdlib.h> /* exit */
# include <string.h> /* strlen */
#include <fcntl.h>


#include <sys/wait.h> /* sockets */
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h> /* toupper */
#include <signal.h> /* signal */
#include <time.h>
#include "intList.c"
#define	maxsize 512
void perror_exit (char *message);

//./contentserver -d /home/teomandi/Desktop/syspro/E1 -p 9013

char *dirorfilename;
int sock, newsock;
intList *list;

void sighandler(){
	int i;
	close(newsock);
	close(sock);
	free(dirorfilename);
	destroyList(list);
	printf("ContentServer terminated by Signal\n");
}


int isDir(char* target){
   struct stat statbuf;
   stat(target, &statbuf);
   return S_ISDIR(statbuf.st_mode);
}

char *maketheList(char* file){
	char *lista, *temp;
	if(isDir(file)){
		struct dirent *ent;
		DIR *dir;

		lista = malloc(sizeof(char)*(strlen(file)+2));
		strcpy(lista,file);
		strcat(lista,"\n");
		if((dir = opendir(file))!= NULL) {
			while((ent= readdir(dir)) != NULL) {
				if(strcmp(ent->d_name,".")==0 || strcmp(ent->d_name,"..")==0)	continue;
				/*fix our data*/
				temp = malloc(sizeof(char)*(strlen(file)+strlen(ent->d_name)+2));
				strcpy(temp,file);
				strcat(temp,"/");
				strcat(temp, ent->d_name);
				if(isDir(temp)){
					if(strlen(lista)==0){
						lista = realloc(lista,sizeof(char)*(strlen(maketheList(temp))));					
						strcpy(lista,maketheList(temp));
					}
					else{
						lista = realloc(lista,sizeof(char)*(strlen(lista)+strlen(maketheList(temp))));					
						strcat(lista,maketheList(temp));
					}
				}
				else{
					if(strlen(lista)==0){
						lista = realloc(lista,sizeof(char)*(strlen(temp)+2));					
						strcpy(lista,temp);
					}
					else{
						lista = realloc(lista,sizeof(char)*(strlen(lista)+strlen(temp)+2));					
						strcat(lista,temp);
					}
					strcat(lista,"\n");
				}
			}
			closedir(dir);
		}
		else
			perror_exit("opendir");
	}
	else{
		if(strlen(lista)==0){
			lista = realloc(lista,sizeof(char)*(strlen(file)+1));		
			strcpy(lista,file);
		}
		else{
			lista = realloc(lista,sizeof(char)*(strlen(lista)+strlen(file)+1));		
			strcat(lista,file);
		}
		strcat(lista,"\n");
	}
	free(temp);
	return lista;
}

void Sentfile(int newsock, char* filename, int casse){
	char buf[maxsize];
	if(casse==1)
		strcpy(buf,"file|");
	else{
		strcpy(buf,filename);
		strcat(buf,"|");
	}

	if(write(newsock, buf, sizeof(buf))< 0)	
		perror_exit ("write");
	if(read(newsock, buf, sizeof(buf)) < 0)
		perror_exit("read");
	if(strcmp(buf,"ok")!=0)
		perror_exit("Worker not ok");
	/*read and sent all the data*/
	int fp, red;
	printf("we open FILE name with path: %s\n",filename);
	fp = open(filename, O_RDONLY);  
	if(fp<0) perror_exit("File didnt open.");
	while((red=read(fp, buf, sizeof(buf)))>0){
		if(write(newsock, buf, sizeof(buf))< 0)	
			perror_exit ("write");
		if(read(newsock, buf, sizeof(buf)) < 0)
			perror_exit("read");
		if(strcmp(buf,"ok")!=0){
			perror_exit("Worker didn't respond");
		}
	}
	strcpy(buf,"end");
	if(write(newsock, buf, sizeof(buf))< 0)	
		perror_exit ("write");
	close(fp);
	if(red<0) perror_exit("didnt read from file");
	if(read(newsock, buf, sizeof(buf)) < 0)
		printf("read.\n");
	if(strcmp(buf,"ok")!=0)
		perror_exit("Worker failed");
	printf("file SENT!\n");
}
void Sentdir(int newsock, char* filename, int casse){
	char buf[maxsize], *file;
	struct dirent *ent ;
	DIR *dir;
	strcpy(buf,"dir|");
	if(casse==2)
		strcat(buf,filename);
	if(write(newsock, buf, sizeof(buf))< 0)	
		perror_exit ("write");	
	if(read(newsock, buf, sizeof(buf)) < 0)
		perror_exit("read");
	if(strcmp(buf,"ok")!=0)
		perror_exit("Worker not ok");

	if((dir = opendir(filename))!= NULL){
		while((ent= readdir(dir)) != NULL) {
			if(strcmp(ent->d_name,".")==0 || strcmp(ent->d_name,"..")==0)	continue;

			file = malloc(sizeof(char)*(strlen(filename)+strlen(ent->d_name)+1));
			strcpy(file,filename);
			strcat(file, "/");
			strcat(file,ent->d_name);
			if(!isDir(file)){
				printf("file %s\n",file);
				Sentfile(newsock,file,2);
			}
			else{
				printf("dir %s\n",file);
				Sentdir(newsock,file,2);
			}
		}
		closedir(dir);
	}
	else
		perror_exit("opendir");
	strcpy(buf,"end_dir");
	if(write(newsock, buf, sizeof(buf))< 0)	
		perror_exit ("write");
	free(file);
	printf("dir sent!\n");
}

int main(int argc, char *argv[]){
	signal(SIGINT, sighandler);
	printf("\t*ContentServer*\n");
	list = createList();
	int i, port;
	char *dirorfilename;
	if (argc > 1){//collecting the arguments
	    for (i=1; i<argc; i++){
    		if(strcmp(argv[i], "-p")==0)
    			port = atoi(argv[i+1]);
			else if(strcmp(argv[i], "-d")==0){
				dirorfilename = (char*)malloc(strlen(argv[i+1])*sizeof(char));
				strcpy(dirorfilename, argv[i+1]);
			}
    	}
    }
    printf("our port%d, dirofnme:%s\n",port, dirorfilename );

    //int sock, newsock;
	struct sockaddr_in server, client;
	socklen_t clientlen;
	struct sockaddr *serverptr =( struct sockaddr *)& server ;
	struct sockaddr *clientptr =( struct sockaddr *)& client ;
	struct hostent *rem ;
	if (( sock = socket ( AF_INET , SOCK_STREAM , 0) ) < 0)
		perror_exit ("socket");
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(port);
	if(bind(sock, serverptr, sizeof(server)) < 0)
		perror_exit ("bind");
	if(listen(sock, 5) < 0) 
		perror_exit ( "listen") ;
	printf ("Listening for connections to port %d\n" , port );

	char buf[maxsize], temp1[maxsize], *token, s[2]=" ";
	int mydelay, pid, k;
	int ourid;
	while(1) {
		if((newsock = accept(sock, clientptr, &clientlen)) < 0) 
			perror_exit ("accept ") ;
		if(read(newsock, buf, sizeof(buf)) < 0)
			perror_exit("read");
		strcpy(temp1, buf);
		printf("==>CS:read: %s\n",buf);
		token = strtok(temp1,s);
		if(strcmp(token,"LIST")==0){
			k=0;
			do{
				token=strtok(NULL,s);
				if(k==0)
					ourid = atoi(token);//our id
				else if(k==1)
					mydelay = atoi(token);
				k++;
			}while(token!=NULL);
		//==================ANSwER=============================//
			printf("ContentS: ourid: %d, mydelay: %d ----- %s\n",ourid, mydelay, dirorfilename);
			add2List( list, ourid, mydelay);
			pid=fork();
			if(pid==0){
				char *ourlist;
				ourlist= maketheList(dirorfilename);
				printf("OURLIST->\n%s\n",ourlist );
				if(write(newsock, ourlist, strlen(ourlist))< 0)	
					perror_exit ("write" );
				printf("LIST done.\n");
				close(newsock);
				exit(1);
			}
			else{
				close(newsock);
				continue;
			}
		}
		else if(strcmp(token,"FETCH")==0){
			pid=fork();
			if(pid==0){
				token=strtok(NULL,s);
				mydelay = findList(list, atoi(token));
				printf("DELAY:%d--- id %d\n",mydelay, atoi(token));
				sleep(mydelay);
				token=strtok(NULL,s);
				if(!isDir(token))
					Sentfile(newsock, token, 1);
				else
					Sentdir(newsock, token, 1);
				break;
				close(newsock);
				exit(1);
			}
			else{
				close(newsock);
				continue;
			}
		}
	}
	close(sock);
    free(dirorfilename);
}

void perror_exit ( char * message ){
	perror(message );
	exit(EXIT_FAILURE);
}
