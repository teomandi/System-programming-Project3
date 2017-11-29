# include <stdio.h>
# include <sys/types.h> /* sockets */
# include <sys/socket.h> /* sockets */
# include <netinet/in.h> /* internet sockets */
# include <unistd.h> /* read , write , close */
# include <netdb.h> /* ge th os tb ya dd r */
# include <stdlib.h> /* exit */
# include <string.h> /* strlen */

#include <sys/wait.h> /* sockets */
#include <ctype.h> /* toupper */
#include <signal.h> /* signal */

#define maxsize 512
void perror_exit ( char * message );

int main(int argc, char *argv[]){
	printf("\t*Mirror Initiator*\n");
	int i, mirrorPort, *contentPort, *delay, CSnum=0, k, j, end;
	char MirrorServerAdress[maxsize], **ContentServerAdress, **dirofile, *token, *tokenplus, s1[2]=",", s2[2]=":", contents[maxsize];
	if (argc > 1){//collecting the arguments
    	for (i = 1; i < argc; i++){
    		if(strcmp(argv[i], "-n")==0)
    			strcpy(MirrorServerAdress, argv[i+1]);
	  		else if(strcmp(argv[i], "-p")==0)
	  			mirrorPort=atoi(argv[i+1]);
	  		else if(strcmp(argv[i], "-s")==0)
	  			strcpy(contents, argv[i+1]);
	  	}
	}
	printf("~~MS:Ad: %s Port: %d~~\n",MirrorServerAdress, mirrorPort);

	/*Sent them through internet to the MirrorServer.*/
	char buf[maxsize];
	int port , sock ;
	struct sockaddr_in server ;
	struct sockaddr * serverptr = ( struct sockaddr *) & server ;
	struct hostent * rem ;
	if (( sock = socket (AF_INET, SOCK_STREAM, 0))< 0)
		perror_exit ("socket");
	if (( rem = gethostbyname (MirrorServerAdress)) == NULL ) {
		herror ( "gethostbyname");
		exit (1) ;
	}	
	port = mirrorPort; /* Convert port number to integer */
	server.sin_family =AF_INET ; /* Internet domain */
	memcpy (& server.sin_addr, rem->h_addr, rem->h_length);
	server.sin_port = htons ( port );
	if ( connect (sock, serverptr, sizeof(server))<0)
		perror_exit ("connect");
	printf ("Connecting to %s port %d\n", MirrorServerAdress, port);

	if(write(sock, contents, sizeof(contents)) < 0)
		perror_exit("write");
	if (read(sock, buf, sizeof(buf)) < 0)
		perror_exit ("read");
	if(strcmp(buf,"ok")!=0){
		printf("Error in sending info for Content servers\n");
		exit(1);
	}
	if(write(sock, "ok", sizeof("ok")) < 0)
		perror_exit("write");

	if(read(sock, buf, sizeof(buf)) < 0)
		perror_exit ("read");
	token=strtok(buf,":");
	i=0;
	printf("RESULTS:\n");
	if(token!=NULL){
		printf("\t-Average size of bytes: %s\n", token);
		token=strtok(NULL,":");
		printf("\t-Diaspora: %s\n", token);
	}

	close(sock);
	printf("END.\n");
}



void perror_exit(char *message ){
	perror(message);
	exit(EXIT_FAILURE);
}