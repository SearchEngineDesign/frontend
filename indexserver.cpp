#include "serverUtils.h"

PluginObject *Plugin = nullptr;

// Root directory for the website, taken from argv[ 2 ].
// (Yes, a global variable since it never changes.)
               
void *Talk( void *talkSocket )
   {
   // TO DO:  Look for a GET message, then reply with the
   // requested file.

   // Cast from void * to int * to recover the talk socket id
   // then delete the copy passed on the heap.
   int talkSocketid = *(int *)talkSocket;
   delete (int *)talkSocket;

   // Read the request from the socket and parse it to extract
   // the action and the path, unencoding any %xx encodings.
   char buffer[10240];
   ssize_t bytesRead = recv(talkSocketid, buffer, sizeof(buffer) - 1, 0);
   if (bytesRead <= 0) {
      close(talkSocketid);
      return nullptr;
   }
   buffer[bytesRead] = '\0';

   // Parse the request
   string request(buffer);
   size_t firstSpace = request.find(" ");
   size_t secondSpace = request.find(" ", firstSpace + 1);
   
   if (firstSpace == npos || secondSpace == npos) {
      AccessDenied(talkSocketid);
      close(talkSocketid);
      return nullptr;
   }

   string action = request.substr(0, firstSpace);
   string path = request.substr(firstSpace + 1, secondSpace - firstSpace - 1);
   // path = UnencodeUrlEncoding(path);


   // Check to see if there's a plugin and, if there is,
   // whether this is a magic path intercepted by the plugin.
   
   //    If it is intercepted, call the plugin's ProcessRequest( )
   //    and send whatever's returned over the socket.
   if (Plugin && Plugin->MagicPath(path)) {
      string response = Plugin->ProcessRequest(request);
      send(talkSocketid, response.c_str(), response.length(), 0);
      close(talkSocketid);
      return nullptr;
   }
   // If it isn't intercepted, action must be "GET" and
   // the path must be safe.
   if (action != (string)"GET" || !SafePath(path.c_str())) {
      std::cout << "Access denied" << std::endl;
      AccessDenied(talkSocketid);
      close(talkSocketid);
      return nullptr;
   }

   if (path.find("/search") == 0) {
      size_t queryPos = path.find("?q=");
      string query;
      if (queryPos != npos) {
          query = path.substr(queryPos + 3);
          for (int i = 0; i < query.size(); i++)
            if(query[i] == '+')
               query[i] = ' ';
      }

      vector<string> urls = getResults(query);
      string buf;
      for (auto &s : urls)
         buf += s + "\n";

      // Send HTTP response
      string header = (string)"HTTP/1.1 200 OK\r\n" +
                      (string)"Content-Type: text/html\r\n" +
                      (string)"Content-Length: " + to_string(buf.length()) + "\r\n" +
                      (string)"Connection: close\r\n\r\n";
      
      send(talkSocketid, header.c_str(), header.length(), 0);
      send(talkSocketid, buf.c_str(), buf.length(), 0);
      close(talkSocketid);
      return nullptr;
   }
}



int main( int argc, char **argv )
   {
   if ( argc != 2 )
      {
      std::cerr << "Usage:  " << argv[ 0 ] << " port " << std::endl;
      return 1;
      }

   int port = atoi( argv[ 1 ] );
   // We'll use two sockets, one for listening for new
   // connection requests, the other for talking to each
   // new client.

   int listenSocket, talkSocket;

   // Create socket address structures to go with each
   // socket.

   struct sockaddr_in listenAddress,  talkAddress;
   socklen_t talkAddressLength = sizeof( talkAddress );
   memset( &listenAddress, 0, sizeof( listenAddress ) );
   memset( &talkAddress, 0, sizeof( talkAddress ) );
   
   // Fill in details of where we'll listen.
   
   // We'll use the standard internet family of protocols.
   listenAddress.sin_family = AF_INET;

   // htons( ) transforms the port number from host (our)
   // byte-ordering into network byte-ordering (which could
   // be different).
   listenAddress.sin_port = htons( port );

   // INADDR_ANY means we'll accept connections to any IP
   // assigned to this machine.
   listenAddress.sin_addr.s_addr = htonl( INADDR_ANY );

   // TO DO:  Create the listenSocket, specifying that we'll r/w
   // it as a stream of bytes using TCP/IP.
   listenSocket = socket(AF_INET, SOCK_STREAM, 0);
   if (listenSocket < 0) {
      std::cerr << "Failed to create listen socket" << std::endl;
      return 1;
   }

   // TO DO:  Bind the listen socket to the IP address and protocol
   // where we'd like to listen for connections.
   int enable = 1;
   if (setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) {
      perror("setsockopt(SO_REUSEADDR) failed");
   }
   if (::bind(listenSocket, (struct sockaddr *)&listenAddress, sizeof(listenAddress)) < 0) {
      std::cerr << "Failed to bind socket" << std::endl;
      close(listenSocket);
      return 1;
   }

   // TO DO:  Begin listening for clients to connect to us.

   // The second argument to listen( ) specifies the maximum
   // number of connection requests that can be allowed to
   // stack up waiting for us to accept them before Linux
   // starts refusing or ignoring new ones.
   //
   // SOMAXCONN is a system-configured default maximum socket
   // queue length.  (Under WSL Ubuntu, it's defined as 128
   // in /usr/include/x86_64-linux-gnu/bits/socket.h.)

   if (listen(listenSocket, SOMAXCONN) < 0) {
      std::cerr << "Failed to listen on socket" << std::endl;
      close(listenSocket);
      return 1;
   }

   // TO DO;  Accept each new connection and create a thread to talk with
   // the client over the new talk socket that's created by Linux
   // when we accept the connection.
   while (true) {
      std::cout << "waiting for accept..." << std::endl;
      talkSocket = accept(listenSocket, (struct sockaddr *)&talkAddress, &talkAddressLength);
      if (talkSocket < 0) {
         std::cerr << "Failed to accept connection" << std::endl;
         continue;
      }

      // Create new thread to handle connection
      int *socketPtr = new int(talkSocket);
      pthread_t thread;
      if (pthread_create(&thread, nullptr, Talk, socketPtr) != 0) {
         std::cerr << "Failed to create thread" << std::endl;
         delete socketPtr;
         close(talkSocket);
         continue;
      }
      pthread_detach(thread);
   }

   close( listenSocket );
   }
