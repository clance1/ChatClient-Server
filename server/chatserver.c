/*  server.c
    Cole Pickford
*/

#include<stdio.h>
#include<string.h>    //strlen
#include<stdlib.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
#include<pthread.h> //for threading , link with lpthread
#include<stdbool.h>

//the thread function
void *connection_handler(void *);
int char_send(int, char*, int);
int char_recv(int, char*, int);
int int_send(int, int);
int int_recv(int);
bool username_checker(char*);
bool password_checker(char*, char*);


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


    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
	pthread_t thread_id;

    //While loop will accept multiple clients
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) ){
        puts("Connection accepted");

        //creates thread
        if( pthread_create( &thread_id , NULL ,  connection_handler , (void*) &client_sock) < 0)
        {
            perror("could not create thread");
            return 1;
        }

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
void *connection_handler(void *socket_desc)
{
    bool usercheck = false;
    bool passcheck = false;
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    int read_size;
    char *message , client_message[2000];

    //Send some messages to the client
    message = "Greetings! I am your connection handler\n";
    write(sock , message , strlen(message));

    //while loop handles username checking
    while (!usercheck){

        //ask for username
        message = "Please enter your username: \n";
        write(sock , message , strlen(message));

        //receive username
        char username[BUFSIZ];
        memset(username, 0, sizeof(username));
	      int username_received = char_recv(sock, username, sizeof(username));

        printf("Username:%s.\n", username);

        //username checker
        usercheck = username_checker(username);
    }

    /*
    //while loop checking password
    while(!passcheck){


        //ask for password
        message = "Please enter your password: \n";
        write(sock , message , strlen(message));

        //receive password
        char password[BUFSIZ];
        memset(password, 0, sizeof(password));
	    int password_received = char_recv(sock, password, sizeof(password));
        password[password_received] = '\0';
        printf("%s", password);

        //password checker
        passcheck = password_checker(username, password);

    }
    */

    /*
    //Receive a message from client
    while( (read_size = recv(sock , client_message , 2000 , 0)) > 0 )
    {
        //end of string marker
		client_message[read_size] = '\0';

		//Send the message back to client
        //write(sock , client_message , strlen(client_message));

		//clear the message buffer
		memset(client_message, 0, 2000);
    }

    if(read_size == 0)
    {
        puts("Client disconnected");
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("recv failed");
    }
     */
    return 0;
}

//password checking function
bool username_checker(char* username){
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
    fp = fopen("passwords.txt", "r");
    //loop through file and check for preexisting usernames
    while(fgets(user, sizeof(user), fp) != NULL){
        puts("checking username");
        size_t ln = strlen(username) - 1;
        if (username[ln] == '\n')
    		  username[ln] = '\0';
        printf("Sent name:%s.\n",username);
        size_t l = strlen(user) - 1;
        if (user[l] == '\n')
    		  user[l] = '\0';
        printf("Fetched name:%s.\n",user);
        //increment count
        count++;
        //check if username exists in right position
        if(!strcmp(user, username)){
            puts("existing user");
            return true;
        }
    }
    fclose(fp);

    //add new username
    printf("Creating new user: %s\n", username);
    fp = fopen("passwords.txt", "a");
    fprintf(fp, "%s\n", username);
    fclose(fp);

    return true;
}

//password checking function
bool password_checker(char* username, char* password){
    //file can be every other line username and password checking until end of file
    FILE *fp = fopen("passwords.txt", "a");
    //file lines
    char user[50];
    char pass[50];
    //loop through file and check for preexisting usernames
    while(fgets(user, sizeof(user), fp)){
        //check if username exists
        if(user == username){
            //check next line for password
            fgets(pass, sizeof(pass), fp);
            if(pass == password){
                //username and password already exist
                return true;
            }
        }
        else{
            //add new username and password
            fprintf(fp, "%s\n", username);
            fprintf(fp, "%s\n", password);

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
