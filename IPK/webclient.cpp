/* 
 * File: webclient.cpp
 * Author: Maros Kopec, xkopec44@stud.fit.vutbr.cz
 * About: Simple web clinet using socet API, that dowloads item from web server
 *        via HTTP 1.0 or HTTP 1.1
 */
 
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <regex>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

#define BUFFSIZE 1024
using namespace std;

typedef struct SHost
{
    int port;
    string url;
    string fpath;
} TSHost;


/* 
 * Displays system call error message.
 */
void handle_error(const char *msg)
{
    perror(msg);
    exit(1);
}


/*
 * Print error to stderr and return error code
 */
void print_error(const char *msg, int errcode) {
    cerr << msg;
    exit(errcode);
}

/*
 * Convert number to string
 * Author: cMinor
 * Copied: 2016-04-03
 * Online at: StackOverflow
 *      /questions/12975341/to-string-is-not-a-member-of-std-says-so-g
 *
 * G++ on server EVA do not know std::to_string() function.
 */
namespace patch
{
    template < typename T > std::string to_string( const T& n )
    {
        std::ostringstream stm ;
        stm << n ;
        return stm.str() ;
    }
}


/*
 * Create socet, connect to server and send HTTP request
 */
int my_connect(int &sockfd, TSHost &host, int ver, int redir) {
    int bytestx;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    
    // redirect counter
    static int redirect_count = 0;
    if (redirect_count > 5)
    {
        return 1;
    }
    // cout << "My connect: " << redirect_count << endl;
    
    //cout << host.url;
//  Create socet 
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        handle_error("ERROR opening socket");
        
    server = gethostbyname(host.url.c_str());
    
    if (server == NULL) 
        print_error("ERROR, no such host\n", 1);

//  Connect
    bzero((char *) &serv_addr, sizeof(serv_addr));

    // set serv_addr structure
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
            (char *)&serv_addr.sin_addr.s_addr, 
            server->h_length);
    serv_addr.sin_port = htons(host.port);
    /* htons converts the unsigned short integer hostshort 
        from host byte order to network byte order. */

    // connecting  
    if (connect(sockfd, (const struct sockaddr *) &serv_addr, 
                sizeof(serv_addr)) != 0)
        handle_error("ERROR connecting");

//  Send HTTP request 
    string request = "GET " + host.fpath + " HTTP/1." + patch::to_string(ver) +"\r\n"
                        + "Host: " + host.url + "\r\n"
                        + "Connection: close\r\n\r\n";

    //cout << request;
    
    bytestx = send(sockfd, request.c_str(), request.size(), 0);
    if (bytestx < 0)
        handle_error("ERROR in sendto");
        
    // increment redirect counter
    if (redir) // if flag is set to true
        redirect_count++;
    
    return 0;
}


/*
 * Convers string to integer
 */
int str2int(const string &str) {
    int res;
    
    stringstream convert;
    convert << str;
    convert >> res;
    
    return res;
}


/*
 * Replace spaces with %20 and tilds with %7E
 */
void replace_chars(string &str) {
    unsigned int space_pos;
    while (str.find(' ') != string::npos) {
        space_pos = str.find(' ');
        str.erase(space_pos, 1);
        str.insert(space_pos, "%20");
    }
    
    unsigned int tild_pos;
    while (str.find('~') != string::npos) {
        tild_pos = str.find(' ');
        str.erase(tild_pos, 1);
        str.insert(tild_pos, "%7E");
    }
}


/*
 * Recive first response with header, get HTTP version and respond code
 */
int check_connection(int sockfd, string &response, char *buffer) {
    smatch matched;
    int bytesrx, version, respond_code;
    
    // recive response
    bytesrx = recv(sockfd, buffer, BUFFSIZE, 0);
    response.append(buffer, bytesrx);
    bzero(buffer, BUFFSIZE);
    // cout << buffer << endl;

    // get HTTP version
    regex regx ("HTTP/1.(1|0)"); // second match is version   
    regex_search (response, matched, regx);
    version = str2int(matched[1]);

    // get response code
    regx = "HTTP/1.[[:digit:]] ([[:digit:]]{3})";
    regex_search (response, matched, regx);
    respond_code = str2int(matched[1]);
    
    return (version*1000 + respond_code);
}


// ============================================================================
// ================================= MAIN =====================================
// ============================================================================
int main(int argc, char *argv[]) {
    
    TSHost host;
    static string urls;

    ofstream fp;
    string filename;
    smatch matched;
    int sockfd, bytesrx;

    
    if (argc != 2) { 
        print_error("Wrong arguments!\n", EXIT_FAILURE);
    }

    
// ########################### Regex matching #################################
    string arg (argv[1]);
    arg.erase(0, 7); // erase http://
    regex regx ("/.*(:[[:digit:]]+)?");
    
    // parse url
    if (!  regex_search (arg, matched, regx) ) { // no match found
        filename = "";
        host.fpath = "/";
        host.url = arg;
    }
    else {
        host.fpath = matched[0];
        filename = host.fpath.substr(host.fpath.find_last_of("/")+1);
        replace_chars(host.fpath);    
        host.url = matched.prefix().str();
        }
    
    // find port
    regx = ":[[:digit:]]+";
    if ( ! regex_search (host.url, matched, regx) ) { // no match found
        host.port = 80;
    }
    else { // port is specified
        string str_port = matched[0];
        str_port.erase(0, 1);
        
        // convert string to number
        stringstream convert(str_port);
        if ( !(convert >> host.port) )
            host.port = 80;
            
        // the port is specified in url, need to be cut out
        host.url = host.url.erase(host.url.find_last_of(":"));
    }

// ######################### Recive HTTP response #############################
char buffer[BUFFSIZE];
string response = "";
int version = 0;
int chunked = 0;
int respond_code;

// create socket, connect and send HTTP request
// =============================== IMPORTANT ==================================
if ( my_connect(sockfd, host, 1, 1) ) {
    close(sockfd);
    print_error("Maximum number of redirections reached.\n", 1);
}
// =============================== IMPORTANT ==================================

if ( (respond_code = check_connection(sockfd, response, buffer)) > 1000 ) {
    version = 1;
    respond_code -= 1000; 
}
else {
    version = 0;
}
//cout << response;

// try again with HTTP 1.0
if (respond_code >= 400) {
    close(sockfd);
    if ( my_connect(sockfd, host, 0, 0) )  { // redir flag set to 0
        close(sockfd);
        print_error("Maximum number of redirections reached.\n", 1);
    }
    
    response.clear();
    if ( (respond_code = check_connection(sockfd, response, buffer)) > 1000 ) {
        version = 1;
        respond_code -= 1000; 
    }
    else {
        version = 0;
    }
}
//cout << response;

// still bad code
if (respond_code >= 400) {    
    string msg = "ERROR " + patch::to_string(respond_code) + " code.\n";
    close(sockfd);
    print_error(msg.c_str(), respond_code);
}


// ################################# Redirect #################################
if (respond_code == 301 || respond_code == 302) {
    // get new location
    regex regx ("Location: (.*)"); 
    regex_search (response, matched, regx);
    host.url = matched[1]; 
    host.url.erase(0, 7); // erase http://
    if (host.url[host.url.length()-1] == '/')
        host.url.erase(host.url.length()-1, 1); // erase last /
    //cout << ">" << host.url << "<" << endl;
    
    while ( ! my_connect(sockfd, host, version, 1) ) { // not > 5 redirs
        // check response code
        response.clear();
        if ( (respond_code = check_connection(sockfd, response, buffer)) > 1000 ) {
            version = 1;
            respond_code -= 1000; 
        }
        else {
            version = 0;
        }
        //cout << response;
        
        // error code
        if (respond_code >= 400) {    
            string msg = "ERROR " + patch::to_string(respond_code) + " code.\n";
            print_error(msg.c_str(), respond_code);
        }
        
        // stop redirecting
        if (respond_code != 301 && respond_code != 302)
            break; 
            
       // get new location
        regex regx ("Location: (.*)"); 
        regex_search (response, matched, regx);
        host.url = matched[1]; 
        host.url.erase(0, 7); // erase http://
        if (host.url[host.url.length()-1] == '/')
            host.url.erase(host.url.length()-1, 1); // erase last /
        
        
        close(sockfd);
    }
}

//cout << response;

// ############################# Handle output ################################
//  All good at this point

// if HTTP 1.1, check for chunked
if (version) {
    // npos is largest possible value of unsigned = -1
    if (response.find("Transfer-Encoding: chunked") != string::npos)
        chunked = 1;
}

// remove header
response = response.substr(response.find("\r\n\r\n") + 4);

// get the rest of the file
while ( (bytesrx = recv(sockfd, buffer, BUFFSIZE, 0)) > 0 ) {
    response.append(buffer, bytesrx);
    bzero(buffer, BUFFSIZE);
}
if (bytesrx < 0) 
    handle_error("ERROR in recvfrom");
// close socet
close(sockfd);
    
// remove chunk information
string str_chunk ("");
unsigned long int chunk;
unsigned long int chunk_pos = 0;

if (chunked) {
    do {
        str_chunk.clear();
        for (int i=chunk_pos; response[i] != '\r'; i++) {
            str_chunk.push_back(response[i]);
        }
        
        // conver string hex into int dec
        stringstream convert;
        convert << std::hex << str_chunk;
        convert >> chunk;
        
        // erase hex + CRLF
        response.erase(chunk_pos, str_chunk.length()+2);
        
        chunk_pos += chunk;
        
        // erase closing CRLF
        if (chunk == 0)
            response.erase(response.length()-2, 2);
        else
            response.erase(chunk_pos, 2);
        
    } while (chunk > 0);
}


// ############################# Open file ####################################
if (filename.compare("") == 0) // is empty 
    // open index.html
    fp.open("index.html", ios::out);
else
    fp.open(filename.c_str(), ios::out | ios::binary);

if (fp.is_open()) 
    fp << response;

// ######################### Close file & return ###############################
fp.close();
return 0; // Succesfull execution
}