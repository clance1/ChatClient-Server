/*  server.c
    Cole Pickford
    Carson Lance
    Jack Conway
*/

#include<stdio.h>
#include<string.h>    //strlen
#include<stdlib.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
#include<pthread.h> //for threading , link with lpthread
#include<stdbool.h>
#include<time.h>

//the thread function
void *connection_handler(void *);
int char_send(int, char*, int);
int char_recv(int, char*, int);
int int_send(int, int);
int int_recv(int);
bool username_checker(char*, int);
bool password_checker(char*, char*);
void write_history(char*, char*, char*);

// IDEA
// Make a struct that contains the username and thread that will be passed with functions
// This will make thread tracking easier. I think


int main(int argc , char *argv[]){
    int socket_desc , client_sock , c;
    struct sockaddr_in server , client;
    int PORT = atoi(argv[1]);

    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( PORT );

    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");

    //Listen
    listen(socket_desc , 3);

    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
	  pthread_t thread_id;

    //While loop will accept multiple clients
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) ){
        puts("Connection accepted");

        //creates thread
        if(pthread_create( &thread_id , NULL ,  connection_handler , (void*) &client_sock) < 0){
            perror("could not create thread");
            return 1;
        }
        printf("Thread ID: %d\n", thread_id);
        puts("Handler assigned");
    }

    //handle failed accept
    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }

    return 0;
}


//This will handle connection for each client
void *connection_handler(void *socket_desc) {
    bool usercheck = false;
    bool passcheck = false;
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    int read_size;
    char *message , client_message[2000];

    //Send some messages to the client
    message = "Greetings! I am your connection handler\n";
    write(sock , message , strlen(message));

    char username[BUFSIZ];
    //while loop handles username checking
    while (!usercheck){

        //ask for username
        message = "Please enter your username: \n";
        write(sock , message , strlen(message));

        //receive username
        memset(username, 0, sizeof(username));
	      int username_received = char_recv(sock, username, sizeof(username));
        printf("Username: %s", username);
        //username checker
        usercheck = username_checker(username, sock);
    }

    printf("Exited Username loop\n");
    //while loop checking password
    while(!passcheck){
        //ask for password
        message = "Please enter your password: \n";
        write(sock , message , strlen(message));

        //receive password
        char password[BUFSIZ];
        memset(password, 0, sizeof(password));
	      int password_received = char_recv(sock, password, sizeof(password));
        printf("Username: %s\n", username);
        printf("Password: %s", password);

        //password checker
        passcheck = password_checker(username, password);
        printf("Password Check complete\n");
    }

    message = "Login Successful\n";
    write(sock, message, strlen(message));

    //Receive a message from client
    while((read_size = recv(sock , client_message , 2000 , 0)) > 0 ) {
        //end of string marker
        if (client_message[sizeof(client_message)-1] == '\n')
		        client_message[read_size] = '\0';
        printf("Message received: %s\n", client_message);
        // Check clients function
        if (!strcmp(client_message, "B")) {
            //ask for message
            message = "Please enter your message: \n";
            write(sock , message , strlen(message));

            // Receive message and write to history
            char broadcast_message[BUFSIZ];
            int broadcast_recv = char_recv(sock, broadcast_message, sizeof(broadcast_message));
            write_history(username, broadcast_message, client_message);
        }
        else if (!strcmp(client_message, "P")) {

        }
        else if (!strcmp(client_message, "H")) {

        }
        else if (!strcmp(client_message, "X")) {

        }
        else {

        }
    }

    return 0;
}

//password checking function
bool username_checker(char* username, int sockfd){
    int status_send;
    //file can be every other line username and password checking until end of file
    FILE *fp = fopen("passwords.txt", "a");
    //check file existence
    fseek(fp, 0L, SEEK_END);
    int sz = ftell(fp);
    if(sz == 0){
        //write file title
        fprintf(fp, "%s\n", "Username then Password on next line");
    }
    //close file
    fclose(fp);
    //file lines
    char user[50];
    int count = 0;
    bool new_user = false;
    fp = fopen("passwords.txt", "r+");
    //loop through file and check for preexisting usernames
    while(fgets(user, sizeof(user), fp) != NULL){
        // Strip newline char off recevied username
        size_t ln = strlen(username) - 1;
        if (username[ln] == '\n')
    		  username[ln] = '\0';
        // Strip newline char off fetched username
        size_t l = strlen(user) - 1;
        if (user[l] == '\n')
    		  user[l] = '\0';
        //increment count
        count++;
        //check if username exists in right position
        if(!strcmp(user, username)){
            puts("Existing user");
            int t = 0;
            status_send = int_send(t, sockfd);
            fclose(fp);
            return true;
        }
    }

    printf("Creating new user: %s\n", username);
    int t = 1;
    status_send = int_send(t, sockfd);
    char* message;

    message = "Created a new user\nPlease enter your password: \n";
    write(sockfd, message , strlen(message));
    //receive password
    char pass[BUFSIZ];
    int password_received = char_recv(sockfd, pass, sizeof(pass));
    printf("Received password\n");

    //add new username
    printf("Creating new user: %s\n", username);
    fprintf(fp, "%s\n", username);
    fprintf(fp, "%s", pass);

    char hist_file[BUFSIZ];
    strcpy(hist_file, username);

    // Create history file
    char file_ending[BUFSIZ] = ".chat";
    strcat(hist_file, file_ending);
    FILE *hist_fp = fopen(hist_file, "w+");
    printf("Created history\n");
    fclose(hist_fp);
    fclose(fp);
    return true;
}

//password checking function
bool password_checker(char* username, char* password){
    //file can be every other line username and password checking until end of file
    FILE *fip = fopen("passwords.txt", "r");
    //file lines
    char user[50];
    char pass[50];
    //loop through file and check for preexisting usernames
    while(fgets(user, sizeof(user), fip)){
        //check if username exists
        size_t ln = strlen(user) - 1;
        if (user[ln] == '\n')
    		  user[ln] = '\0';
        if(!strcmp(user, username)){
            printf("Found user\n");
            //check next line for password
            fgets(pass, sizeof(pass), fip);
            // Strips newline char from both
            size_t l = strlen(pass) - 1;
            if (pass[l] == '\n')
        		  pass[l] = '\0';
            size_t lp = strlen(password) - 1;
            if (password[lp] == '\n')
          		 password[lp] = '\0';
            printf("Password found: %s\n", pass);
            printf("Password comparing: %s\n", password);
            if(!strcmp(pass, password)){
                //username and password already exist
                printf("Passwords match\n");
                return true;
            }
            else {
              printf("Passwords do not match\n");
              return false;
            }
        }
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

void write_history(char* username, char* message, char* action) {
  char hist_file[BUFSIZ];
  strcpy(hist_file, username);
  char space[BUFSIZ] = " ";
  char mess[BUFSIZ] = "Message: \"";
  char quote[BUFSIZ] = "\"";

  // Find history file
  char file_ending[BUFSIZ] = ".chat";
  strcat(hist_file, file_ending);

  printf("Opening %s for writing\n", hist_file);
  FILE *hist_fp = fopen(hist_file, "w");

  // Get time
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  char date[BUFSIZ];
  sprintf(date, "Date: %d-%d-%d %d:%d ", tm.tm_year + 1900, tm.tm_mon + 1,tm.tm_mday, tm.tm_hour, tm.tm_min);

  char hist_entry[BUFSIZ];
  strcat(hist_entry, date);

  size_t lp = strlen(message) - 1;
  if (message[lp] == '\n')
     message[lp] = '\0';

  if (!strcmp(action, "B")) {
    char action_full[BUFSIZ] = "Action: Broadcast User: ";
    strcat(hist_entry, action_full);
    strcat(hist_entry, username);
    strcat(hist_entry, space);
    strcat(hist_entry, mess);
    strcat(hist_entry, message);
    strcat(hist_entry, quote);
  }

  printf("hist_entry: %s\n", hist_entry);
  fprintf(hist_fp, "%s\n", hist_entry);
  printf("Wrote to hist file\n");
  fclose(hist_fp);
}
