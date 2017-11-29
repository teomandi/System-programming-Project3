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
#include <ctype.h> /* toupper */
#include <signal.h> /* signal */
#include <pthread.h> /* For threads */
#include <semaphore.h>

#define perror2(s,e) fprintf(stderr, "%s: %s \n",s,strerror(e))

#include "list.c"
#define	maxsize 2048
#define mediumsize 512
#define buffer_size 10
void perror_exit (char *message);


//./mirrorserver -m /home/teomandi/Desktop -w 1 -p 9038


typedef struct content_info{
	int contentPort;
	char *ContentServerAdress;
	int delay;
	char *dirofile;
}content_info;

typedef struct MM_data{
	content_info ci;
	pthread_t id;
	List *buffer;
}MM_data;

typedef struct W_data{
	pthread_t id;
	char *dirname;
	List *buffer;
}W_data;

/*our mutex to control the buffer*/
pthread_mutex_t mutex;
sem_t full, empty;

int MManagerOver=0;
int bytesTransferred=0; 
int filesTransferred=0;
int ontotites=0;
int numDevicesDone=0;

int *bytesArray;
int posArray=0;

content_info *ci;
int CSnum=0, sock, newsock, ID=0;
List *buffer;
char* dirname;

void sighandler(){
	int i;
	close(newsock);
	close(sock);
 	for(i=0;i<CSnum;i++){
		free(ci[i].dirofile);
		free(ci[i].ContentServerAdress);
	}
	free(bytesArray);
	free(ci);
	destroyList(buffer);
	free(dirname);
	printf("%s\n", "Mirror Server is shuting down.\nBYE\n");
}



void * MirrorManager(void * mdata){
	MM_data *my_data=(MM_data*) mdata;
	my_data->id = pthread_self();
	int sock;
	char buf[mediumsize], *token1, s[2]="\n", temp[mediumsize];
	/*we are client*/
	struct sockaddr_in server ;
	struct sockaddr* serverptr = ( struct sockaddr *) & server ;
	struct hostent* rem;
	if (( sock = socket (AF_INET, SOCK_STREAM , 0) ) < 0)
		perror_exit ("socket");
	if (( rem = gethostbyname (my_data->ci.ContentServerAdress) ) == NULL ){
		herror("gethostbyname"); exit(1) ;
	}
	server.sin_family = AF_INET ; 
	memcpy(&server.sin_addr, rem->h_addr, rem->h_length );
	server.sin_port = htons ( my_data->ci.contentPort ); 
	if (connect(sock , serverptr , sizeof(server)) < 0)
		perror_exit ("connect ");
	printf ("Connecting to %s port %d\n" , my_data->ci.ContentServerAdress , my_data->ci.contentPort );

	pthread_mutex_lock(&mutex);
	sprintf(buf,"LIST %d %d",ID, my_data->ci.delay);
	ID++;
	pthread_mutex_unlock(&mutex);


	if(write(sock, buf, sizeof(buf))< 0)
		perror_exit ("write");

	memset(buf,0,mediumsize);
	char *list = realloc(NULL,sizeof(char)*mediumsize);
	int max_size=mediumsize;
	int num=0;
	int size=0;
	while((num=read(sock, buf, mediumsize)) > 0){
		size=size+num;
		if(max_size < size){
			list = realloc(list,sizeof(char)*(max_size+=mediumsize));
		}
		if(strlen(list)==0)
			strcpy(list,buf);
		else
			strncat(list, buf, num);
	}
	close(sock);
	printf("CS list response:\n%s\n",list);
	printf("ourwant:%s\n",my_data->ci.dirofile);
	
	char *templist = malloc(sizeof(char)*strlen(list));
	
	int k=0, end=0, i;
	while(!end){
		strcpy(templist,list);
		token1=strtok(templist,s);
		for(i=0;i<k;i++)
			token1=strtok(NULL,s);
		if(token1==NULL){
			end=1;
			break;
		}
		k++;
		if(strcmp(my_data->ci.dirofile,token1)==0){
			pthread_mutex_lock(&mutex);
			sprintf(temp,"%s,%d,%s,%d,%s",token1,my_data->ci.contentPort, my_data->ci.ContentServerAdress,ID-1, "end");
			sem_wait(&empty);
			if(!add2List(my_data->buffer,temp))
				perror_exit("Buffer is full");
			printf("PRODUCED: MirrorManger: %d->%s\n",my_data->buffer->size, temp);
			pthread_mutex_unlock(&mutex);
			sem_post(&full);
			break;
		}
	}
	printf("->MM printing list before terminate:");
	printList(my_data->buffer);
	free(my_data);
	free(list);
	free(templist);
	pthread_exit(NULL);
}

void Storefile(char *path, int sock, char *ourfile){
	printf("Store new FILE at:%s\n",path );
	int i,j, x=0, namelen=0, slash=0;
	char *filename, buf[mediumsize], *temppath;//[mediumsize];
	temppath=malloc(sizeof(char)*strlen(path));
	strcpy(temppath,path);
	for(i=0;i<strlen(ourfile);i++){
		if(ourfile[i]=='/'){
			slash++;
			namelen=0;
		}
		namelen++;
	}
	filename = malloc(sizeof(char)*namelen);
	for(i=0;i<strlen(ourfile);i++){
		if(ourfile[i]=='/')
			x++;
		if(x==slash){
			for(j=0;j<namelen;j++){
				filename[j]=ourfile[i];
				i++;
			}
			break;
		}
	}
	temppath= realloc(temppath, sizeof(char)*(strlen(temppath)+namelen));
	strcat(temppath, filename);
	int fp = open(temppath, O_WRONLY | O_CREAT);
	if(fp<0) perror_exit("SF:File didnt open.");
	/*answer to soket ok to start sending bytes*/
	strcpy(buf,"ok");
	if(write(sock, buf, sizeof(buf))< 0)
		perror_exit ("write" );
	/*read the bytes to write ourfile*/
	
	pthread_mutex_lock(&mutex);

	while(read(sock, buf, sizeof(buf))>0){
		if(strcmp(buf,"end")==0)
			break;
		bytesTransferred = bytesTransferred + sizeof(buf);
		bytesArray[posArray] = bytesArray[posArray] + sizeof(buf);
		write(fp, buf, sizeof(buf));
		strcpy(buf,"ok");
		if(write(sock, buf, sizeof(buf))< 0)
			perror_exit ("write");
	}
	posArray++;
	ontotites++;
	filesTransferred++;
	strcpy(buf,"ok");
	if(write(sock, buf, sizeof(buf)) < 0)
		perror_exit ("write");

	pthread_mutex_unlock(&mutex);
	
	close(fp);
	free(filename);
	free(temppath);
	printf("File STORED!\n");
}

void StoreDir(char *path, int sock, char *ourdir){
	printf("Store new DIR at:%s\n",path );
	char buf[mediumsize], *dirname, *token, *pathtemp;
	int i,j, x=0, namelen=0, slash=0;
	pathtemp=malloc(sizeof(char)*strlen(path));
	strcpy(pathtemp,path);
	for(i=0;i<strlen(ourdir);i++){
		if(ourdir[i]=='/'){
			slash++;
			namelen=0;
		}
		namelen++;
	}
	dirname = malloc(sizeof(char)*namelen);
	for(i=0;i<strlen(ourdir);i++){
		if(ourdir[i]=='/')
			x++;
		if(x==slash){
			for(j=0;j<namelen;j++){
				dirname[j]=ourdir[i];
				i++;
			}
			break;
		}
	}
	pathtemp= realloc(pathtemp, sizeof(char)*(strlen(pathtemp)+namelen));
	strcat(pathtemp, dirname);
	mkdir(pathtemp, 0777);

	pthread_mutex_lock(&mutex);
	ontotites++;
	pthread_mutex_unlock(&mutex);

	strcpy(buf,"ok");
	if(write(sock, buf, sizeof(buf))< 0)
		perror_exit ("write" );
	if(read(sock, buf, sizeof(buf)) < 0)
		perror_exit ("read");
	while(strcmp(buf,"end_dir")!=0){
		token = strtok(buf,"|");
		if(strcmp(token,"dir")==0){
			token=strtok(NULL,"|");
			char *temp =malloc(sizeof(char)*strlen(token));
			strcpy(temp,token);
			StoreDir(pathtemp, sock, temp);
			free(temp);
		}
		else{
			char *temp =malloc(sizeof(char)*strlen(token));
			strcpy(temp,token);
			Storefile(pathtemp, sock, temp);
			free(temp);
		}
		if(read(sock, buf, sizeof(buf)) < 0)
			perror_exit ("read");
	}
	free(dirname);
	free(pathtemp);
	printf("dir STORED!\n");
}

void* Worker(void *wdata){//we should free the info from char arrays
	W_data *my_data=(W_data*) wdata;
	my_data->id = pthread_self();
	int k,i, contentPort, sock, idd;
	char *data, *token, s[2]=",",*contentAdress, *ourfile, buf[mediumsize];
	while(!MManagerOver){
		while(!isEmpty(my_data->buffer)){
			sem_wait(&full);
			pthread_mutex_lock(&mutex);
			data=removeLast(my_data->buffer);
			printf("CONSUMED: Worker: %d->%s\n",my_data->buffer->size, data);
			pthread_mutex_unlock(&mutex);
			sem_post(&empty);
			k=0;
			token=strtok(data,s);
			do{
				if(k==0){
					ourfile = malloc(sizeof(char)*strlen(token));
					strcpy(ourfile,token);
				}
				else if(k==1)
					contentPort=atoi(token);
				else if(k==2){
					contentAdress=malloc(sizeof(char)*strlen(token));
					strcpy(contentAdress,token);
				}
				else if(k==3)
					idd=atoi(token);
				k++;
				token=strtok(NULL,s);
			}while(token!=NULL);

			/*make our path to store the files*/
			char *path, folder_name[mediumsize];
			path = malloc(sizeof(char)*strlen(my_data->dirname));
			strcpy(path,my_data->dirname);
			sprintf(folder_name,"/%s_%d",contentAdress, contentPort);
			path =  realloc(path, sizeof(char)*(strlen(path)+strlen(folder_name)));
			strcat(path, folder_name);
			mkdir(path, 0777);

			/*Connect with Content Server */
			struct sockaddr_in server ;
			struct sockaddr* serverptr = ( struct sockaddr *) & server ;
			struct hostent* rem;
			if(( sock = socket (AF_INET, SOCK_STREAM, 0) ) < 0)
				perror_exit ("socket ");
			if(( rem = gethostbyname (contentAdress) ) == NULL ){
				herror ("gethostbyname "); exit(1) ;
			}
			server.sin_family = AF_INET ; 
			memcpy (&server.sin_addr, rem->h_addr, rem->h_length );
			server.sin_port = htons(contentPort); 
			if (connect(sock , serverptr , sizeof ( server )) < 0)
				perror_exit ("connect ");
			printf ( "Worker_Connecting to %s port %d\n" , contentAdress, contentPort );
			sprintf(buf,"FETCH %d %s",idd,ourfile);
			if(write(sock, buf, sizeof(buf))< 0)
				perror_exit ("write");

			if (read(sock, buf, sizeof(buf)) < 0)
				perror_exit ("read");
			token=strtok(buf,"|");
			if(strcmp(token,"file")==0)
				Storefile(path, sock, ourfile);
			else
				StoreDir(path, sock, ourfile);
			free(data);
			free(contentAdress);
			free(ourfile);
		}

	}
	printf("Worker %ld ENDS\n",pthread_self());
	free(my_data);
	pthread_exit(NULL);
}



int main(int argc, char *argv[]){
	signal(SIGINT, sighandler);
	printf("\t*MirrorServer *\n");
	int i, port, thnum;
	char buf[maxsize];
	if (argc > 1){//collecting the arguments
	    for (i=1; i<argc; i++){
    		if(strcmp(argv[i], "-p")==0)
    			port = atoi(argv[i+1]);
			else if(strcmp(argv[i], "-m")==0){
				dirname = (char*)malloc(strlen(argv[i+1])*sizeof(char));
				strcpy(dirname, argv[i+1]);
			}
			else if(strcmp(argv[i], "-w")==0)
    			thnum = atoi(argv[i+1]);
    	}
    }
    printf("--our port:%d, thrnum:%d, dir:%s--\n",port, thnum, dirname );

    /*take our data from MirrorInitiator*/
	struct sockaddr_in server , client ;
	socklen_t clientlen ;
	struct sockaddr *serverptr =( struct sockaddr *) & server ;
	struct sockaddr *clientptr =( struct sockaddr *) & client ;
	struct hostent *rem;
	if(( sock = socket ( AF_INET , SOCK_STREAM , 0) ) < 0)
		perror_exit ("socket ");
	server.sin_family = AF_INET; /* Internet domain */
	server.sin_addr . s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(port);
	if(bind(sock, serverptr, sizeof(server)) < 0)
		perror_exit("bind");
	if(listen(sock, 5) < 0)
		perror_exit ("listen ");
	printf ( "Listening for connections to port %d\n" , port );
while(1){
	if (( newsock = accept ( sock , clientptr , & clientlen ) ) < 0) 
		perror_exit ("accept ") ;
	if(read(newsock, buf, sizeof(buf)) < 0)
		perror_exit ("read" );
	if(write(newsock, "ok", sizeof("ok")) < 0)
		perror_exit("write");
	/*take the info about the contents*/
	char temp1[mediumsize], temp2[mediumsize], *token, *tokenplus, s1[2]=",", s2[2]=":";
	int end, j, k;
	strcpy(temp1, buf);
	token = strtok(temp1,s1);
  	do{
  		CSnum++;	
  		token = strtok(NULL, s1);
  	}while( token != NULL );
  	/*================================================================================================*/

  	ci = malloc(sizeof(content_info)*CSnum);

  	end=0;
  	j=0;
  	strcpy(temp1, buf);
  	token = strtok(temp1,s1);
  	do{	
  		k=0;
  		strcpy(temp2,token);
		tokenplus = strtok(temp2,s2);
		do{
			if(k==0){
	   			ci[j].ContentServerAdress=(char*)malloc(strlen(tokenplus)*sizeof(char));
				strcpy(ci[j].ContentServerAdress,tokenplus);
			}
			else if(k==1)
				ci[j].contentPort=atoi(tokenplus);
			else if(k==2){
				ci[j].dirofile=(char*)malloc(strlen(tokenplus)*sizeof(char));
				strcpy(ci[j].dirofile,tokenplus);
			}
			else if(k==3)
				ci[j].delay=atoi(tokenplus);
			tokenplus=strtok(NULL,s2);	
			k++;
			}while( tokenplus != NULL);
		j++;
		strcpy(temp1, buf);
		token = strtok(temp1,s1);
		for(k=0;k<j;k++){
			token = strtok(NULL, s1);
			if(token==NULL)
				end=1;
			}
  	}while(!end);

	/*Mirror Managers creation*/
	buffer = createList(buffer_size);
	pthread_mutex_init(&mutex, NULL);
	sem_init(&full, 0, 0);
	sem_init(&empty, 0, buffer_size);

	int err, status;
	pthread_t thr[CSnum];
	pthread_t workers[thnum];

	bytesArray = malloc(sizeof(int)*CSnum);//for diaspora
	
	for(i=0; i<CSnum;i++){
		bytesArray[i] = 0;//init the array

		MM_data *mdata = malloc(sizeof(MM_data));
		mdata->ci = ci[i];
		mdata->buffer=buffer;

		if(err=pthread_create(&thr[i], NULL, MirrorManager, (void*)mdata)){
			perror2("pthread_create", err );
			exit(1);
		}
		printf("MirrorManager%d thread %ld CREATED. \n", i, thr[i]);
	}

	/*Workers creation*/
	for(i=0; i<thnum; i++){
		W_data *wdata = malloc(sizeof(W_data));
		wdata->buffer=buffer;
		wdata->dirname=dirname;
		if(err=pthread_create(&workers[i], NULL, Worker, (void*)wdata)){
			perror2("pthread_create", err );
			exit(1);
		}
		printf("Worker%d thread %ld CREATED\n", i, workers[i]);
	}
	for(i=0; i<CSnum; i++){
		int k = i;
		if(err=pthread_join(thr[k], (void **)&status)){
			perror2("pthread_join_MirrorManagers ", err); 
			exit(1);
		}
		i=k;
	}
	

	printf("Mirror Managers finished!\n");
	MManagerOver =1; //mirror managers are over
	printf("wait/join workers!\n");
	for(i=0; i<thnum; i++){
		int k=i;
		if(err=pthread_join(workers[i], (void **)&status)){
			perror2("pthread_join_workers", err); 
			exit(1);
		}
		i=k;
	}
	printf("Workers finished!\nPrinting results:\n\t-bytesTransferred: %d\n\t-filesTransferred: %d\n\t-Ontotites: %d\n",bytesTransferred, filesTransferred, ontotites);
	int avgBytes = bytesTransferred/filesTransferred;
	int Sum;
	float s;//diaspora
	for(i=0;i<CSnum;i++)
		Sum = Sum + (avgBytes - bytesArray[i]);
	s=Sum/filesTransferred;
	if(read(newsock, buf, sizeof(buf)) < 0)
		perror_exit ("read" );
	sprintf(buf,"%d:%f",avgBytes, s);//mesos oros k diaspora
	printf("last buf: %s\n",buf);
	if(write(newsock, buf, sizeof(buf)) < 0)
		perror_exit("write");
	close(newsock);	
}
	
	close(sock);
/*=========== FREE MEMORY ===============*/
  	for(i=0;i<CSnum;i++){
		free(ci[i].dirofile);
		free(ci[i].ContentServerAdress);
	}
	free(bytesArray);
	free(ci);
	destroyList(buffer);
	free(dirname);
	printf("%s\n", "Mirror Server is shuting down\nBYEE\n");
}

void perror_exit (char * message){
	perror (message);
	exit(EXIT_FAILURE);
}