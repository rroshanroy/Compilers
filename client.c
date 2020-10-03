#include <stdio.h> //printf
#include <string.h> //memset
#include <stdlib.h> //exit(0);
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h> // close() function
#include <signal.h>
#include <stdbool.h>
#include <sys/time.h>
#include <limits.h>

#include "packet.h"
#define TIMEOUT_SEC 2

// Function: Exit with error message
void die(char * s) {
	perror(s);
	exit(1);
}

// Function: Read a packet of PACKET_SIZE length from the input file
bool readPacketfromFile(FILE* fp, char* data, int size) {
    int nread = fread(data,1,PACKET_SIZE,fp);
    bool last = false;

    // If nread = PACKET_SIZE then the data is inserted into the array
    
    if (nread < PACKET_SIZE) {
        // Reached EOF
        if (feof(fp))
            last = true;
            int i=nread;
            while(i!=PACKET_SIZE) {
                data[i]='\0';
                i++;
            }
           // Error occurred
        if (ferror(fp))
            exit(1);
            last = true;
    }
    return last;
}

// Function: Return the older timevalue structure 
struct timeval older(struct timeval t1, struct timeval t2) {
	if(t1.tv_sec > t2.tv_sec)
		return t2;
	
	else if(t1.tv_sec < t2.tv_sec)
		return t1;
	
	else
	{
		if(t1.tv_usec > t2.tv_usec)
			return t2;
		else
			return t1;
	}
}

//Function: Check if first timevalue structure is older than the second
bool isOlder(struct timeval t1, struct timeval t2) {
	if(t1.tv_sec > t2.tv_sec)
		return true;
	
	else if(t1.tv_sec < t2.tv_sec)
		return false;
	
	else
	{
		if(t1.tv_usec > t2.tv_usec)
			return true;
		else
			return false;
	}
}

//Function: Calculate the difference between two timevalue structures
struct timeval diff(struct timeval t1, struct timeval t2) {
	struct timeval temp = older(t1, t2);
	if(temp.tv_usec == t1. tv_usec && temp.tv_sec == t1.tv_sec)
	{
		temp.tv_sec -= t2.tv_sec;
		temp.tv_usec -= t2.tv_usec;
		return temp;
	}
	else {
		temp.tv_sec -= t1.tv_sec;
		temp.tv_usec -= t1.tv_usec;
		return temp;
	}
}


int main(void) {

	int channel1_fd, channel2_fd;
	int activity;
	struct sockaddr_in serv_addr;
	DATA_PKT packet1, packet2, ack1, ack2;

	// Create the two channels/sockets to transmit the data to server
	int opt = true;
    if((channel1_fd = socket(AF_INET , SOCK_STREAM , 0)) == 0) {   
        perror("socket failed");   
        exit(EXIT_FAILURE);   
    }
    if(setsockopt(channel1_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {   
        perror("setsockopt");   
        exit(EXIT_FAILURE);   
    }
    if((channel2_fd = socket(AF_INET , SOCK_STREAM , 0)) == 0) {   
        perror("socket failed");   
        exit(EXIT_FAILURE);   
    }
    if(setsockopt(channel2_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {   
        perror("setsockopt");   
        exit(EXIT_FAILURE);   
    }
    printf("Successfully created the client sockets. Reusable port numbers\n");


    // Initialize the target server address structure 
	memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");	
	int addrlen = sizeof(serv_addr);


    // Attempt a connection to the server
    if(connect(channel1_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0) {
        printf("\n Error : Channel 1 Connection Failed \n");
        return 1;
    }
    if(connect(channel2_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0) {
        printf("\n Error : Channel 2 Connection Failed \n");
        return 1;
    }
    printf("Connected to server on port %d \n", PORT);


    // Open the input file
    FILE *fp;     
    fp = fopen("input.txt", "r"); 
    if(fp == NULL) {
      printf("Error opening file");
        return 1;
    }


    // Initialize the concurrency mechanism
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(channel1_fd, &readfds);
    FD_SET(channel2_fd, &readfds);
    int max_sd = (channel1_fd>channel2_fd) ? channel1_fd:channel2_fd;


    // Initialize the TWO timers that keep track of timeout for indivdual channels
    struct timeval select_timer;
    struct timeval c1Time, c2Time;
    struct timeval presentTime, finish, first, timeout;
    struct timeval sendtime1, sendtime2;
    timeout.tv_sec = TIMEOUT_SEC;
	timeout.tv_usec = 0;
	sendtime1.tv_sec = sendtime2.tv_sec = INT_MAX;
    

	// Initialise the DATA_PKTs for each channel. To be sent in first iteration
    bool last_pkt=false;
    int elder_channel = 0;
    int offset=-1;

    last_pkt = readPacketfromFile(fp, packet1.data, PACKET_SIZE);
	packet1.size = PACKET_SIZE;
	packet1.channel_id = 1;
	packet1.seq_no = (++offset)*PACKET_SIZE; // Fix this
	packet1.last_pkt = last_pkt;
	packet1.ack = false;

	last_pkt = readPacketfromFile(fp, packet2.data, PACKET_SIZE);
	packet2.size = PACKET_SIZE;
	packet2.channel_id = 2;
	packet2.seq_no = (++offset)*PACKET_SIZE; // Fix this
	packet2.last_pkt = last_pkt;
	packet2.ack = false;

	
	// Start sending DATA_PKTs from the client to the server. Receive ACKs in response
    int STOP = 0, END = 20;

    while(END) {
    	printf("\nIter->\n");

    	/* Use select() to maintain two concurrently running channels from client to server. Select()
			must act like a blocking receive for both channels, but for only one at a time. Choose which 
			which channel to block based on the last send times for each channel, and dedtermine how much 
			Select() should wait before allowing timeout. */
    	
    	gettimeofday(&presentTime, NULL);
		first = older(sendtime1, sendtime2);
		if(sendtime1.tv_sec == INT_MAX || sendtime2.tv_sec == INT_MAX ||isOlder(first, diff(presentTime, timeout))) {
			finish.tv_sec = 0;
			finish.tv_usec = 0;
		}
		else 
			finish = diff(timeout, diff(presentTime, first));

		// Checking for activity on both channels. Acts a blocking receive for 'finish' timevalue
    	activity = select(max_sd + 1, &readfds , NULL , NULL , &finish);
    	printf("Activity: %d\n", activity);
    
		// Error occurred in Select()    
    	if (activity == -1) 
    		die("Error in select");

    	// Timeout occurred
    	if(activity==0) {
    		// Channel 1 timed out. Retransmit
    		if(elder_channel==0) {
    			if (send(channel1_fd, &packet1, sizeof(packet1), 0) == -1)
					die("sendto channel 1()");
				printf("SENT PKT: Seq No %d of Size %d bytes from Channel %d\n", packet1.seq_no, packet1.size, packet1.channel_id);

				// Set last sent time for Channel 1
				gettimeofday(&c1Time, NULL);
				// Set the older channel (0=CHANNEL 1, 1 = CHANNEL 2)
				elder_channel = 1;
    		}
    		// Channel 2 timed out. Retransmit
    		else {
    			if (send(channel2_fd, &packet2, sizeof(packet2), 0) == -1)
					die("sendto channel 1()");
				printf("SENT PKT: Seq No %d of size %d bytes from Channel %d\n", packet2.seq_no, packet2.size, packet2.channel_id);

				// Set last sent time for Channel 2 (0=CHANNEL 1, 1 = CHANNEL 2)
				gettimeofday(&c2Time, NULL);
				// Set the older channel
				elder_channel = 0;
    		}
    	}

    	// Either Channel 1 or Channel 2 or both of them are active (ACK received)
    	if(activity>0) {
    		//At least one ACK  received
    		if (FD_ISSET(channel1_fd, &readfds)) {
    				// ACK from Channel 1 received
    			if (recv(channel1_fd, &ack1, sizeof(ack1), 0) == -1)
					die("recvfrom()");
				if(ack1.last_pkt == true) {
					STOP = 1;
				}

				printf("RCVD ACK: Seq. No %d from channel %d\n", ack1.seq_no, ack1.channel_id);

				if(last_pkt==false) {
					// Construct and send the DATA_PKT
					last_pkt = readPacketfromFile(fp, packet1.data, PACKET_SIZE);
					packet1.size = PACKET_SIZE;
					packet1.channel_id = 1;
					packet1.seq_no = (++offset)*PACKET_SIZE; // Fix this
					packet1.last_pkt = last_pkt;
					packet1.ack = false;

					if (send(channel1_fd, &packet1, sizeof(packet1), 0) == -1)
						die("sendto channel 1()");
					printf("SENT PKT: Seq No %d of Size %d bytes from Channel %d\n", packet1.seq_no, packet1.size, packet1.channel_id);

					//Update Channel 1's last sent time
					gettimeofday(&c1Time, NULL);
					elder_channel = 1;
				}
		    }

		//Last ACK has been received on Channel 1
/*		if(STOP){
			if(END == 20)
				END = 5;
			END--;
		}
*/			

		    if (FD_ISSET(channel2_fd, &readfds)) {
		    	//ACK from Channel 1 received
    			if (recv(channel2_fd, &ack2, sizeof(ack2), 0) == -1)
					die("recvfrom()");
				if(ack2.last_pkt == true) {
					STOP = 1;
				}

				printf("RCVD ACK: Seq. No %d from channel %d\n", ack2.seq_no, ack2.channel_id);

				// Read next packet from file only if the one just received is not the last one
				if(last_pkt==false) {
					//Contruct and send the DATA_PKT
					last_pkt = readPacketfromFile(fp, packet2.data, PACKET_SIZE);
					packet2.size = PACKET_SIZE;
					packet2.channel_id = 2;
					packet2.seq_no = (++offset)*PACKET_SIZE; // Fix this
					packet2.last_pkt = last_pkt;
					packet2.ack = false;

					if (send(channel2_fd, &packet2, sizeof(packet2), 0) == -1)
						die("sendto channel 1()");
					printf("SENT PKT: Seq No %d of size %d bytes from Channel %d\n", packet2.seq_no, packet2.size, packet2.channel_id);

					//Update Channel 2's last sent time
					gettimeofday(&c2Time, NULL);
					elder_channel = 0;
				}
	        }

	    //Last ACK has been received on Channel 2
	    if(STOP){
//			if(END == 20)
//				END = 10;
//			END--;
//			printf("%d\n", END);
//	    	END--;
	    	break;
		}

/*		if(STOP)
			break;
*/		}
		
		// Reset the concurrency maintenance mechanism
		FD_ZERO(&readfds);
		FD_SET(channel1_fd, &readfds);
		FD_SET(channel2_fd, &readfds);
	}
	/* Sleep for 1s to allow server to execute code fully. Otherwise connection is lost and both processes 
		stop execution */
	sleep(1);
	close(channel1_fd);
    close(channel2_fd);

    return 0;
}