Compiler Construction Project | 2019-20 Spring Semester | CS F363 

1) Files Submitted are: 

   - server.c, client.c 
   - Server executable, Client executable 
   - buffer.h (for memory buffer implementation), 
   - packet.h (for packet structure definition) 
   - sample input.txt

2) Running instructions:

   1. compile server: gcc server.c -o Server;
   2. compile client: gcc client.c -o Client;

   Open two terminals
   1. execute server: ./Server
   2. execute client: ./Client

   The input file MUST be named 'input.txt' and present in the same directory.
   There must be NO INSTANCE of an output file named output.txt, please delete all instances of the same before running.
   An output file 'transferred_file.txt' will be generated.

3) Implementation Details

   1. After running the code, if NO OUTPUT FILE is detected, please close both terminals and relaunch the two programs. 

   2. The buffer is implemented as a linked list. Every time the server acknowledges a packet, it is  placed into the server Buffer. Duplicate packets do not enter the buffer and the packets are arranged in increasing order of seq_no.

   3. A pseudo double timer has been implemented. Use select() to maintain two concurrently running channels from client to server. Select() must act like a blocking receive for both channels, but for only one at a time. Choose which channel to block based on the last send times for each channel, and determine how much Select() should wait before allowing timeout.
