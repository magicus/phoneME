/*
 * @(#)io_sockets.c	1.0 07/10/12
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.  
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER  
 *   
 * This program is free software; you can redistribute it and/or  
 * modify it under the terms of the GNU General Public License version  
 * 2 only, as published by the Free Software Foundation.   
 *   
 * This program is distributed in the hope that it will be useful, but  
 * WITHOUT ANY WARRANTY; without even the implied warranty of  
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU  
 * General Public License version 2 for more details (a copy is  
 * included at /legal/license.txt).   
 *   
 * You should have received a copy of the GNU General Public License  
 * version 2 along with this work; if not, write to the Free Software  
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  
 * 02110-1301 USA   
 *   
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa  
 * Clara, CA 95054 or visit www.sun.com if you need additional  
 * information or have any questions. 
 *
 */

/*
 *  Module public API
 * 
 * 	int SIOInit()  
 *      - initializes the socket servers            
 *      - returns 0 on success and non-zero on error 	
 *  int SIOStop()  
 *      - closes sockets and frees all alocated system resources
 *      - returns 0 on success and non-zero on error 	
 *  int SIOWrite(int fd, char * buffer, int len)
 *      - writes len bytes from buffer into stream described by fd
 *      - acceptable values for fd are 1 for stdout and 2 for stderr
 *      - returns 0 if the supplied fd is valid returns 1 otherwise
 *  int SIOGetListeningPort(int fd)
 *      - returns port number for given file descriptor, zero otherwise
 *      - the module must be initialized with sio_init before this 
 *        function is called, -1 is returned otherwise
 * 
 * To enable debug messages compile with following line uncomented.
 * 
#define SIO_DEBUG 1
 *
 */

#ifdef WINCE
#include <winsock.h>
#else
#include <winsock2.h>
#endif

#ifndef WINSOCK_VERSION
#define WINSOCK_VERSION MAKEWORD(1,1)
#endif

#ifdef SIO_DEBUG
#include <stdio.h>
#endif

#define SIO_CLIENTS_DELTA 10

/* Data structures definictions */
typedef struct SioClientRegistry {
    SOCKET * list; 					/* connected clients */
    int length; 					/* number of slots allocated */
    int count; 						/* number of slots used */
    HANDLE mutex; 					/* operation synchronization mutex */
}
SIO_CLIENTSREGISTRY;

typedef struct SioServer {
    SOCKET socket; 					/* accepting socket */
    int port; 						/* socket port */
    DWORD thread_id; 				/* id of started thread */
    HANDLE thread; 					/* handle of started thread */
    SIO_CLIENTSREGISTRY clients; 	/* clients connected to this server */
}
SIO_SERVER;

/* Public Api functions */
int SIOInit(int, int);
int SIOStop();
int SIOWrite(int fd, char * buff, int len);
int SIOGetListeningPort(int);

/* Win Socket Provider data holder */
static WSADATA sio_WSA_data;

/* Server management data instances */
static SIO_SERVER sio_stdout_server;
static SIO_SERVER sio_stderr_server;
static int sio_servers_initialized = 0;

/* Server managerment functions */
static int sio_init_server(SIO_SERVER *, int);
static int sio_start_server(SIO_SERVER *);
static int sio_get_server_port(SIO_SERVER *);
static int sio_accept_clients(SIO_SERVER *);
static int sio_stop_server(SIO_SERVER *);

/* Thread entry functions */
DWORD WINAPI sio_server_thread_entry(LPVOID);

/* Clients management functions */
static void sio_lock_clients(SIO_SERVER *);
static void sio_unlock_clients(SIO_SERVER *);
static void sio_add_client(SIO_SERVER *, SOCKET);
static void sio_cleanup_clients(SIO_SERVER *);
static int sio_send_data_to_clients(SIO_SERVER *, char *, int);

/* WSA management */
static int sio_init_WSA();
static int sio_stop_WSA();

/* Debugging output management */
static void sio_debug(char *, ...);
static int sio_handle_WSA_error(char *);
static int sio_handle_system_error(char *);
static int sio_handle_error(char *, char *, int);

/*
 * Initializes all listening servers and their structures,
 * if the data is already initialized 0 is returned and the call
 * has no effect
 */
int SIOInit(int stdoutPort, int stderrPort) {
    int last_error;
    /* check for status  */
    if (sio_servers_initialized == 1) {
        return 0;
    }
    /* check port numbers  */
    if (stdoutPort < 0 && stderrPort < 0) {
        return 0;
    } else {
        /* provider should start
        must start both sockets
        so make sure values are correct */
        if (stdoutPort < 0) {
            stdoutPort = 0;
        }
        if (stderrPort < 0) {
            stderrPort = 0;
        }
    }
    /* start WSA */
    sio_debug("Starting WSA");
    last_error = sio_init_WSA();
    if (last_error != 0) {
        return last_error;
    }
    /* init servers  */
    sio_debug("Initializinig servers with ports %d and %d", stdoutPort, stderrPort);
    last_error = sio_init_server(&sio_stdout_server, stdoutPort);
    if (last_error != 0) {
        return last_error;
    }
    last_error = sio_init_server(&sio_stderr_server, stderrPort);
    if (last_error != 0) {
        return last_error;
    }
    /* start servers  */
    sio_debug("Starting servers");
    last_error = sio_start_server(&sio_stdout_server);
    if (last_error != 0) {
        return last_error;
    }
    last_error = sio_start_server(&sio_stderr_server);
    if (last_error != 0) {
        return last_error;
    }
    sio_servers_initialized = 1;
    return 0;
}

/*
 * Initializes server data structures
 */
static int sio_init_server(SIO_SERVER * server, int desiredPort) {
    SOCKADDR_IN listen_address;
    int last_error;
    /* create socket structure */
    server->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server->socket == INVALID_SOCKET) {
        return sio_handle_WSA_error("Canot create listening socket");
    }
    /* create listening address */
    listen_address.sin_family = AF_INET;
    listen_address.sin_addr.s_addr = INADDR_ANY;
    listen_address.sin_port = htons(desiredPort);
    /* bind the socket to the local address */
    if ( bind(server->socket, (SOCKADDR*) &listen_address, sizeof(listen_address)) == SOCKET_ERROR ) {
        return sio_handle_WSA_error("Canot bind listening socket");
    }
    /* retrieve and store the bound port */
    last_error = sio_get_server_port(server);
    if (last_error != 0) {
        return last_error;
    }
    /* init locking structures */
    server->clients.mutex = CreateMutex(NULL, FALSE, NULL);
    if (server->clients.mutex == NULL) {
        return sio_handle_system_error("Failed to create mutex");
    }
    /* init clients array */
    server->clients.length = SIO_CLIENTS_DELTA;
    server->clients.count = 0;
    server->clients.list = (SOCKET *) malloc(server->clients.length * sizeof(SOCKET));
    return 0;
}

/* 
 *  Reads port information from socket
 */
int sio_get_server_port(SIO_SERVER * server) {
    struct sockaddr_in address;
    int len = sizeof(address);
    if (getsockname(server->socket, (SOCKADDR*) &address, &len) == SOCKET_ERROR) {
        return sio_handle_WSA_error("Canot retrieve server address");
    } else {
        server->port = ntohs(address.sin_port);
        sio_debug("Server started on port %d", server->port);
        return 0;
    }
}

/* 
 *  Starts the accepting thread
 */
static int sio_start_server(SIO_SERVER * server) {
    int last_error;
    /* set socket as listening socket */
    if (listen(server->socket, SOMAXCONN) == SOCKET_ERROR) {
        return sio_handle_WSA_error("Error listening on socket");
    }
    /*  create the thread with system default settings */
    server->thread = CreateThread(NULL, 0, sio_server_thread_entry, server, 0, &server->thread_id);
    if (server->thread == NULL) {
        return sio_handle_system_error("Canot start listener");
    }
    return 0;
}

/* 
 *  Entry point for the listening thread
 *  Just invokes sio_accept_clients
 */
static DWORD WINAPI sio_server_thread_entry(LPVOID server) {
    return sio_accept_clients((SIO_SERVER *) server);
}

/* 
 *  Accepts clients on a socked
 *  If the socket is closed or WSA disabled the function shuts down
 */
static int sio_accept_clients(SIO_SERVER * server) {
    SOCKET new_client;
    int res = 0;
    sio_debug("Listener thread started");
    while (1) {
        new_client = accept(server->socket, NULL, NULL);
        if (server->clients.length <= 0) {
            sio_debug("Server terminating.");
            closesocket(new_client);
            res = 0;
            break;
        } else if (new_client == INVALID_SOCKET) {
            res = sio_handle_WSA_error("Error while accepting client. Stopping server.");
            break;
        } else {
            sio_lock_clients(server);
            sio_add_client(server, new_client);
            sio_unlock_clients(server);
        }
    }
    sio_debug("Listener finished");
    return res;
}

/* 
 *  Terminates all running servers
 *  If the module was not started using sio_init
 *  0 is returned and nothing is done.
 */
int SIOStop() {
    int last_error = 0;
    /* check status */
    if (sio_servers_initialized == 0) {
        return 0;
    }
    /* stop servers */
    last_error = sio_stop_server(&sio_stdout_server);
    if (last_error != 0) {
        return last_error;
    }
    last_error = sio_stop_server(&sio_stderr_server);
    if (last_error != 0) {
        return last_error;
    }
    /* stop WSA */
    if (sio_stop_WSA() != 0) {
        return sio_handle_WSA_error("Canot stop WSA provider");
    }
    sio_servers_initialized = 0;
    sio_debug("Socket IO provider stopped");
    return 0;
}

/* 
 *  Closes the listening socket and performs cleanup of alocated resources
 */
static int sio_stop_server(SIO_SERVER * server) {
    sio_cleanup_clients(server);
    /* close listening socket */
    if (closesocket(server->socket) == SOCKET_ERROR) {
        return sio_handle_WSA_error("Canot close server socket");
    }
    /* wait for listen thread to end */
    WaitForSingleObject(server->thread, INFINITE);
    CloseHandle(server->thread);
    CloseHandle(server->clients.mutex);
    sio_debug("Server stopped");
    return 0;
}

/* 
 *  Acquires the Mutex for current thread
 */
static void sio_lock_clients(SIO_SERVER * server) {
    DWORD wait_result;
    int last_error;
    wait_result = WaitForSingleObject(server->clients.mutex, INFINITE);
    while (wait_result != WAIT_OBJECT_0) {
        if (wait_result == WAIT_FAILED) {
            sio_handle_system_error("Canot obtain mutex lock");
            return;
        } else {
            sio_debug("Failed to obtain lock with result %d. Trying again", wait_result);
        }
        wait_result = WaitForSingleObject(server->clients.mutex, INFINITE);
    }
    sio_debug("LOCK");
}

/* 
 *  Releases Mutex acqired for current thread
 */
static void sio_unlock_clients(SIO_SERVER * server) {
    if (!ReleaseMutex(server->clients.mutex)) {
        sio_handle_system_error("Canot release mutex");
    }
    sio_debug("UNLOCK");
}

/* 
 *  Adds a client to active clients registry
 *  Performs registry expansion if necessary
 *  Current thread must hold the registry Mutex in order to guarantee
 *  thread-security
 */
static void sio_add_client(SIO_SERVER * server, SOCKET client) {
    SOCKET * old_clients = NULL;
    int i = 0;
    /* expand if necessary */
    if (server->clients.count == server->clients.length) {
        sio_debug("Expanding clients array");
        old_clients = server->clients.list;
        server->clients.length += SIO_CLIENTS_DELTA;
        server->clients.list =
	    (SOCKET *) malloc(server->clients.length * sizeof(SOCKET));
        for (i = 0; i < server->clients.count; i++) {
            server->clients.list[i] = old_clients[i];
        }
        free(old_clients);
        sio_debug("Done");
    }
    /* add a client */
    server->clients.list[server->clients.count] = client;
    server->clients.count++;
    sio_debug("Client added");
}

/* 
 *  Disconnects all client sockets and frees alocated memory
 */
static void sio_cleanup_clients(SIO_SERVER * server) {
    int index;
    sio_lock_clients(server);
    /* disconnect */
    for ( index = 0; index < server->clients.count; index++ ) {
        if (closesocket(server->clients.list[index]) == SOCKET_ERROR) {
            sio_handle_WSA_error("Canot close client socket");
        }
    }
    /* cleanup and free */
    server->clients.count = 0;
    server->clients.length = -1;
    free(server->clients.list);
    server->clients.list = NULL;
    sio_unlock_clients(server);
}

/* 
 *  Sends data to clients depending on file descriptor
 *  1 = stdout, 2 = stderr
 */
int SIOWrite(int fd, char * buff, int len) {
    /* module state */
    if (sio_servers_initialized != 1) {
        return -1;
    }
    /* send output */
    if (fd == 1) {
        sio_send_data_to_clients(&sio_stdout_server, buff, len);
        return 0;
    } else if (fd == 2) {
        sio_send_data_to_clients(&sio_stderr_server, buff, len);
        return 0;
    } else {
        return 1;
    }
}

/* 
 *  Gets port number of server asociated with given file
 *  descriptor. 0 on invalid descriptor, -1 if the module
 *  was not initialized.
 */
int SIOGetListeningPort(int fd) {
    /* validate input and module state */
    if (sio_servers_initialized != 1) {
        return -1;
    } else if (fd < 1 || fd > 2) {
        return 0;
    }
    /* return port number */
    if (fd == 1) {
        return sio_stdout_server.port;
    } else if (fd == 2) {
        return sio_stderr_server.port;
    }
    /* this is to avoid compiler complaints */
    return 0;
}

/* 
 *  Sends supplied data to all connected clients
 *  This function acquires its own lock
 */
static int sio_send_data_to_clients(SIO_SERVER * server, char * data, int len) {
    int cli_iter, add_iter;
    int * log;
    /* prepare log */
    sio_lock_clients(server);
    log = (int *) malloc(server->clients.count * sizeof(int));
    /* send data to clients */
    for ( cli_iter = 0; cli_iter < server->clients.count; cli_iter++ ) {
        if (send(server->clients.list[cli_iter], data, len, 0) == SOCKET_ERROR) {
            log[cli_iter] = sio_handle_WSA_error("Canot send data to client, closing");
        } else {
            log[cli_iter] = 0;
        }
    }
    /* remove disfunctional sockets */
    add_iter = 0;
    for ( cli_iter = 0; cli_iter < server->clients.count; cli_iter++ ) {
        if (log[cli_iter] == 0) {
            server->clients.list[add_iter] = server->clients.list[cli_iter];
            add_iter++;
        }
    }
    free(log);
    server->clients.count = add_iter;
    sio_debug("Sent %d bytes to %d clients", len, server->clients.count);
    sio_unlock_clients(server);
    return 0;
}

/* 
 *  Initializes the WSA Provider
 */
static int sio_init_WSA() {
    int error_code;
    error_code = WSAStartup(WINSOCK_VERSION, &sio_WSA_data);
    if (error_code != 0) {
        sio_debug("WSAStartup() failed with error code = %d", error_code);
        return error_code;
    } else {
        return 0;
    }
}

/* 
 *  Stops the WSA provider
 */
static int sio_stop_WSA() {
    if (WSACleanup() != 0) {
        return sio_handle_WSA_error("WSACleanup failed");
    } else {
        return 0;
    }
}

/* 
 *  Prints a debug message of last occured WSA error
 */
static int sio_handle_WSA_error(char * msg) {
    return sio_handle_error("WSA", msg, WSAGetLastError());
}

/* 
 *  Prints a debug message of last occured System error
 */
static int sio_handle_system_error(char * msg) {
    return sio_handle_error("System", msg, GetLastError());
}

/* 
 *  Prints a debug error message for given error code with given
 *  descrioption and error type
 */
static int sio_handle_error(char * type, char * msg, int error_code) {
#ifdef SIO_DEBUG
    LPVOID message_buffer;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
        NULL, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &message_buffer,
        0, NULL );
    sio_debug("%s Error occured: %s. Error: %d (%s).", type, msg, error_code, message_buffer);
    LocalFree(message_buffer);
#endif
    return error_code;
}

/* 
 *  Depending on compilation directive settings
 *  prints supplied message to stdout
 */
static void sio_debug(char * msg, ...) {
#ifdef SIO_DEBUG
    va_list args;
    va_start(args, msg);
    vfprintf(stdout, msg, args);
    fprintf(stdout, "\n");
    va_end(args);
    return;
#endif
}
