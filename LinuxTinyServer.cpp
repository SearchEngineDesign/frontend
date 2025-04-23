// Linux tiny HTTP server.
// Nicole Hamilton  nham@umich.edu

// This variation of LinuxTinyServer supports a simple plugin interface
// to allow "magic paths" to be intercepted.  (But the autograder will
// not test this feature.)

// Usage:  LinuxTinyServer port rootdirectory

// Compile with g++ -pthread LinuxTinyServer.cpp -o LinuxTinyServer
// To run under WSL (Windows Subsystem for Linux), may have to elevate
// with sudo if the bind fails.

// LinuxTinyServer does not look for default index.htm or similar
// files.  If it receives a GET request on a directory, it will refuse
// it, returning an HTTP 403 error, access denied.

// It also does not support HTTP Connection: keep-alive requests and
// will close the socket at the end of each response.  This is a
// perf issue, forcing the client browser to reconnect for each
// request and a candidate for improvement.


#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <iostream>
// #include <string.h>
// #include <string>
#include <cassert>
#include "../utils/searchstring.h"
#include "../utils/vector.h"
#include "../ranking/driver.h"
// using namespace std;


 // The constructor for any plugin should set Plugin = this so that
 // LinuxTinyServer knows it exists and can call it.

#include "Plugin.h"
PluginObject *Plugin = nullptr;
const int npos = -1;


// Root directory for the website, taken from argv[ 2 ].
// (Yes, a global variable since it never changes.)

char *RootDirectory;


//  Multipurpose Internet Mail Extensions (MIME) types

struct MimetypeMap
   {
   const char *Extension, *Mimetype;
   };

const MimetypeMap MimeTable[ ] =
   {
   // List of some of the most common MIME types in sorted order.
   // https://developer.mozilla.org/en-US/docs/Web/HTTP/Basics_of_HTTP/MIME_types/Complete_list_of_MIME_types
   ".3g2",     "video/3gpp2",
   ".3gp",     "video/3gpp",
   ".7z",      "application/x-7z-compressed",
   ".aac",     "audio/aac",
   ".abw",     "application/x-abiword",
   ".arc",     "application/octet-stream",
   ".avi",     "video/x-msvideo",
   ".azw",     "application/vnd.amazon.ebook",
   ".bin",     "application/octet-stream",
   ".bz",      "application/x-bzip",
   ".bz2",     "application/x-bzip2",
   ".csh",     "application/x-csh",
   ".css",     "text/css",
   ".csv",     "text/csv",
   ".doc",     "application/msword",
   ".docx",    "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
   ".eot",     "application/vnd.ms-fontobject",
   ".epub",    "application/epub+zip",
   ".gif",     "image/gif",
   ".htm",     "text/html",
   ".html",    "text/html",
   ".ico",     "image/x-icon",
   ".ics",     "text/calendar",
   ".jar",     "application/java-archive",
   ".jpeg",    "image/jpeg",
   ".jpg",     "image/jpeg",
   ".js",      "application/javascript",
   ".json",    "application/json",
   ".mid",     "audio/midi",
   ".midi",    "audio/midi",
   ".mpeg",    "video/mpeg",
   ".mpkg",    "application/vnd.apple.installer+xml",
   ".odp",     "application/vnd.oasis.opendocument.presentation",
   ".ods",     "application/vnd.oasis.opendocument.spreadsheet",
   ".odt",     "application/vnd.oasis.opendocument.text",
   ".oga",     "audio/ogg",
   ".ogv",     "video/ogg",
   ".ogx",     "application/ogg",
   ".otf",     "font/otf",
   ".pdf",     "application/pdf",
   ".png",     "image/png",
   ".ppt",     "application/vnd.ms-powerpoint",
   ".pptx",    "application/vnd.openxmlformats-officedocument.presentationml.presentation",
   ".rar",     "application/x-rar-compressed",
   ".rtf",     "application/rtf",
   ".sh",      "application/x-sh",
   ".svg",     "image/svg+xml",
   ".swf",     "application/x-shockwave-flash",
   ".tar",     "application/x-tar",
   ".tif",     "image/tiff",
   ".tiff",    "image/tiff",
   ".ts",      "application/typescript",
   ".ttf",     "font/ttf",
   ".vsd",     "application/vnd.visio",
   ".wav",     "audio/x-wav",
   ".weba",    "audio/webm",
   ".webm",    "video/webm",
   ".webp",    "image/webp",
   ".woff",    "font/woff",
   ".woff2",   "font/woff2",
   ".xhtml",   "application/xhtml+xml",
   ".xls",     "application/vnd.ms-excel",
   ".xlsx",    "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",
   ".xml",     "application/xml",
   ".xul",     "application/vnd.mozilla.xul+xml",
   ".zip",     "application/zip"
   };

inline void reverse_string(string& str) 
{
if (str.empty()) return;
size_t left = 0;
size_t right = str.size() - 1;
while (left < right) 
   {
      // Swap characters at left and right positions
      char temp = str[left];
      str[left] = str[right];
      str[right] = temp;
      // Move inward from both ends
      ++left;
      --right;
   }
}   

string to_string(int n)
   {
   if (n == 0) return "0";
   bool negative = n < 0;
   string temp;
   if (negative) n = -n;
   while (n > 0) 
      {
      temp.push_back( (char)(n % 10 + '0') );
      n /= 10;
      }
   if (negative) 
      temp.push_back('-');
   reverse_string( temp );
   return temp;
   }
   



const char *Mimetype( const string filename )
   {
   // TO DO: if a matching a extentsion is found return the corresponding
   // MIME type.

   // Anything not matched is an "octet-stream", treated
   // as an unknown binary, which can be downloaded.

   size_t dotPos = filename.find_last_of('.');
   if (dotPos != npos) {
      string ext = filename.substr(dotPos);
      // Binary search through MimeTable
      int left = 0;
      int right = sizeof(MimeTable) / sizeof(MimeTable[0]) - 1;

      while (left <= right) {
         int mid = (left + right) / 2;
         int comp = strcmp(ext.c_str(), MimeTable[mid].Extension);
         
         if (comp == 0)
            return MimeTable[mid].Mimetype;
         else if (comp < 0)
            right = mid - 1;
         else
            left = mid + 1;
      }

   }
   return "application/octet-stream";
   }


int HexLiteralCharacter( char c )
   {
   // If c contains the Ascii code for a hex character, return the
   // binary value; otherwise, -1.

   int i;

   if ( '0' <= c && c <= '9' )
      i = c - '0';
   else
      if ( 'a' <= c && c <= 'f' )
         i = c - 'a' + 10;
      else
         if ( 'A' <= c && c <= 'F' )
            i = c - 'A' + 10;
         else
            i = -1;

   return i;
   }


string UnencodeUrlEncoding( string &path )
   {
   // Unencode any %xx encodings of characters that can't be
   // passed in a URL.

   // (Unencoding can only shorten a string or leave it unchanged.
   // It never gets longer.)

   const char *start = path.c_str( ), *from = start;
   string result;
   char c, d;


   while ( ( c = *from++ ) != 0 )
      if ( c == '%' )
         {
         c = *from;
         if ( c )
            {
            d = *++from;
            if ( d )
               {
               int i, j;
               i = HexLiteralCharacter( c );
               j = HexLiteralCharacter( d );
               if ( i >= 0 && j >= 0 )
                  {
                  from++;
                  result += ( char )( i << 4 | j );
                  }
               else
                  {
                  // If the two characters following the %
                  // aren't both hex digits, treat as
                  // literal text.

                  result += '%';
                  from--;
                  }
               }
            }
         }
      else
         result += c;

   return result;
   }


bool SafePath( const char *path )
   {
   // The path must start with a /.
   if ( *path != '/' )
      return false;

   // TO DO:  Return false for any path containing ..
   // segments that attempt to go higher than the root
   // directory for the website.
   const char *p = path;
   int depth = 0;

   while (*p) {
      if (p[0] == '.' && p[1] == '.' && (p[2] == '/' || p[2] == '\0')) {
         depth--;
         if (depth < 0)
            return false;
         p += 2;
      }
      else if (*p == '/') {
         depth++;
         p++;
      }
      else {
         p++;
      }
   }


   return true;
   }




off_t FileSize( int f )
   {
   // Return -1 for directories.

   struct stat fileInfo;
   fstat( f, &fileInfo );
   if ( ( fileInfo.st_mode & S_IFMT ) == S_IFDIR )
      return -1;
   return fileInfo.st_size;
   }


void AccessDenied( int talkSocket )
   {
   const char accessDenied[ ] = "HTTP/1.1 403 Access Denied\r\n"
         "Content-Length: 0\r\n"
         "Connection: close\r\n\r\n";

   std::cout << accessDenied;
   send( talkSocket, accessDenied, sizeof( accessDenied ) - 1, 0 );
   }

   
void FileNotFound( int talkSocket )
   {
   const char fileNotFound[ ] = "HTTP/1.1 404 Not Found\r\n"
         "Content-Length: 0\r\n"
         "Connection: close\r\n\r\n";

   std::cout << fileNotFound;
   send( talkSocket, fileNotFound, sizeof( fileNotFound ) - 1, 0 );
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
      vector<string> urls = results(query);


      string resultsHtml;
      for (int i = 0; i < urls.size(); ++i) {
           // example
          resultsHtml += (string)"<li><a href=\"" + urls[i] + 
          (string)"\">Result " + to_string(i) + (string)" for '" + query + (string)"': "+ urls[i] + (string)" </a></li>\n";
      }
      
      // Replace {{results}} in template
      size_t resultsPos = templateHtml.find("{{results}}");
      if (resultsPos != npos) {
          string before = templateHtml.substr(0, resultsPos);
          string after = templateHtml.substr(resultsPos + 11, templateHtml.size() - (resultsPos + 11)); // 11 is length of {{results}}
          templateHtml = before + resultsHtml + after;
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
