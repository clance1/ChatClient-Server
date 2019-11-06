/*  client.c
    Cole Pickford
		Carson Lance
		Jack Conway
*/

#include <iostream>
#include <unistd.h>
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

using namespace std;

#define MD5LEN 32
#define SERVER_PORT 41036
#define MIN(a,b) (((a)<(b)) ? (a) : (b))

bool ack, confirmed, result;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *connection_handler(void *);
int char_send(int, char*, int);
int char_recv(int, char*, int);
int int_send(int, int);
int int_recv(int);
void broadcast(int);
void private_chat(int, char*);
void acknowledgement_handler();
bool confirmation_handler();

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

	// Receive status
	int status_recv = int_recv(sockfd);

	// New user
	if (status_recv == 1) {
		// Receive new user creation ack
		memset(greet, 0, sizeof(greet));
		int creation = char_recv(sockfd, greet, sizeof(greet));
		printf("%s", greet);

		//return password request
		char new_pass[50];
		fgets(new_pass, 50, stdin);
		int new_password_sent = char_send(sockfd, new_pass, strlen(new_pass));
	}

	char login_success[BUFSIZ] = "Login Successful\n";
	// Handle password
	while(strcmp(login_success, greet)) {
			//revieve password request
			memset(greet, 0, sizeof(greet));
			int password_received = char_recv(sockfd, greet, sizeof(greet));
			printf("%s", greet);

			//return password request
			char password[50];
			fgets(password, 50, stdin);
			int password_sent = char_send(sockfd, password, strlen(password));

			//revieve password acknowledgement
			memset(greet, 0, sizeof(greet));
			int password_acknowledgment = char_recv(sockfd, greet, sizeof(greet));
			printf("%s", greet);
	}

	pthread_t thread_id;
	if(pthread_create( &thread_id , NULL ,  connection_handler, (void*) &sockfd) < 0){
			perror("could not create thread");
			return 1;
	}

	char input[50];
	while(strcmp(input, "X")) {
		printf("Enter operation\n");
		printf("B: Broadcast Messaging\nP: Private Messaging\nH: Show History\nX: Exit\n");
		fgets(input, sizeof(input), stdin);
		size_t l = strlen(input) - 1;
		if (input[l] == '\n')
			input[l] = '\0';

		if (!strcmp(input, "B") || !strcmp(input, "P") || !strcmp(input, "H") || !strcmp(input, "X")) {
				int send_code = char_send(sockfd, input, sizeof(input));
		}

		acknowledgement_handler();

		if (!strcmp(input, "B")) {
			printf("Broadcast selected\n");
			broadcast(sockfd);
		}
		else if (!strcmp(input, "P")) {
			printf("Private message selected\n");
			private_chat(sockfd, username);
		}
		else if (!strcmp(input, "H")) {
			printf("History selected\n");
			acknowledgement_handler();
		}
		else if (!strcmp(input, "X")) {
			printf("Exiting\n");
			acknowledgement_handler();
			pthread_join(thread_id, NULL);
			break;
		}
		else {
			printf("Invalid Selection\n");
		}
	}
	close(sockfd);
	return 0;
}

void *connection_handler(void *sock) {

		int sockfd = *(int*)sock;
		//recieve greeting
		int count = 0;
		char message[BUFSIZ]= "";
		while(1 && count++ < 20) {
			memset(message, 0, sizeof(message));
			int greet_received = char_recv(sockfd, message, sizeof(message));

			if (!strcmp(message, "ACK")) {
				ack = true;
			}
			else if (!strcmp(message, "SUCCESS")) {
				confirmed = true;
				result = true;
			}
			else if (!strcmp(message, "FAILURE")) {
				confirmed = true;
				result = false;
			}
			else if (!strcmp(message, "EXIT")) {
				cout << "EXITING" << endl;
				ack = true;
				confirmed = true;
				result = true;
				break;
			}
			else {
				pthread_mutex_lock(&mutex);
				printf("%s", message);
				fflush(stdout);
				pthread_mutex_unlock(&mutex);
				cout << "Unlocked" << endl;
			}
		}
		return 0;
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

void broadcast(int sockfd) {
	printf("Enter your message: \n> ");
	fflush(stdout);

	// Send message
	char message[BUFSIZ];
	fgets(message, sizeof(message), stdin);

	int broadcast_send = char_send(sockfd, message, sizeof(message));
	fflush(stdout);

	if (confirmation_handler()) {
		printf("Broadcast Successful\n");
	}
	else {
		printf("Error with broadcast\n");
	}
}

void private_chat(int sockfd, char* username) {

	acknowledgement_handler();

	// Select user
	char user_selected[BUFSIZ];
	printf("Enter the user you would like to slide into dms with: ");
	fgets(user_selected,  sizeof(user_selected), stdin);
	int user_send = char_send(sockfd, user_selected, sizeof(user_selected));
	fflush(stdout);

	// Enter message
	char message[BUFSIZ];
	printf("Enter message to be sent:\n");
	fgets(message, sizeof(message), stdin);

	int message_send = char_send(sockfd, message, sizeof(message));

	if (confirmation_handler()) {
		printf("Successfully sent.\n");
	}
	else {
		printf("User does not exist\n");
	}
}

void acknowledgement_handler() {
	while (!ack);

	ack = false;
}

bool confirmation_handler() {
	while(!confirmed);

	confirmed = false;
	return result;
}
