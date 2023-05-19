# Event-Driven Chat Server

## Description

The event-driven chat server is a program that allows multiple clients to connect and communicate with each other in a chat-like environment. The server utilizes an event-driven architecture to handle incoming connections and messages in an efficient and scalable manner.

The main function of the chat server is to forward each incoming message to all connected clients, excluding the client that sent the message. This behavior is implemented entirely in an event-driven manner, without the use of threads. The server uses the `select` function to monitor multiple sockets for activity and efficiently manage incoming and outgoing data.

Each client connection is represented by a separate socket, and the server maintains a queue for each connection to hold the messages that need to be sent to that particular client. When a client sends a message, the server adds it to the appropriate queues of all other connected clients, ensuring that the message is delivered to everyone except the sender.

The server provides console output to provide information about its status, including the acceptance of new connections, reading and writing data from and to sockets, and closure of client connections. Error handling mechanisms are in place to handle any failures and ensure smooth operation.

## Background

In this exercise, you will implement an event-driven chat server. The function of the chat server is to forward each incoming message to all client connections (i.e., to all clients) except for the client connection over which the message was received. The challenge in such a server lies in implementing this behavior in an entirely event-driven manner without the use of threads.

In this exercise, you will use the `select` function to check which socket descriptor is ready for reading or writing. You should call `select` inside a loop, but it should appear only once in your exercise.

When checking the read set, you should distinguish the main socket, which gets new connections, from the other sockets that negotiate data with the clients. When the main socket is ready for reading, you should call `accept`. When any other socket is ready for reading, you should call `read` or `recv`.

You will maintain a queue for each connection, which will hold all the messages that must be written on that connection.

When checking the write set, note that if a socket descriptor is ready for writing, you can write once to the socket without the risk of being blocked. Since you would like to write all messages in the queue to the socket descriptor, you will set the socket to be non-blocking. To make a socket with the socket descriptor `fd` to be non-blocking, you can use the `ioctl` function.

## Usage

To run the event-driven chat server, follow these steps:

1. Compile the source code using a suitable C compiler.
2. Run the compiled executable, providing the desired port number as a command-line parameter.
3. The server will start listening for incoming connections on the specified port.
4. Clients can connect to the server using a suitable chat client application.
5. The server will forward each incoming message to all connected clients, except for the client that sent the message.

To use the event-driven chat server, compile the source code, run the compiled executable with the desired port number as a command-line parameter, and connect to the server using a suitable chat client application. Messages exchanged between clients will be forwarded by the server to all connected clients, creating a real-time chat experience. 


Output messages will be printed to the console, providing information about the server's status, connections, and message handling.



