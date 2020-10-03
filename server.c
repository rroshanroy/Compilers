#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "packet.h"
#include "buffer.h"

#define CHANNELS 2 // Number of channels to transfer data from client to server
#define PER 0.1 // Probability of packet loss at server (artificial)

// Function: Exit with error message
void die(char * s) {
	perror(s);
	exit(1);
}

int main(void) {

	// Create the listening socket to search for incoming TCP connections. Bind server address to the socket
	int opt = true;
	int listen_fd;
	if((listen_fd = socket(AF_INET , SOCK_STREAM , 0)) == 0) {   
        perror("socket failed");   
        exit(EXIT_FAILURE);   
    }
	if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {   
        perror("setsockopt");   
        exit(EXIT_FAILURE);   
    }
    printf("Successfully retrieved the server socket. Reusable port number\n");
	

    // Bind the listening socket to localhost port 8882  
	struct sockaddr_in serv_addr, cli_addr;
	int addrlen = sizeof(serv_addr);
	memset(&serv_addr, '0', sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);

    if (bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0) {   
        perror("bind failed");   
        exit(EXIT_FAILURE);   
    }   
    printf("Listener on port %d \n", PORT);


    // Actively start listening for a TCP connection
    if (listen(listen_fd, 2) < 0)  
        die("listen");     
    
    puts("Waiting for connections ...");

    
    // Create 2 channel connections once TCP handshake is established
	int channel1_fd, channel2_fd;
    channel1_fd = accept(listen_fd, (struct sockaddr*) &cli_addr, (socklen_t *)&addrlen);
    channel2_fd = accept(listen_fd, (struct sockaddr*) &cli_addr, (socklen_t *)&addrlen);

    printf("Connected to client.\n");

    
    // Create a linked list Buffer to store packets of data. This helps to store out-of-order packets
    linkedlist* buffer = createList();

	fd_set readfds;
	
	// Start receiving DATA_PKTs from the client. Acknowledge with ACK DATA_PKTs
	int max_sd, activity;
	DATA_PKT packet1, packet2, ack1, ack2;
	int DROP=0, STOP=0, END=20;
	int recv_len1, recv_len2;
	int state1=0, state2=0;
	
	max_sd = (channel1_fd>channel2_fd) ? channel1_fd:channel2_fd;

    while(END) {
    	printf("\nIter->\n");
    	FD_ZERO(&readfds);
    	FD_SET(channel1_fd, &readfds);
    	FD_SET(channel2_fd, &readfds);
    	
    	/* Select() serves as a blocking receive without a timeout. Both server channels are waiting for incoming data
    		from the client */

    	activity = select(max_sd + 1, &readfds , NULL , NULL , NULL);
    	
    	// Error in select()
    	if ((activity < 0) && (errno!=EINTR))    
            die("select error");
		
		// CHANNEL 1 active
		if (FD_ISSET(channel1_fd, &readfds)) {		
			
			// Receive DATA_PKT from the client
			printf("Stop: %d\n", STOP);
			if (((recv_len1 = recv(channel1_fd, &packet1, sizeof(DATA_PKT), 0)) == -1))
				die("recvfrom()");

			// Simulating packet loss at the server
			if (rand() % 100 > 100 * (1 - PER))
				DROP = 1; // data packet lost (simulation)
			else
				DROP = 0; //data packet received (simulation)

			// Data packet is received
			if (!DROP) {
				printf("RCVD PKT: Seq No %d from Channel %d\n", packet1.seq_no, packet1.channel_id);
				//printf("Data: %s\n", packet1.data);
				
				// Constructing the ACK_PKT
				ack1.size = PACKET_SIZE;
				ack1.channel_id = 1;
				ack1.seq_no = packet1.seq_no;		// Sequence number is the same as that of incoming DATA_PKT
				ack1.last_pkt = packet1.last_pkt; 
				ack1.ack = true;

				// Flag the last ACK_PKT 
				if(ack2.last_pkt == true) {
					STOP = 1;
					printf("LMFAO\n");
				}

				// Send ACK to acknowledge the receipt of DATA_PKT
				if (send(channel1_fd, &ack1, sizeof(ack1), 0) == -1)
					die("sendto()");
				printf("SENT ACK: Seq No %d from Channel %d\n", ack1.seq_no, ack1.channel_id);

				// Add DATA_PKT received from client to buffer
				insertPacketdata(buffer, &packet1);
			}
		}

/*		if(STOP) {
			if(END == 20)
				END = 10;
		}
*/
		// CHANNEL 2 active
		if (FD_ISSET(channel2_fd, &readfds)) {
			
			// Receive DATA_PKT from the client
			printf("Stop: %d\n", STOP);
			if (((recv_len2 = recv(channel2_fd, &packet2, sizeof(DATA_PKT), 0)) == -1))
				die("recvfrom()");

			// Simulating packet loss at the server
			if (rand() % 100 > 100 * (1 - PER))
				DROP = 1; // DATA_PKT lost (simulation)
			else
				DROP = 0; // DATA_PKT received (simulation)

			//DATA_PKT received
			if (!DROP) {
				printf("RCVD PKT: Seq No %d from Channel %d\n", packet2.seq_no, packet2.channel_id);
				//printf("Data: %s\n", packet2.data);
				
				// Constructing the ACK_PKT
				ack2.size = PACKET_SIZE;
				ack2.channel_id = 2;
				ack2.seq_no = packet2.seq_no; // Sequence number is the same as that of incoming DATA_PKT
				ack2.last_pkt = packet2.last_pkt;
				ack2.ack = true;

				// Flag the last ACK_PKT 
				if(ack2.last_pkt == true) {
					STOP = 1;
					printf("LMFAO\n");
				}

				// Send ACK to acknowledge the receipt of DATA_PKT
				if (send(channel2_fd, &ack2, sizeof(ack2), 0) == -1)
					die("sendto()");
				printf("SENT ACK: Seq No %d from Channel %d\n", ack2.seq_no, ack2.channel_id);

				// Add DATA_PKT received from client to buffer
				printf("Adding PKT data to Buffer\n");
				insertPacketdata(buffer, &packet2);
			}
		}

		if(STOP){
			if(END == 20)
				END = 5;
			END--;
			printf("%d\n", END);
		}
	}

	// Open the output file and print the content of the server Buffer to the file
	FILE *fp = fopen("transferred_file.txt","w");
    if(fp==NULL) {
        printf("File open error");
        return 1;   
   	}
   	printf("\n\nPreparing file!\n");
   	printBuffertoFile(buffer, fp);

  	//TESTING BLOCK 	
	node* temp = buffer->head;
   	while(temp != NULL) {
   		printf("%d\n", temp->offset);
   		temp = temp->right;
   	}


    close(channel1_fd);
    close(channel2_fd);
    close(listen_fd);

    return 0;
}