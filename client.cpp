/****************************************************************/
/* Simple client.  argv[1] must be host name.  argv[2] must be  */
/* correct port number.  It won't work otherwise (little or no  */
/* error checking).                                             */
/****************************************************************/
#include <signal.h>
#include <stdio.h>
#include <iostream>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>

using namespace std;

extern int errno;

main (int argc, char *argv[])
{
  //char hname[255];			// FQDN?  I hope.
  char username[30];		        // Username from user input
  struct sockaddr_in sin;               // Socket address.
  struct sockaddr_in lin;               // local socket address.
  socklen_t sinlength;
  socklen_t linlength;
  struct hostent *ph;			// Host entry.
  int thesock;				// TCP socket.
  int len;				// Length.
  long address;				// IP address.
  char host[40];			// Remote host.
  char strinput[1024];			// Generic keyboard input
  char strinput2[1024];			// copy of generic keyboard input
  char buff[1024];			// Buffer for receiving 
  struct sockaddr_in to_addr;		// Host to which I send
  struct sockaddr_in from;		// Host from which I receive
  socklen_t fromlen;			// Length of sockaddr struct
  int pid;				// Used to fork a child
  ssize_t n;				// Used to check recvfrom
  char tempuser[30];                    // Used to store username temp
  
  //Initialize buffers
  bzero(username, 30);
  bzero(strinput, 1024);
  bzero(strinput2, 1024);
  bzero(buff, 1024);
  bzero(tempuser, 30);

  // Find Internet address of remote host.
  if (argc != 3){
	printf("Usage: ./client regal.csep.umflint.edu 3022\n");
	exit(1);
  }
  else strcpy(host, argv[1]);

  if ((ph = gethostbyname (host)) == NULL) {
    switch (h_errno) {
      case HOST_NOT_FOUND:
        fprintf(stderr, "%s:  no such host %s\n", argv[0], host);
        exit(1);
      case TRY_AGAIN:
        fprintf(stderr, "%s:  host %s, try again later\n", argv[0], host);
        exit(1);
      case NO_RECOVERY:
        fprintf(stderr, "%s:  host %s DNS ERROR\n", argv[0], host);
        exit(1);
      case NO_ADDRESS:
        fprintf(stderr, "%s:  No IP address for %s\n", argv[0], host);
        exit(1);
      default:
        fprintf(stderr, "Unknown error: %d\n", h_errno);
        exit(1);
    }
  }
  else {
    memcpy ((char *) &sin.sin_addr, (char *) ph -> h_addr_list[0], ph -> h_length);
    sin.sin_family = PF_INET;
  }

  // Get port number of IP server
  sin.sin_port = atoi(argv[2]);

  // Open a socket
  if ((thesock = socket (PF_INET, SOCK_DGRAM, 0)) < 0) {
    perror ("socket");
    exit(1);
  }

  //Initialize IP address
  lin.sin_family = PF_INET;
  lin.sin_addr.s_addr = INADDR_ANY;
  lin.sin_port = 0;
  linlength = sizeof(lin);
  
  // Bind an IP address to the socket
  if (bind(thesock, (struct sockaddr *) &lin, linlength) < 0) {
    perror ("bind");
    exit(1);
  }

  //Returns the currents address of the socket
  if (getsockname(thesock, (struct sockaddr *) &lin, (socklen_t *) &linlength) < 0)
  {
    perror("getsockname");
    exit(1);
  }

  printf("\n");
  cout << "Local socket is using port number " << lin.sin_port << endl;

  // Ask the user for their username, then send it to the server.
  printf("Please enter in your desired username: ");
  fgets(tempuser, 30, stdin); 
  strcpy(username, "1gh4f "); //Some garbage to use as command for first connection
  strcat(username, tempuser); // Tack the username onto the end of the garbage

  //len = sizeof(hname);
  sinlength = sizeof(struct sockaddr_in);
  
  if(sendto(thesock, username, strlen(username), 0, (struct sockaddr *) &sin, sinlength) <= 0){
    puts("Error sending");
  }
  else printf("Username was sent to the server.");

  //Print out options for the application
  printf("\n");
  printf("========================================================================================\n");
  printf(" There are two special commands that this application supports.\n");
  printf(" .send <username> = allows for a private message to be sent to the specified username.\n");
  printf(" Please be sure not to include the <> when using the command above.\n");
  printf(" .quit = quit the program. \n");
  printf("========================================================================================\n");

  pid = fork(); //Fork outside while loop so we aren't contiually creating/killing processes

  //Fork a child
  while(1){
    //Child process used for printing and receiving   
    if (pid  == 0){
        n = recvfrom(thesock, buff, 1024, 0, (struct sockaddr *) &from, &fromlen);
        if (n < 0 ) puts("Error receiving.");
	buff[n] = '\0';
        
        //Terminate if we receive '.quit' ack from server   
	if (strstr(buff, ".quit")){
          printf("The client is now closing. \n");
          close(thesock);
          exit(0);
        }
        //Print out contents of server reply
        else {
          printf("\n");
	  printf("%s", buff);
	  printf("\n");
       }
    }
    
    //Parent process used for sending
    else {
	//Read in input from user
	char *strinput2;
	fgets(strinput, 1024, stdin);
	
	//Send quit command and kill parent; also prevents invalid command input	
	if (strinput2 = strstr (strinput, ".quit")){     
  	  if (sendto(thesock, strinput2, strlen(strinput2), 0, (struct sockaddr *) &sin, sinlength) <= 0){
	    puts("Error while sending");
	  }
	  else kill(getpid(), SIGKILL); //Kill parent process
	}
	 //Let server handle .send command, send all messages aside from '.quit'
        else {
	  if (sendto(thesock, strinput, strlen(strinput), 0, (struct sockaddr *) &sin, sinlength) <= 0){
	    puts("Error while sending");
	  }
	  //else printf("\nMessage sent\n");
        }	
    }//end of parent process
  }//end of entire while loop
 }//end of main loop
