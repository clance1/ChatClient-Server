/*  client.c
    Cole Pickford
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>

#define MD5LEN 32
#define SERVER_PORT 41036
#define MIN(a,b) (((a)<(b)) ? (a) : (b))

int char_send(int, char*, int);
int char_recv(int, char*, int);
int int_send(int, int);
int int_recv(int);

int main(int argc, char *argv[]) {
    // Check argument count
	if(argc != 3) {
		printf("%s: Incorrect usage.\n Usage: %s ADDR PORT \n", argv[0], argv[0]);
		exit(1);
	}

	int port = atoi(argv[2]);

	int sockfd;
    //create socket
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )  {
		printf("Client: Error creating socket\n");
		exit(0);
	}

	struct sockaddr_in sin;

	memset(&sin, 0, sizeof(sin));

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr*) gethostbyname(argv[1])->h_addr_list[0])));
	sin.sin_port = htons(port);

	// connect
	printf("Connection to %s on port %d\n", argv[1], port);
	if (connect(sockfd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		printf("Client: Error connecting to server: %s\n", strerror(errno));
		exit(0);
	}
	printf("Connection established\n");


    //While loop handles password checking
    while(1){
        //recieve greeting
		char greet[BUFSIZ];
		memset(greet, 0, sizeof(greet));
		int greet_received = char_recv(sockfd, greet, sizeof(greet));
		printf("%s", greet);

		//recieve username request
		memset(greet, 0, sizeof(greet));
		int username_received = char_recv(sockfd, greet, sizeof(greet));
		printf("%s", greet);

		//return username request
		char username[50];
		fgets(username, 50, stdin);
		printf("Entered username:%s", username);
		int username_sent = char_send(sockfd, username, strlen(username));

		//revieve password request
		memset(greet, 0, sizeof(greet));
		int password_received = char_recv(sockfd, greet, sizeof(greet));
		printf("%s", greet);

		//return password request
		char password[50];
		fgets(password, 50, stdin);
		int password_sent = char_send(sockfd, password, strlen(password));

    }
}

//send strings
int char_send(int sockfd, char* buffer, int size) {
	int len;
	if ((len = write(sockfd, buffer, size)) == -1) {
		perror("ERROR: Sending string");
		exit(1);
	}
	return len;
}

//receive strings
int char_recv(int sockfd, char* buffer, int size) {
	int len;
	bzero(buffer, sizeof(buffer));
	if ((len = read(sockfd, buffer, size)) == -1) {
		perror("ERROR: Recieving string");
		exit(1);
	}
	return len;
}

//send integers
int int_send(int value, int sockfd) {
	int len;
	uint32_t temp = htonl(value);
	if ((len = write(sockfd, &temp, sizeof(temp))) == -1) {
		perror("ERROR: Sending size");
		exit(1);
	}

	return len;
}

//receive integers
int int_recv(int sockfd) {
	int buffer;
	int len;
	bzero(&buffer, sizeof(buffer));
	if ((len = read(sockfd, &buffer, sizeof(buffer))) == -1) {
		perror("ERROR: Receiving size");
		exit(1);
	}

	int temp = ntohl(buffer);
	return temp;
}
