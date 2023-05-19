
#include "chatServer.h"  // conn_pool_t, msg_t, conn_t, init_pool, add_conn, remove_conn, add_msg, write_to_client

#include <stdio.h> // perror, printf

#include <signal.h> // signal, SIGINT

#include <stdlib.h> // exit, atoi, malloc

#include <netinet/in.h> // socket, SOCK_STREAM, PF_INET, htons, accept

#include <sys/ioctl.h> // ioctl, FIONBIO

#include <unistd.h> // read, write ,close

#include <string.h> // strcpy



// Macro for the pool

// the max value in file descriptor table used by select function
#define pool_maxFD(p) p -> maxfd
// select return value how many fd is ready
#define pool_readyFD(p) p -> nready
// set for read
#define pool_readSet(p) p -> read_set
// working set for read
#define pool_readyReadSet(p) p -> ready_read_set
// set for write
#define pool_writeSet(p) p -> write_set
// working set for write
#define pool_readyWriteSet(p) p -> ready_write_set
// the head of the connection list
#define pool_connection_head(p) p -> conn_head
//the number of the connection
#define pool_connection_number(p) p -> nr_conns

// Macro for the connection

//get the next connection
#define connection_next(c) c -> next
//get the previous connection
#define connection_previous(c) c -> prev
//get the head queue messages
#define connection_messages_head(c) c -> write_msg_head
//get the tail queue messages
#define connection_messages_tail(c) c -> write_msg_tail
//get the connection fd
#define connection_FD(c) c -> fd

//Macro for the message

//get the next message
#define message_next(n) n -> next
//get the previous message
#define message_prev(n) n -> prev
//get the messages
#define message_val(n) n -> message
//get the messages size
#define message_size(n) n -> size

static int end_server = 0; // to stop the server

void intHandler(int SIG_INT)
{// when we receive signal we stop CTRL-C
    end_server = 1;
}

void initConnection(conn_t*, int); // start connection to add to the list
int dbllist_remove(conn_pool_t*, conn_t*); // delete node from list
msg_t * copyMsg(char*, int); // copy and malloc new  msg
int isNum(char*, int); // str is digit only


int main(int argc, char * argv[])
{


    if ( argc < 2 || argc > 2 )
    { // check the number of the parameter we should receive one
        printf( "\nUsage: server <port> \n" ); // input legacy
        exit( 1 ); // exit the program
    }
    if ( isNum( argv[ 1 ], strlen( argv[ 1 ] ) ) == -1 )
    { // the input " port number " should contain digit only
        printf("\nUsage: server <port> \n" ); // input legacy
        exit( 1 ); // exit the program
    }
    // change the string to number
    int welcomeSocketPort = atoi( argv[ 1 ] );
    if ( welcomeSocketPort > 65536 || welcomeSocketPort < 0 )
    { // check if the value of the number is in the appropriate interval 1 - 2^16
        printf("\nUsage: server <port> \n"); // input legacy
        exit( 1 ); // exit the program
    }


    // assign the signal SIGINT to intHandler - when we get SIGINT we call intHandler
    signal( SIGINT,  intHandler );
    int welcomeSocket; // fd Welcome Socket - here we receive the new connections
    char buf[ BUFFER_SIZE ]; // buffer to handle the read operation  - read into it
    int newSocket; // fd new socket
    int on = 1; // value to pass to ioctl


    // address to welcome socket
    struct sockaddr_in *welcomeSocketAddress =( struct sockaddr_in* ) malloc(sizeof ( struct sockaddr_in )); // Socket Address
    // bind welcomeSocket
    welcomeSocketAddress -> sin_family = AF_INET; // family
    welcomeSocketAddress -> sin_addr.s_addr =  INADDR_ANY ; // any address
    welcomeSocketAddress -> sin_port = htons( welcomeSocketPort ); // port number


    conn_pool_t * pool;
    // make the pool
    if( ( pool = malloc( sizeof( conn_pool_t ) ) ) == NULL){
        free( welcomeSocketAddress ); // free the welcome socket address
        printf("\nMalloc\n"); // malloc fail
        exit( 1 ); // exit the program
    }
    // start the pool of the connection
    init_pool( pool );

    // socket
    // make welcomeSocket - start it
    if ( ( welcomeSocket = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1)
    {
        free( welcomeSocketAddress ); // free the welcome socket address
        free( pool ); // in case of error free the data
        perror( "\nsocket\n" ); // print error and exit
        exit( 1 ); // exit the program
    }

    // ioctl
    // non-blocking socket - so when we read or write multiple time we will not be blocked
    if ( ioctl( welcomeSocket, ( int ) FIONBIO, ( char * ) & on ) == -1)
    {
        free( welcomeSocketAddress ); // free the welcome socket address
        close( welcomeSocket ); // close the welcome socket
        free( pool ); // in case of error free the data
        perror( "\nioctl\n" ); // print error and exit
        exit( 1 ); // exit the program
    }

    // bind
    if ( bind( welcomeSocket, ( struct sockaddr * ) welcomeSocketAddress, sizeof( *welcomeSocketAddress ) ) == -1 )
    {
        free( welcomeSocketAddress ); // free the welcome socket address
        close( welcomeSocket ); // close the welcome socket
        free( pool ); // in case of error free the data
        perror( "\nbind\n" ); // print error and exit
        exit( 1 ); // exit the program
    }

    // listen
    if ( listen( welcomeSocket, 5 ) == -1 )
    {// set num request in the same time
        free( welcomeSocketAddress ); // free the welcome socket address
        close( welcomeSocket ); // close the welcome socket
        free( pool ); // in case of error free the data
        perror( "\nlisten\n" ); // print error and exit
        exit( 1 ); // exit the program
    }

    // set welcomeSocket to pool_readSet - we want to check welcomeSocket with select
    FD_SET( welcomeSocket, & pool_readSet( pool ) );

    // max fd  now surely is welcomeSocket because he is the first fd we start ,0 1 2 is fd saved per process
    pool_maxFD( pool ) = welcomeSocket;
    do
    { // let's go
        // assign the readyReadSet set  - readSet changes  and  readyReadSet work
        memcpy( & pool_readyReadSet( pool ), & pool_readSet( pool ), sizeof(pool_readSet( pool ) ) );
        // assign the readyWriteSet set  - writeSet changes  and  readyWriteSet work
        memcpy( & pool_readyWriteSet( pool ), & pool_writeSet( pool ), sizeof(pool_writeSet( pool ) ) );
        printf("Waiting on select()...\nMaxFd %d\n", pool_maxFD(pool));
        // select function  take max fd and working sets and waite to ready socket "fd"
        if ( ( pool_readyFD(pool) = select(pool_maxFD(pool) + 1, & pool_readyReadSet(pool),& pool_readyWriteSet(pool), 0, 0)) == -1)
        {
            end_server = 1; // stop the loop
            perror("\nselect\n"); // error
            continue; // skip the for to exit
        }

        // run over the fd and check who is ready
        for ( int w = 0, j = 0; w <= pool_maxFD(pool) && j < pool_readyFD(pool); w++ )
        {
            // ready to read !?!
            if ( FD_ISSET( w, & pool_readyReadSet( pool ) ) )
            {
                buf[0] = '\0';
                // ready for read && welcome socket
                //   ****    new connection    ****
                if ( welcomeSocket == w )
                {
                    j++; // we hande one
                    // accept
                    // **** take the new connection ****
                    if ( ( newSocket = accept( welcomeSocket, NULL, NULL ) ) == -1)
                    {
                        perror( "\naccept\n" ); // error
                        continue; // skip this iteration
                    }
                    // add connection  to the pool
                    if ( ( add_conn( newSocket, pool ) ) == -1 )
                        continue; // skip this iteration
                    // if new fd bigger than  ool_maxFD
                    if ( newSocket > pool_maxFD( pool ) )
                        pool_maxFD(pool) = newSocket;
                    // print
                    printf( "New incoming connection on sd %d\n", newSocket );
                    // add to be checked for reading
                    FD_SET( newSocket, & pool_readSet( pool ) );
                }
                    // regular socket
                else if ( welcomeSocket != w )
                { // if the fd is not the welcome socket this mean regular socket, so we read from it
                    j++; // we hande one
                    // print
                    printf( "Descriptor %d is readable \n", w );
                    // remove connection
                    if ( read(w,buf,BUFFER_SIZE) == 0 ) // if read return 0
                    { // we read nothing close
                        remove_conn( w, pool ); // remove from the pool
                        printf("Connection closed for sd %d \n", w );
                        printf("removing connection with sd %d \n", w );

                    }
                    else if( ( int ) strlen( buf ) != 0 )
                    { // else we read msg

                        printf( "%d bytes received from sd %d \n", ( int ) strlen( buf ), w );
                        //  send to  other  msg
                        //************ add to the all connections ****************
                        if ( add_msg( w, buf, strlen( buf ), pool ) == -1 )
                        {
                            continue; // skip the iteration
                        }
                    }
                }
            }
            //  ready for write
            if ( FD_ISSET( w, & pool_readyWriteSet( pool ) ) )
            {
                j++; // we hande one
                if ( write_to_client(w, pool) == -1 )
                {
                    continue; // skip
                }
            }
        }
    } while ( end_server == 0 );

    // ******************* here free - close

    conn_t * nex = pool_connection_head( pool ); // this use to rune over the list
    conn_t * prv; // this used to free data
    while ( nex ) // until the last member
    {
        prv = nex; // previous
        nex = connection_next( nex ); // the next
        if ( connection_FD(prv) != welcomeSocket ) // if it is not the welcome
            printf( "removing connection with sd %d \n", connection_FD( prv ) );
        remove_conn( connection_FD( prv ), pool ); // remove  connection
    }
    free( pool ); // free the pool
    free( welcomeSocketAddress ); // free the welcome socket address
    close( welcomeSocket ); // close the welcome socket
    return 0; // success
}
/*
 * start the pool valus
 */
int init_pool(conn_pool_t * pool)
{ // start the pool values
    // the ready fd and number connection is 0
    pool_readyFD( pool ) = pool_maxFD( pool ) = pool_connection_number( pool ) = 0;
    // the head of the connection list is null
    pool_connection_head( pool ) = NULL;
    // start the list pool_readSet
    FD_ZERO( & pool_readSet( pool ) );
    // start the list pool_writeSet
    FD_ZERO( & pool_writeSet( pool ) );
    // start the list pool_readyReadSet
    FD_ZERO( & pool_readyReadSet( pool ) );
    // start the list pool_readyWriteSet
    FD_ZERO( & pool_readyWriteSet( pool ) );
    return 0; // success
}
/*
 * start the connection
 */
void initConnection(conn_t * connection, int FD)
{ // start the connection values
    // head and tail of the msg list is null
    connection_messages_head( connection ) = connection_messages_tail( connection ) = NULL;
    // next and previous is null
    connection_next( connection ) = connection_previous( connection ) = NULL;
    // the assign fd
    connection_FD( connection ) = FD;
}
/*
 * add connection to the pool
 */
int add_conn(int sd, conn_pool_t * pool)
{
    // the number of connection  rise by 1
    pool_connection_number( pool ) ++;
    // make the connection
    conn_t * connection;
    if ( ( connection = ( conn_t* ) malloc( sizeof( conn_t ) ) ) == NULL)
    {
        return -1; // malloc failed
    }
    // start the connection
    initConnection( connection, sd );
    // check if the list of the pool  is empty
    if ( pool_connection_head( pool ) == NULL )
        pool_connection_head( pool ) = connection;
    else
    { // the list not empty add to the end
        // the prev of the head is connection
        connection_previous( pool_connection_head( pool ) ) = connection;
        // the next of the connection is head
        connection_next( connection ) = pool_connection_head(pool);
        // now the connection take the head place
        pool_connection_head( pool ) = connection;
    }
    return 0; // success
}
/*
 * remove conn from the list
 */
int remove_conn(int sd, conn_pool_t * pool)
{
    msg_t * msg; // msg

    /*
     * search the node that we want to remove
     */
    conn_t * search_node;
    for ( search_node = pool_connection_head( pool ); search_node != NULL; search_node = connection_next( search_node ) )
    {  //  check fd - search
        if ( connection_FD( search_node ) == sd)
        {

            if ( connection_messages_head( search_node ) != NULL ) {
                /*
                 * run over the list msg  and deallocate the msg
                 */
                for (; connection_messages_head(search_node) != NULL;) {
                    msg = connection_messages_head(search_node); // take the msg
                    connection_messages_head(search_node) = message_next(
                            connection_messages_head(search_node)); // skip to the next
                    free(message_val(msg)); // free the msg data
                    message_val(msg) = NULL; //msg val is null
                    free(msg); // free the msg
                    msg = NULL; // msg is null
                }
            }

            dbllist_remove( pool, search_node ); // remove from list
            free( search_node ); // fre its data
            break; // stop search
        }
    }

    // the number of connection  decries by 1
    pool_connection_number( pool ) --;
    // remove from read set
    FD_CLR( sd, & pool_readSet( pool ) );
    // close its fd
    close( sd );
    // remove from write set
    FD_CLR( sd, & pool_writeSet( pool ) );

    /*
     * if the deleted sd equal to max fd find the second max
     */
    if ( sd == pool_maxFD( pool ) )
    {
        pool_maxFD(pool) = 3; // the min fd
        for ( search_node = pool_connection_head( pool ); search_node != NULL; search_node = connection_next( search_node ) )
        { // find the max fd
            if (connection_FD( search_node ) > pool_maxFD( pool ) )
            { // we find number bigger than pool_maxFD
                pool_maxFD( pool ) = connection_FD( search_node );
            }
        }
    }

    return 0; // succeed
}
/*
 * add msg to the connections
 */
int add_msg(int sd, char * buffer, int len, conn_pool_t * pool)
{
    // search to the connection
    msg_t * addMsg; // msg
    for ( conn_t * search_node = pool_connection_head( pool ); search_node != NULL; search_node = connection_next( search_node ) )
    { // run over the connections
        if ( connection_FD(search_node) == sd )
            continue; // if it is the welcome socket we do not want to write to it
        addMsg = copyMsg( buffer, len ); // new msg
        if ( addMsg == NULL ) // failed to make msg
            return -1;
        /*
         * add msg to the list
         */
        if  ( connection_messages_head( search_node ) != NULL )
        { // the msg list not empty
            // the next of tail is msg
            message_next( connection_messages_tail( search_node ) ) = addMsg;
            // the prev of the head is msg
            message_prev( addMsg ) = connection_messages_tail( search_node );
            // the msg is now the tail
            connection_messages_tail( search_node ) = addMsg;
        }
        // if the msg list is empty
        if ( connection_messages_head( search_node ) == NULL )
            // head and tail is null
            connection_messages_head( search_node ) = connection_messages_tail( search_node ) = addMsg;
        // set the connection to be checked if we can write to it
        FD_SET( connection_FD( search_node ), & pool_writeSet( pool ));
    }
    return 0;
}
/*
*  write to the connection
*/
int write_to_client(int sd, conn_pool_t * pool)
{
    msg_t * msg; // msg
    /*
     * run over the list connection search to the connection
     */
    for ( conn_t * search_node = pool_connection_head( pool ); search_node != NULL; search_node = connection_next( search_node ) )
    {    // search to the connection
        if ( connection_FD( search_node ) == sd )// if we find the connection  start write
        {
            /*
             * run over the list msg and write and deallocate the msg
             */
            for ( ; connection_messages_head( search_node ) != NULL; )
            {
                msg = connection_messages_head( search_node ); // take the msg
                connection_messages_head( search_node ) = message_next( connection_messages_head( search_node ) ); // skip to the next
                if ( write( sd, message_val( msg ), message_size( msg ) ) == -1 ) // write the msg
                {
                    perror("\nwrite\n");
                    return -1;
                }

                free( message_val( msg ) ); // free the msg data
                message_val( msg ) = NULL; //msg val is null
                free( msg ); // free the msg
                msg = NULL; // msg is null
            }
            break;   // stop search
        }
    }
    // remove the connection from the set of write
    FD_CLR( sd, & pool_writeSet( pool ) );
    return 0;
}
/*
 * remove node from list
 */
int dbllist_remove(conn_pool_t * pool, conn_t * conn_node)
{
    if ( pool_connection_head( pool ) == conn_node && pool_connection_number( pool ) == 1 )
    { // the list will be empty
        pool_connection_head( pool ) = NULL;
    } else if ( pool_connection_head( pool ) == conn_node )
    { // delete the head
        pool_connection_head( pool ) = connection_next( pool_connection_head( pool ) );
        connection_previous( pool_connection_head(pool ) ) = NULL;
    } else
    { // the node somewhere in the middle
        connection_next( connection_previous( conn_node ) ) = connection_next( conn_node );
        if ( connection_next(conn_node) != NULL ) // it in the tail
            connection_previous(connection_next( conn_node ) ) = connection_previous( conn_node );
    }
    return 0; // success
}
/*
 * copy msg
 */
msg_t * copyMsg(char * buffer, int len)
{
    // to use copy determine and to the buffer
    buffer[ len ] = '\0';
    msg_t* msg; // msg
    char* str; // the string
    if ( ( msg = ( msg_t* ) malloc( sizeof( msg_t ) ) ) == NULL ) // make the msg
        return NULL;
    if ( ( str = ( char* ) malloc(( len + 1 ) * sizeof( char ) ) ) == NULL ) // make the string
        return NULL;
    // copy the msg
    strcpy( str, buffer );
    // start the msg head tail
    message_next( msg ) = message_prev( msg ) = NULL;
    // the msg tail
    message_size( msg ) = len;
    // the msg val
    message_val( msg ) = str;
    // return the msg
    return msg;
}
/*
 * check string if it is contained digit
 */
int isNum( char * str, int len )
{
    /*
     * run over the chars
     */
    int j = 0;
    while ( j < len )
    {
        if ( str[ j ] > 57 || str[ j ] < 48 )
        {
            return -1;
        }
        j++;
    }
    return 0;
}