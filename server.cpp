#include "serverUtils.h"
#include <cpr/cpr.h>

PluginObject *Plugin = nullptr;

ReaderWriterLock vec_lock;
vector<Result> out;

string StylizedResults() {
   WithWriteLock wl(vec_lock);
   string resultsHtml = "";
   Crawler c;
   for (auto &result : out) {
      
      resultsHtml += string("<li><a href=\"") + result.url + string("\">");
      /*auto pUrl = ParsedUrl(url);
      auto buffer = std::make_unique<char[]>(1000000);
      size_t pageSize = 0;
      try {
         c.crawl(pUrl, buffer.get(), pageSize);
         auto parser = std::make_unique<HtmlParser>(buffer.get(), pageSize);
         for (int i = 0; i < parser->titleWords.size(); i++)
            resultsHtml += parser->titleWords[i] + string(" ");
         resultsHtml += string(" </a><br>") + url + string("</li>\n\n");
      } catch (std::runtime_error &e) {
         continue;
      }*/
      resultsHtml += string("placeholder title</a><br>") + result.url + string("</li>\n\n");
   }
   return resultsHtml;
}

// Root directory for the website, taken from argv[ 2 ].
// (Yes, a global variable since it never changes.)

char *RootDirectory;

const string talkport = "8080";

//TODO: edit according to the actual IPs of running servers
const int NUM_INDICES = 2;
const string indexIPs[NUM_INDICES] = {
   "34.42.99.109",
   "34.150.163.39"
};

struct curlStruct {
   string path;
   string host; 
   string port;

   // Custom copy assignment operator
   curlStruct() = default;
   curlStruct(const string &path_in, const string &h_in, const string &port_in) 
            : path(path_in), host(h_in), port(port_in) {}
   curlStruct& operator=(const curlStruct& other) {
      if (this != &other) {
         path = other.path;
         host = other.host;
         port = other.port;
      }
      return *this;
   }
};

void* getdata(void *args) {
   curlStruct* cURL = static_cast<curlStruct*>(args);
   addrinfo hints, *res;
   memset(&hints, 0, sizeof hints);
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM;

   if (getaddrinfo(cURL->host.c_str(), cURL->port.c_str(), &hints, &res) != 0) {
      std::cerr << "Error resolving hostname: " << cURL->host << std::endl;
      return nullptr;
   }

   int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
   if (sock == -1) {
      std::cerr << "Error creating socket" << std::endl;
      return nullptr;
   }

   if (connect(sock, res->ai_addr, res->ai_addrlen) == -1) {
      std::cerr << "Error connecting to server" << std::endl;
      close(sock);
      freeaddrinfo(res);
      return nullptr;
   }

   string ss;
   ss = string("GET ") + cURL->path + " HTTP/1.1\r\n" + 
   "Host: " + cURL->host + "\r\n" +
   "Connection: close\r\n\r\n";

   if (send(sock, ss.c_str(), ss.size(), 0) == -1) {
      std::cerr << "Error sending request" << std::endl;
      close(sock);
      freeaddrinfo(res);
      return nullptr;
   }

   char buffer[4096];
   std::string response;
   int buf_size = 0;
   ssize_t bytes_received;
   auto start_t = std::clock();
   while ((bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
      buffer[bytes_received] = '\0';
      response += buffer;
      buf_size += bytes_received;
   }

   if (bytes_received == -1) {
      std::cerr << "Error receiving data" << std::endl;
   } else {
      std::cout << "recv: " << string(buffer) << std::endl;
   }

   close(sock);
   freeaddrinfo(res);

   char *end = buffer + buf_size;
   char *p = buffer;
   char *start = nullptr;
   bool inlink;
   string url = "";
   int score = 0;
   std::unordered_set<size_t> seen;
   for (; p != end; p++) {
      if (string(p, 4, end) == "http")
         start = p;
      if (*p == '\t' && start != nullptr) {
         url = string(start, p - start, end);
         start = p;
      }
      if (*p == '\n' && start != nullptr) {
         score = atoi(string(start, p - start, end).c_str());
         WithWriteLock wl(vec_lock);
         size_t hash = hashbasic(url.c_str());
         if (seen.find(hash) == seen.end()) {
            std::cout << url << std::endl;
            out.push_back({score, url});
            seen.insert(hash);
         }
      }
   }

   return nullptr;
}

void getServerResults(string query) {
   vector<pthread_t> threads;
   out.clear();
   curlStruct argList[NUM_INDICES];
   int i = 0;
   for (auto &ip : indexIPs) {
      const string path = string("/search?q=") + query;
      pthread_t thread;
      argList[i] = {path, ip, talkport};
      pthread_create(&thread, nullptr, getdata, &argList[i]);
      threads.push_back(thread);
      i++;
   }
   for (auto &t : threads)
      pthread_join(t, nullptr);
   std::sort(out.begin(), out.end(), Driver::compareResults);
}
               
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
  
      // Unencode %20 etc.
      // query = UnencodeUrlEncoding(query);
  
      string templatePath = string(RootDirectory) + "/search.html";
      int templateFd = open(templatePath.c_str(), O_RDONLY);
      if (templateFd < 0) {
         FileNotFound(talkSocketid);
         close(talkSocketid);
         return nullptr;
      }

      off_t templateSize = FileSize(templateFd);
      if (templateSize < 0) {
         AccessDenied(talkSocketid);
         close(templateFd);
         close(talkSocketid);
         return nullptr;
      }

      char *templateBuffer = new char[templateSize + 1];
      ssize_t bytesRead = read(templateFd, templateBuffer, templateSize);
      templateBuffer[bytesRead] = '\0'; // null-terminate

      close(templateFd);

      string templateHtml(templateBuffer);
      delete[] templateBuffer;
      
      // Replace {{query}} in template
      size_t queryPosInHtml = templateHtml.find("{{query}}");
      if (queryPosInHtml != npos) {
          string before = templateHtml.substr(0, queryPosInHtml);
          string after = templateHtml.substr(queryPosInHtml + 9, templateHtml.size() - (queryPosInHtml + 9)); // 9 is length of {{query}}
          templateHtml = before + query + after;
          //templateHtml.replace(queryPosInHtml, 9, query);
      }

      // TODO: Replace {{results}} with actual search results.
      // Generate dummy results (replace with your real data if available)

      // input: string query
      // output: vector of string urls
      // title? 
      char * p = query.c_str();
      for ( ; *p; ++p) *p = tolower(*p);
      
      auto start = std::clock();
      getServerResults(query);
      string resultsHtml = StylizedResults();
      double duration = ( std::clock() - start ) / (double) CLOCKS_PER_SEC;
      
      // Replace {{results}} in template
      size_t resultsPos = templateHtml.find("{{results}}");
      if (resultsPos != npos) {
          string before = templateHtml.substr(0, resultsPos);
          string after = templateHtml.substr(resultsPos + 11, templateHtml.size() - (resultsPos + 11)); // 11 is length of {{results}}
          templateHtml = before + string(std::to_string(duration).c_str()) + string(" seconds taken.") + resultsHtml + after;
          //templateHtml.replace(resultsPos, 11, resultsHtml);
      }
      // Send HTTP response
      string header = (string)"HTTP/1.1 200 OK\r\n" +
                      (string)"Content-Type: text/html\r\n" +
                      (string)"Content-Length: " + to_string(templateHtml.length()) + "\r\n" +
                      (string)"Connection: close\r\n\r\n";
      
      send(talkSocketid, header.c_str(), header.length(), 0);
      send(talkSocketid, templateHtml.c_str(), templateHtml.length(), 0);
      close(talkSocketid);
      return nullptr;
   }
       
   // If the path refers to a directory, access denied.
   // If the path refers to a file, write it to the socket.
   else{
      string fullPath = string(RootDirectory) + path;

      int file = open(fullPath.c_str(), O_RDONLY);
      if (file < 0) {
         FileNotFound(talkSocketid);
         close(talkSocketid);
         return nullptr;
      }

      off_t size = FileSize(file);
      if (size < 0) {
         AccessDenied(talkSocketid);
         close(file);
         close(talkSocketid);
         return nullptr;
      }
      // Send HTTP header
      string header = (string)"HTTP/1.1 200 OK\r\n" +
                     (string)"Content-Type: " + string(Mimetype(fullPath)) + "\r\n" +
                     (string)"Content-Length: " + to_string(size) + "\r\n" +
                     (string)"Connection: close\r\n\r\n";
      
      send(talkSocketid, header.c_str(), header.length(), 0);

      // Send file content
      char fileBuffer[8192];
      ssize_t bytes;
      while ((bytes = read(file, fileBuffer, sizeof(fileBuffer))) > 0) {
         send(talkSocketid, fileBuffer, bytes, 0);
      }

      close(file);
      close(talkSocketid);
      return nullptr;
      // Close the socket and return nullptr.
   }

}



int main( int argc, char **argv )
   {
   if ( argc != 3 )
      {
      std::cerr << "Usage:  " << argv[ 0 ] << " port rootdirectory" << std::endl;
      return 1;
      }

   int port = atoi( argv[ 1 ] );
   RootDirectory = argv[ 2 ];

   // Discard any trailing slash.  (Any path specified in
   // an HTTP header will have to start with /.)

   char *r = RootDirectory;
   if ( *r )
      {
      do
         r++;
      while ( *r );
      r--;
      if ( *r == '/' )
         *r = 0;
      }

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
      std::cout << "serving " << NUM_INDICES << " index servers at http://localhost:" << port << "/index.html" << std::endl;
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
      {
      // TO DO:  Create and detach a child thread to talk to the
      // client using pthread_create and pthread_detach.

      // When creating a child thread, you get to pass a void *,
      // usually used as a pointer to an object with whatever
      // information the child needs.
      
      // The talk socket is passed on the heap rather than with a
      // pointer to the local variable because we're going to quickly
      // overwrite that local variable with the next accept( ).  Since
      // this is multithreaded, we can't predict whether the child will
      // run before we do that.  The child will be responsible for
      // freeing the resource.  We do not wait for the child thread
      // to complete.
      //
      // (A simpler alternative in this particular case would be to
      // caste the int talksocket to a void *, knowing that a void *
      // must be at least as large as the int.  But that would not
      // demonstrate what to do in the general case.)
      }

   close( listenSocket );
   }
