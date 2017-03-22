#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>

using namespace std;

extern int errno;


main (int argc, char *argv[])
{
  int p = 0; 				// Counter variable
  int z = 0; 				// Counter variable
  int s;                                // Network socket.
  int len;                              // Length.
  struct sockaddr_in sin;               // Socket address.
  int sinlength;                        // length of above.
  char recvbuff[1024];                  // data buffer.
  struct sockaddr_in from;              // For the client that sends to me
  socklen_t fromlen;                    // For the client that sends to me
  struct host{                          // Struct for client address
    unsigned short int port;            // Port
    struct in_addr addr;                // Internet(IP) address.
    char name[20];                      // Username
    int empty;				// Flag
   };
 
  struct host clients[3];               // Array to  store clients
  ssize_t n;                            // Used to check recvfrom()
  char *message = NULL, *command = NULL;// Used for parsing messages from clients
  int messagelen;                       // Used to store message length
  char *privateName = NULL;             // Holds name for private message                                                                  
  char finalmessage[1024]; 		// Final message sent to clients
  char *reply = ".quit";                // Reply send for .quit
  char quitmess[50];			// Quit message sent to all users 
  char joined[50]; 			//Joined message for all users

  // Create the socket
  if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    perror ("socket");
    exit(1);
  }

  //Initialize socket address
  sin.sin_family = PF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = 0;
  sinlength = sizeof(sin);

  // Bind address to local end of socket
  if (bind(s, (struct sockaddr *) &sin, sinlength) < 0) {
    perror ("bind");
    exit(1);
  }

  if (getsockname(s, (struct sockaddr *) &sin, (socklen_t *) &sinlength) < 0){
    perror("getsockname");
    exit(1);
  }

  printf("Socket is using port: %d\n", sin.sin_port);
  printf("The server is now running... \n");

  //Zero out buffers and intialize fromlen
  bzero(recvbuff, 1024);
  bzero(finalmessage, 1024);
  bzero(quitmess, 50);
  bzero(joined, 50);
  fromlen = sizeof(struct sockaddr_in);
   
  //Initialize array used to store clients
  for (int p = 0; p < 3; p++){
    clients[p].empty = 1;
  }

  //Infinite loop to handle client requests
  while(1){
    //Initialize variables
    int send = 0, entrycode = 0, z = 0, found=0;

    //Receive message from clients
    n = recvfrom(s, recvbuff, sizeof(recvbuff), 0, (sockaddr *) &from, &fromlen);
    if (n < 0) puts("Error Receiving");
    recvbuff[n] = '\0';
    message = strdup(recvbuff); 
    bzero(recvbuff, sizeof(recvbuff));

    //Attempt to find existing user in array
    while (clients[z].port != from.sin_port && z < 3){
      z++; 
    }
 
    //Parse command from message pending a command is sent from client
    if(strstr(message, ".send")) send = 1;
    if(strstr(message, "1gh4f")) entrycode = 1;
    
    if (strstr(message, ".quit") || send == 1 || entrycode == 1){
      recvbuff[n] = '\0';
      command = strtok_r(message, " ", &message);
      //Parse username if send/intialization is given
      if (send == 1 || entrycode == 1){
        privateName = strtok_r(message, " ", &message); 
      }
      messagelen = strlen(message);
      message[messagelen-1]='\0'; //Remove '\n' from fgets()--> client
      
      //Acknowledge client .quit request
      if (strncmp(command, ".quit", 5) == 0){
	if (z < 3){
	  printf("Removal of %s successful.", clients[z].name);
	  clients[z].empty=1; // Set flag
          //Send ack to client
	  if (sendto(s, reply, strlen(reply), 0, (struct sockaddr *) &from, fromlen) < 0){
	    puts ("Error replying to the client .quit command.");
	    cout << "Errno was: " << errno << endl;
	  }
        
        //Assemble quit message
	strcpy(quitmess, clients[z].name);
	strcat(quitmess, " has left the chatroom"); 
	printf("\n%s\n",quitmess);
	
	//Send message to all clients that user left 
	for (p=0; p < 3; p++){ 
	  if (!clients[p].empty){
	    from.sin_port = clients[p].port;
	    from.sin_addr = clients[p].addr;
	    if (sendto(s, quitmess, 50, 0, (struct sockaddr *) &from, fromlen) < 0){
	      printf("Error sending to %s\n", clients[p].name);
	    }
	  }
	}
      }
      else printf("Username does not exist\n"); 
     }

     //Sends private message to the requested user
     else if (strncmp(command, ".send", 5) == 0){
	for (p = 0; p < 3; p++){
	  if (!strcmp(privateName, clients[p].name)){
	    found = 1;
	    from.sin_port = clients[p].port;
	    from.sin_addr = clients[p].addr;
	    strcpy(finalmessage, "Private message from "); 
	    strcat(finalmessage, clients[z].name);
	    strcat(finalmessage, ": ");
	    strcat(finalmessage, message);
	    if (sendto(s, finalmessage,1024 , 0 , (struct sockaddr *) &from, fromlen) < 0){
	      printf("\nError sending message");
	    }
	  }
      	}
	if (!found){ //If .send did not find that username, send message to sender
	strcpy(finalmessage, "Error: That username does not exist");
	if (sendto (s,finalmessage,1024,0,(struct sockaddr *) &from, fromlen) <0){
	printf("\n Error sending message");	}
	}
      }
      
      //Handles initial username request
      else if(strncmp(command, "1gh4f", 5) == 0){
        // Also check if username already exists within host client already
        printf("A username request for %s has been made.\n", privateName);

	//Find empty index within client array
	z=0;
	while (!clients[z].empty && z < 3){
	   z++;
	}
	
	if (clients[z].empty){
	  //Add client to array
	  strcpy(clients[z].name, privateName); //Add username
	  clients[z].port = from.sin_port; //Add port number
	  clients[z].addr = from.sin_addr; //Add address
	  clients[z].empty = 0; //Set flag
	  printf("Username: %s created properly\n",privateName); //Server side message
	
          //Assemble join message
	  strcpy(joined, clients[z].name); 
	  strcat(joined, " has joined the chatroom.");
	  printf("\n%s\n", joined); 

          //Send joined message to all connected clients
	  for (p = 0; p < 3; p++){ 
	    if (!clients[p].empty){ 
	      from.sin_port = clients[p].port;
	      from.sin_addr = clients[p].addr;
	      if (sendto(s, joined, 50, 0, (struct sockaddr *) &from, fromlen) < 0){
	        printf("\nError sending message");
	      }
	    }
	  }
	}
	else{ //Terminate new client if all spaces are used on server
	  printf("\nError! No more room in the client array!"); 
	  if (sendto(s, reply, strlen(reply), 0,(struct sockaddr *) &from, fromlen) < 0){
	    printf("\nError sending .quit command to client.");
	  }
	}
      }
    }
    //Echo messages to all connected clients
    else {
      //Assemble final message
      strcat(finalmessage, clients[z].name);
      strcat(finalmessage, " said: ");
      strcat(finalmessage, message);

      for (p = 0; p < 3; p++){
        if (!clients[p].empty){
	  from.sin_port = clients[p].port;
	  from.sin_addr = clients[p].addr; 
	  if (sendto(s, finalmessage, strlen(finalmessage), 0, (struct sockaddr *) &from, fromlen) < 0){
	    printf("\nError sending message");
	  }
	} 
      }
   }//else statement
   //Deallocate/Cleanup 
   //delete command, message, privateName;
   bzero(finalmessage, 1024);
   bzero(quitmess, 50);
   bzero(joined, 50); 
 }//while loop
 delete reply, command, message, privateName;
}//main loop


                                                                                                                 
