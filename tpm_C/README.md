# How to Run the Models

1. Open two separate terminal windows—one for the server and one for the client.

2. Compile the programs using the following commands:

   ```bash
   gcc server.c -o server
   gcc client.c -o client

3. On the server terminal, start the server by running:

   ./server 4000

4. On the client terminal, start the client by running:

   ./client

   Then enter the server's IP address and the port number (e.g., 4000) when prompted.

5. The synchronization process between the server and client will begin automatically.
