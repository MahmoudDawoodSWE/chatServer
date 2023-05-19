# Event-Driven Chat Server

## Description

The event-driven chat server is a program that allows multiple clients to connect and communicate with each other in a chat-like environment. The server utilizes an event-driven architecture to handle incoming connections and messages in an efficient and scalable manner.

The main function of the chat server is to forward each incoming message to all connected clients, excluding the client that sent the message. This behavior is implemented entirely in an event-driven manner, without the use of threads. The server uses the `select` function to monitor multiple sockets for activity and efficiently manage incoming and outgoing data.

Each client connection is represented by a separate socket, and the server maintains a queue for each connection to hold the messages that need to be sent to that particular client. When a client sends a message, the server adds it to the appropriate queues of all other connected clients, ensuring that the message is delivered to everyone except the sender.

The server provides console output to provide information about its status, including the acceptance of new connections, reading and writing data from and to sockets, and closure of client connections. Error handling mechanisms are in place to handle any failures and ensure smooth operation.

## Background

The event-driven chat server is designed to facilitate real-time communication between multiple clients. Its main function is to forward incoming messages to all connected clients, excluding the sender. The server achieves this behavior through an event-driven approach, eliminating the need for threads.

The server utilizes the select function to determine which socket descriptors are ready for reading or writing. This function is called within a loop, but it appears only once in the server implementation.

When checking the read set, the server distinguishes the main socket, responsible for accepting new connections, from other sockets that handle data negotiation with clients. If the main socket is ready for reading, the server calls the accept function. For other sockets, it uses the read or recv functions to process incoming data.

To manage outgoing messages, the server maintains a message queue for each client connection. Messages in the queue are written to the corresponding socket descriptor when it is ready for writing. To ensure non-blocking behavior, the server sets the socket to be non-blocking using the ioctl function.

These mechanisms enable efficient event-driven processing, allowing the chat server to handle incoming messages and manage client connections effectively.

## Usage

To run the event-driven chat server, follow these steps:

1. Compile the source code using a suitable C compiler.
2. Run the compiled executable, providing the desired port number as a command-line parameter.
3. The server will start listening for incoming connections on the specified port.
4. Clients can connect to the server using a suitable chat client application.
5. The server will forward each incoming message to all connected clients, except for the client that sent the message.



Output messages will be printed to the console, providing information about the server's status, connections, and message handling.



