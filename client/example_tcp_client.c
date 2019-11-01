/*
 * Program: myftp.c
 * Carson Lance (clance1)
 * Cole Pickford (cpickfor)
 * Jack Conway (jconway7)
 *
 * Simple TCP FTP client that interacts with created server
 * on a designated port
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
#define SERVER_PORT 41026
#define MIN(a,b) (((a)<(b)) ? (a) : (b))

void DNLD(int);
void UPLD(int);
void CRFL(int);
void RMFL(int);
void LIST(int);
void MKDR(int);
void RMDR(int);
void CHDR(int);
int char_send(int, char*, int);
int char_recv(int, char*, int);
int int_send(int, int);
int int_recv(int);
static long get_time();

int main(int argc, char *argv[]) {

	// Check argument count
	if(argc != 3) {
		printf("%s: Incorrect usage.\n Usage: %s ADDR PORT \n", argv[0], argv[0]);
		exit(1);
	}

	int port = atoi(argv[2]);

	int sockfd;
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

	char *input;

	while(1) {

		// Wait for user input
		printf("> ");
		fgets(input, 50, stdin);

		// Chomp string
		input[strlen(input)-1] = '\0';

		char * code = strtok(input, " \t\n");

		if (strlen(code) != 4) {
			perror("ERROR: Input error\n");
			continue;
		}

		int sent = char_send(sockfd, code, strlen(code));

		// DNLD: Download
		if (!strcmp(code, "DNLD")) {
			DNLD(sockfd);
		}
		// UPLD: Upload
		else if (!strcmp(code, "UPLD")) {
			UPLD(sockfd);
		}
		// LIST: List
		else if (!strcmp(code, "LIST")) {
			LIST(sockfd);
		}
		// MKDR: Make Directories
		else if (!strcmp(code, "MKDR")) {
			MKDR(sockfd);
		}
		// RMDR: Remove Directory
		else if (!strcmp(code, "RMDR")) {
			RMDR(sockfd);
		}
		// CHDR: Change Directory
		else if (!strcmp(code, "CHDR")) {
			CHDR(sockfd);
		}
		// CRFL: Create File
		else if (!strcmp(code, "CRFL")) {
			CRFL(sockfd);
		}
		// RMFL: Remove File
		else if (!strcmp(code, "RMFL")) {
			RMFL(sockfd);
		}
		// QUIT: Quit
		else if (!strcmp(code, "QUIT")) {
			printf("Closing client\n");
			break;
		}
		// Default: invalid
		else {
			printf("Invalid command.\n");
		}

	}
	close(sockfd);
}

void DNLD(int sockfd){
	// Get file name and size
	char* filename = strtok(NULL, " ");

	if (filename == NULL) {
		printf("ERROR: Input error\n");
		exit(1);

	}

	// Send filename size
	int_send(strlen(filename), sockfd);

	// Send filename
	char_send(sockfd, filename, strlen(filename));

	// Recieve server response
	int status = int_recv(sockfd);

	if (status == -1) {
		printf("%s does not exist on server.\n", filename);
		return;
	} else {
		int filesize = status;

		// Receive md5sum
		char server_md5[MD5LEN + 1];
		bzero(server_md5, sizeof(server_md5));

		char_recv(sockfd, server_md5, MD5LEN);

		// Create file
		FILE *fp = fopen(filename, "w");

		if (fp == NULL) {
			printf("ERROR: File creation\n");
			exit(1);
		} else {

			// Recieve file data
			int total_bytes = 0;
			int bytes;
			char buffer[BUFSIZ];
			bzero((char*)&buffer, sizeof(buffer));
			long time_start = get_time();
			while(total_bytes < filesize) {
				bytes = char_recv(sockfd, buffer, MIN(BUFSIZ,filesize-bytes));
				total_bytes += bytes;
				if (fwrite(buffer, sizeof(char), bytes, fp) < bytes) {
					printf("ERROR: Writing to file\n");
					exit(1);
				}
				bzero((char*)&buffer, sizeof(buffer));
			}
			long time_end = get_time();

			// Calculate throughput
			long diff = time_end - time_start;
			double diff_s = (double)diff * 0.000001;

			double megabytes = (double)bytes * 0.000001;

			double throughput = megabytes / diff_s;

			fclose(fp);

			// Calculate md5hash
			char client_md5[MD5LEN + 1];
			bzero(client_md5, sizeof(client_md5));
			char cmd[7 + strlen(filename) + 1];

			sprintf(cmd, "md5sum %s", filename);

			// Run shell command
			FILE *p = popen(cmd, "r");
			if (p == NULL) {
				printf("Error calculating md5sum\n");
				exit(1);
			}

			int i, ch;
			for (i = 0; i < 32 && isxdigit(ch = fgetc(p)); i++) {
				client_md5[i] = ch;
			}

			client_md5[MD5LEN] = '\0';

			pclose(p);

			// Output data transfer
			printf("%d bytes transferred in %f seconds: %f MegaBytes\\sec.\n", total_bytes, diff_s, throughput);

			// Compare md5
			printf("MD5Hash: %s (%s)\n", client_md5, (!strcmp(server_md5, client_md5)) ? "matches" : "doesn't match");
		}
	}
}

void UPLD(int sockfd){
	// Get file name and size
	char* filename = strtok(NULL, " ");

	if (filename == NULL) {
		printf("ERROR: Input error\n");
		exit(1);
	}

	// Send filename length
	int_send(strlen(filename), sockfd);

	// Send filename
	char_send(sockfd, filename, strlen(filename));

	// Wait for server response
	int_recv(sockfd);

	// Check if file exits
	struct stat st;

	if (stat(filename, &st) == 0 && S_ISREG(st.st_mode)) {
		// Send file size
		int fileSize = st.st_size;
		int sent_size = int_send(fileSize, sockfd);

		// Send the actual file
		FILE *fp = fopen(filename, "r");
		char buffer[BUFSIZ + 1];
		bzero(buffer, sizeof(buffer));

		// Send file in chunks of BUFSIZ
		int sentBytes = 0;
		while(sentBytes < fileSize) {
			int bytesRead = fread(buffer, sizeof(char), BUFSIZ, fp);
			sentBytes += char_send(sockfd, buffer, bytesRead);
			bzero(buffer, sizeof(buffer));
		}

		// Receive throughput
		double throughput;
		int len;
		if ((len = read(sockfd, &throughput, sizeof(throughput))) == -1) {
			perror("Failure to read from TCP socket.\n");
			exit(1);
		}

		// Receive time
		double diff_s;
		if ((len = read(sockfd, &diff_s, sizeof(diff_s))) == -1)
		{
			perror("Failure to read from TCP socket.\n");
			exit(1);
		}

		// Receive md5sum
		char server_md5[MD5LEN + 1];
		bzero(server_md5, sizeof(server_md5));
		char_recv(sockfd, server_md5, MD5LEN);

		// Calculate md5hash
		char client_md5[MD5LEN + 1];
		bzero(client_md5, sizeof(client_md5));
		char cmd[7 + strlen(filename) + 1];

		sprintf(cmd, "md5sum %s", filename);

		// Run linux command
		FILE *p = popen(cmd, "r");
		if (p == NULL) {
			printf("Error calculating md5sum\n");
			exit(1);
		}

		int i, ch;
		for (i = 0; i < 32 && isxdigit(ch = fgetc(p)); i++) {
			client_md5[i] = ch;
		}

		client_md5[MD5LEN] = '\0';
		printf("Calculated: %s\n", client_md5);
		pclose(p);

		if (!strcmp(server_md5, client_md5)) {
			printf("%d bytes transferred in %f seconds: %f Megabytes/sec\n", fileSize, diff_s, throughput);
			printf("\tMD5Hash: %s (%s)\n", client_md5, (!strcmp(server_md5, client_md5)) ? "matches" : "doesn't match");
		} else {
			printf("Transfer failed.\n");
		}

	} else {
		int_send(-1, sockfd);
		printf("File does not exist.\n");
	}

	return;
}

void CRFL(int sockfd) {
	// Get file name and size
	char* filename = strtok(NULL, " ");

	if (filename == NULL) {
		printf("ERROR: Input error\n");
		exit(1);
	}

	int_send(strlen(filename), sockfd);

	// Send filename
	char_send(sockfd, filename, strlen(filename));

	// Recieve server response
	int status = int_recv(sockfd);

	if (status > 0) {
		printf("The file was successfully created.\n");
	} else if (status < 0) {
		printf("The file already exists.\n");
	} else {
		printf("ERROR: Server response error\n");
	}
}

void RMFL(int sockfd) {
	// Get file name and size
	char* filename = strtok(NULL, " ");

	if (filename == NULL) {
		printf("ERROR: Input error");
		exit(1);
	}

	int received = int_send(strlen(filename), sockfd);

	// Send filename
	char_send(sockfd, filename, strlen(filename));

	// Receive server response
	int status = int_recv(sockfd);

	if (status < 0) {
		printf("The file does not exist on server.\n");
	} else if (status > 0) {
		// Get user confirmation
		printf("Are you sure you would like to delete %s? [Yes/No]\n", filename);

		char confirm[5];
		bzero(confirm, sizeof(confirm));
		fgets(confirm, 5, stdin);

		// Chomp input
		confirm[strlen(confirm)-1] = '\0';
		confirm[4] = '\0';
		confirm[5] = '\0';

		// Send confirmation
		char_send(sockfd, confirm, 5);

		if (!strcmp(confirm, "Yes")) {
			// User wants to delete
			int status = int_recv(sockfd);

			if (status > 0) {
				printf("%s deleted.\n", filename);
			} else if (status < 0) {
				printf("Failed to delete %s\n", filename);
			} else {
				printf("ERROR: Server response error\n");
			}
		} else {
			printf("Delete abandoned by the user!\n");
		}

	} else {
		printf("ERROR: Server response error\n");
	}
}

void LIST(int sockfd){
	char buffer[BUFSIZ];
	char list_contents[BUFSIZ];
	bzero(buffer, sizeof(buffer));
	bzero(list_contents, sizeof(list_contents));

	// Get size of directory
	int size_dir = int_recv(sockfd);

	// Recieve listing
	int bytes = 0;
	while(bytes < size_dir) {
		memset(&buffer, 0, sizeof(buffer));
		bytes += char_recv(sockfd, buffer, sizeof(buffer)-1);
		strcat(list_contents, buffer);
		bzero(buffer, sizeof(buffer));

		// Escape reading loop if its empty or error
		if (bytes <= 0) {
			break;
		}
	}
	bzero(buffer, sizeof(buffer));
	printf("%s", list_contents);
}

void MKDR(int sockfd) {
	// Get directory name and size
	char* dirname = strtok(NULL, " ");

	if (dirname == NULL) {
		printf("ERROR: Input error\n");
		exit(1);
	}

	// Send length of directory name (int)
	int received = int_send(strlen(dirname), sockfd);

	// Send directory name (string)
	char_send(sockfd, dirname, strlen(dirname));

	// Recieve status update and inform user
	int status = int_recv(sockfd);

	if (status == -2) {
		printf("The directory already exists on the server.\n");
	}
	else if (status == -1) {
		printf("Error in making directory.\n");
	}
	else {
		printf("The directory was successfully made.\n");
	}

}

void RMDR(int sockfd){
	// Get directory name and size
	char* dirname = strtok(NULL, " ");

	if (dirname == NULL) {
		printf("ERROR: Input error\n");
		exit(1);
	}

	// Send length of directory name (int)
	int received = int_send(strlen(dirname), sockfd);

	// Send directory name (string)
	char_send(sockfd, dirname, strlen(dirname));

	// Recieve status update and inform user
	int status = int_recv(sockfd);

	if (status == -1) {
		// Directory DNE
		printf("The directory does not exist on server.\n");
	}
	else if (status == -2) {
		// Directory not empty
		printf("The directory is not empty.\n");
	}
	else if (status == 1) {
		// Directory can be deleted

		// Get user confirmation
		printf("Are you sure you would like to delete %s? [Yes/No]\n", dirname);

		char confirm[5];
		bzero(confirm, sizeof(confirm));
		fgets(confirm, 5, stdin);

		// Get rid of the endline character
		confirm[strlen(confirm)-1] = '\0';
		// Ensure last two characters are null
		confirm[4] = '\0';
		confirm[5] = '\0';

		// Send confirmation
		char_send(sockfd, confirm, 5);


		if (!strcmp(confirm, "Yes")) {
			// User wants to delete
			int status = int_recv(sockfd);

			if (status > 0) {
				printf("Directory deleted\n");
			} else if (status < 0) {
				printf("Failed to delete directory\n");
			} else {
				printf("ERROR: Server response error\n");
			}
		} else {
			// User does not want to delete
			printf("Delete abandoned by the user!\n");
		}

		} else {
			printf("ERROR: Server response error\n");
		}
}

void CHDR(int sockfd){
	// Get directory name and size
	char* dirname = strtok(NULL, " ");

	if (dirname == NULL) {
		printf("ERROR: Input error\n");
		exit(1);
	}

	// Send size of dirname
	int received = int_send(strlen(dirname), sockfd);

	// Send directory name
	char_send(sockfd, dirname, strlen(dirname));

	// Recieve server's response
	int status = int_recv(sockfd);

	if (status > 0) {
		printf("Changed current directory\n");
	} else if (status == -1) {
		printf("Error in changing directory\n");
	} else if (status == -2) {
		printf("The directory does not exist on server\n");
	} else {
		printf("Unknown status received for CHDR from server.\n");
	}
}

int char_send(int sockfd, char* buffer, int size) {
	int len;
	if ((len = write(sockfd, buffer, size)) == -1) {
		perror("ERROR: Sending string");
		exit(1);
	}
	return len;
}

int char_recv(int sockfd, char* buffer, int size) {
	int len;
	bzero(buffer, sizeof(buffer));
	if ((len = read(sockfd, buffer, size)) == -1) {
		perror("ERROR: Recieving string");
		exit(1);
	}
	return len;
}

int int_send(int value, int sockfd) {
	int len;
	uint32_t temp = htonl(value);
	if ((len = write(sockfd, &temp, sizeof(temp))) == -1) {
		perror("ERROR: Sending size");
		exit(1);
	}

	return len;
}

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

static long get_time() {
	struct timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec * (int)1e6 + t.tv_usec;
}
