/*  client.c
    Cole Pickford
		Carson Lance
		Jack Conway
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
void broadcast(int);
void private(int);
void history(int);

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

				// Receive status
				int status_recv = int_recv(sockfd);

				// New user
				if (status_recv == 1) {
					// Receive new user creation ack
					memset(greet, 0, sizeof(greet));
					int creation = char_recv(sockfd, greet, sizeof(greet));
					printf("%s", greet);

					//revieve password request
					memset(greet, 0, sizeof(greet));
					int new_password_request = char_recv(sockfd, greet, sizeof(greet));
					printf("%s", greet);

					//return password request
					char new_pass[50];
					fgets(new_pass, 50, stdin);
					int new_password_sent = char_send(sockfd, new_pass, strlen(new_pass));
				}
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

				char input[50];
				while(strcmp(input, "X")) {
					printf("Enter operation\n");
					printf("B: Broadcast Messaging\n P: Private Messaging\nH: Show History\nX: Exit");
					fgets(input, sizeof(input), stdin);
					size_t l = strlen(input) - 1;
					if (input[l] == '\n')
						input[l] = '\0';

					if (!strcmp(input, "B")) {
						printf("Broadcast selected\n");
						broadcast(sockfd);
					}
					else if (!strcmp(input, "P")) {
						printf("Private message selected\n");
						private(sockfd);
					}
					else if (!strcmp(input, "H")) {
						printf("History selected\n");
						history(sockfd);
					}
					else if (!strcmp(input, "X")) {
						printf("Exiting\n");
						char exit[BUFSIZ] = "X";
						int exit_send = char_send(sockfd, exit, sizeof(exit));
					}
					else {
						printf("Invalid Selection\n");
					}
				}
				break;
    }
		close(sockfd);
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
	// Communicate inital broadcast with server
	char initiate[50] = "broadcast";
	int initate_response = char_send(sockfd, initiate, 50);

	// Receive acknowledgement
	char greet[BUFSIZ];
	memset(greet, 0, sizeof(greet));
	int greet_received = char_recv(sockfd, greet, sizeof(greet));
	printf("%s", greet);

	// Send message
	char message[BUFSIZ];
	printf("Enter your message to be broadcasted:");
	fgets(message, sizeof(message), stdin);
	int broadcast_send = char_send(sockfd, message, sizeof(message));

	// Receive confimation
	int confirmation = int_recv(sockfd);

	if (confirmation == 0) {
		printf("Broadcast Successful\n");
	}
	else {
		printf("Error with broadcast\n");
	}
}

void private(int sockfd) {
	// Communicate inital state with server
	char initiate[50] = "private";
	int initate_response = char_send(sockfd, initiate, 50);

	printf("List of online users:\n");
	// Receive acknowledgement
	char users[BUFSIZ];
	memset(users, 0, sizeof(users));
	int users_received = char_recv(sockfd, users, sizeof(users));
	printf("%s", users);

	// Select user
	char user_selected[BUFSIZ];
	printf("Enter the user you would like to slide into dms with: ");
	fgets(user_selected,  sizeof(user_selected), stdin);
	int user_send = char_send(sockfd, user_selected, sizeof(user_selected));

	// Enter message
	char message[BUFSIZ];
	printf("Enter message to be sent:\n");
	fgets(message, sizeof(message), stdin);
	int message_send = char_send(sockfd, message, sizeof(message));

	// Receive acknowledgement
	int confimation = int_recv(sockfd);
	if (confimation == 0) {
		printf("Message sent\n");
	}
	else if (confimation == 1) {
		printf("User not found\n");
	}
	else {
		printf("Miscelaneous error\n");
	}
}

void history(int sockfd) {
	// Communicate inital state with server
	char initiate[50] = "history";
	int initate_response = char_send(sockfd, initiate, 50);

	// Receive History
	char history[BUFSIZ];
	memset(history, 0, sizeof(history));
	int hist_received = char_recv(sockfd, history, sizeof(history));
	printf("%s", history);
}
