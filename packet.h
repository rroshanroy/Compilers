#ifndef PACKET_H
#define PACKET_H

#include <stdbool.h>


#define PACKET_SIZE 100  // Size of payload for each packet
#define PORT 8882   // Server port

// Packet structure for both DATA and ACK
typedef struct packet {
    int channel_id;			// 0 or 1
    int size;				
    int seq_no; 			// 0-indexed
    bool last_pkt;
    bool ack;				// Is ACK or DATA?
    char data[PACKET_SIZE]; // Payload
}DATA_PKT;

#endif
