/*
Chris DeStefano
CIS - 3207 Sec 003
Dr. Tamer Aldwairi 
11-03-2019
Project 3
*/
#include <fstream>
#include <iostream> 
#include <string> 
#include <cstring>  
#include <iomanip> 
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <bits/stdc++.h>
#include <vector>
#include <stdio.h> 
#include <string.h> 
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <queue> 
#include <pthread.h> 


using namespace std;

#define DEFAULT_DICTIONARY "words-2.txt" 
#define MAX_JOBS 50 
#define MAX_CLIENTS 50 
#define MAX_HANDLERS 30
#define MAX_WRITERS 30 
#define BUFF_SIZE 4096



/* internet address structure 
struct in_addr {
unsigned int s_addr;
};


/* 
Internet-style socket address structure 
struct sockaddr_in {
unsigned short sin_family; /* Address family (always AF_INET) 
unsigned short sin_port; /* Port number in network byte order 
struct in_addr sin_addr; /* IP address in network byte order 
unsigned char sin_zero[8]; /* Pad to sizeof(struct sockaddr) 
};
*/


struct word{
string wrd_to_check; 
int size; 

};

vector <string> dict;

pthread_t handler_pool[MAX_HANDLERS]; 
pthread_t writer; 


pthread_mutex_t mute = PTHREAD_MUTEX_INITIALIZER; 
pthread_cond_t cond_Var = PTHREAD_COND_INITIALIZER;

pthread_mutex_t log_mute = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t log_cond = PTHREAD_COND_INITIALIZER;


queue <int> work;
queue <string> log_entrys; 

ofstream file;
 

/*
This function accepts a vector of dictionary words and a word the user wishes to check
It returns a bool true if the user's word is spell correctly and false other wise
*/
bool check_words (vector <string> check, string word){
    bool correct= false; 
    
    for (int i = 0; i<=check.size()-1; i++){
        if (check[i].compare(word)==0){
            correct = true; 
            return correct; 
        }
        else {
            correct = false; 
        }
    }

return correct; 
}


string to_string(char c[], int c_size){

string s; 
int i =0; 
    
    for(i = 0; i <= BUFF_SIZE; i++){
        if(c[i] != '\0')
            if(c[i] == '\r')
                return s; 
            s += c[i]; 
    
    }


    return s; 
}

void write_logfile(string s){

 
file.open("Log.txt", std::fstream::out | std::fstream::app);
    if(!file){
        cout << "error opening log file" << endl; 
    }

file << s << "\n"; 
file.close(); 
}

void handle_client(int client_sock){
    

//cout << "servicing client socket " << client_sock <<endl; 
    char buff[BUFF_SIZE]; 
    vector <word> rec_words; 
    string dc = " Disconnected";
    string log_disc = "Client " + to_string(client_sock) + dc;
    word temp; 
    int bytes_Rec;

    memset(buff,0, BUFF_SIZE); 

    //accept a stream from a client 
    while (bytes_Rec = recv(client_sock, buff, BUFF_SIZE, 0) >0){

        
    string log_recv; 
    string sent = " sent "; 
    //print the stream to the server 
    cout << "Recieved: " << string(buff, BUFF_SIZE); 
    
    
    
    string string_word = to_string(buff, BUFF_SIZE); 
    log_recv = "Client " + to_string(client_sock) + sent + string_word; 
    pthread_mutex_lock(&log_mute);
    log_entrys.push(log_recv);
    pthread_mutex_unlock(&log_mute);
    pthread_cond_signal(&log_cond);

    string right = " is spelled correctly";
    string wrong = " is spelled inncorrectly"; 
    int bytes_send= 0; 
    string log_check;



    //send the stream back 
    if (string_word.compare("xx")==0){
        close(client_sock); 
        pthread_mutex_lock(&log_mute);
        log_entrys.push(log_disc);
        pthread_mutex_unlock(&log_mute);
        pthread_cond_signal(&log_cond);

    }
    
    
    if (check_words(dict, string_word )){
        string_word = string_word + right;
        char c_arr [string_word.size()+1]; 
        strcpy(c_arr, string_word.c_str()); 
        c_arr[string_word.size()+1] = '\n'; 
        bytes_send=sizeof(c_arr);
         
        log_check = string_word; 
        pthread_mutex_lock(&log_mute);
        log_entrys.push(log_check);
        pthread_mutex_unlock(&log_mute);
        pthread_cond_signal(&log_cond);
        cout << string_word << endl; 
        send(client_sock,c_arr, bytes_send+1, 0);


    }
    else{
        string_word = string_word + wrong;
        char c_arr [string_word.size()+1];
        strcpy(c_arr, string_word.c_str()); 
        c_arr[string_word.size()+1] = '\n'; 
        bytes_send=sizeof(c_arr);
        
        log_check = string_word; 
        pthread_mutex_lock(&log_mute);
        log_entrys.push(log_check);
        pthread_mutex_unlock(&log_mute);
        pthread_cond_signal(&log_cond); 

        send(client_sock,c_arr, bytes_send+1, 0);
    }



     
}
 
    if (bytes_Rec == -1){
            cout << "error connecting" << endl; 
         }

        if (bytes_Rec == 0){
            cout << "Client " << client_sock << " Disconnected"  << endl;
            pthread_mutex_lock(&log_mute);
            log_entrys.push(log_disc);
            pthread_mutex_unlock(&log_mute);
            pthread_cond_signal(&log_cond);

        }




}



void * handler_func(void * arg){
   // cout << "entered handler function" <<endl;
    while(true){ //keep running forever 
        int client = 0; 
        pthread_mutex_lock(&mute); // acquire the lock incase theirs work to be done 
        //cout << "Lock acquired in handle"<<endl; 

        if(work.empty()){ 
                pthread_cond_wait(&cond_Var, &mute);
                //cout << "Thread blocked and lock revoked in handle" << endl; 
                client = work.front();
                //cout << client << " removed from queue"<<endl;
                work.pop(); 
                
        }   

        if (pthread_mutex_unlock(&mute)==0){
            //cout << "Lock revoked in handle"<<endl; 
        } 

        if(client != 0){ // if the queue is not empty aka work to be done  
            handle_client(client);
        }
            
            
        
        

        }
        
    }
   

void *writer_func(void*arg){
    while(true){
        string entry = ""; 
        pthread_mutex_lock(&log_mute);
       // cout << "lock acquired in writer" << endl;

    if(log_entrys.empty()){ 
                
                pthread_cond_wait(&log_cond, &log_mute);
                //cout << "Thread blocked and lock revoked in handle" << endl; 
                entry = log_entrys.front();
                //cout << client << " removed from queue"<<endl;
                log_entrys.pop(); 
                
        }   

        if (pthread_mutex_unlock(&log_mute)==0){
           // cout << "Lock revoked in writer"<<endl; 
        } 

        if(entry.compare("") != 0){ // if the queue is not empty aka work to be done  
            write_logfile(entry);
        }  
    }
}




/*

MAIN

*/
int main (int argc, char* argv[]){
 


ifstream File;  
//if you input a dictionary file then open it instead of the standard dict file
     
        if (argc == 2){ // if we enter a new file 
        File.open(argv[1]); //open it 
            if (!File){ //if their is an error opening it open the defult file instead 
                cerr << "Can't open file" << argv[1] << endl; 
                File.open(DEFAULT_DICTIONARY); 
                    if (!File){
                        cerr << "Can't open file words-2.txt" << endl; 
                    } 
            }
        }
        else {
        File.open(DEFAULT_DICTIONARY); 
            if (!File){
                cerr << "Can't open file words-2.txt" << endl;   
            }
            
        }

// load the file into a data structer 
 
string str; 

    while(getline(File, str)){
        dict.push_back(str); 
    }

File.close();






//Network Stuff 
//Creating a sokcet 
int listener =0;
listener = socket(AF_INET, SOCK_STREAM, 0); 

    if (listener == -1){
        cout << "Error creating Socket" << endl;
    }

//Bind socket with IP/port#

sockaddr_in hint;
hint.sin_family = AF_INET;
hint.sin_port = htons(54000); 

//everytime we interact with the port # use htons
//HOME ADDRESS 127.0.0.1

inet_pton(AF_INET, "0.0.0.0", &hint.sin_addr);  // 0.0.0.0 means any IP 

//Bind the socket to the IP &hint 
    if(bind(listener, (struct sockaddr*)&hint, sizeof(hint)) == -1){
        cerr << "error bind to IP/port";
        return -1; 
    }

//Mark our socket for listening 
    if (listen(listener, SOMAXCONN) == -1){
        cerr << "Error Listening" << endl; 
    }

//cout << "Listening" << endl; 

//Create a socket address for the Client 
sockaddr_in client;
socklen_t clientSize = sizeof(client);
char host[NI_MAXHOST];
char svc[NI_MAXSERV]; 
memset(host, 0, NI_MAXHOST); 
memset(svc, 0, NI_MAXSERV); 





//hold the incoming client 
int client_Socket; 

//Create our threads 
for(int i=0; i<=MAX_HANDLERS; i++){
pthread_create(&handler_pool[i], NULL, handler_func, NULL); 
//cout << "Thread " << i << " created" << endl; 
}
pthread_create(&writer, NULL, writer_func, NULL); 

//reset log file 
file.open("Log.txt", std::fstream::trunc);
file.close();  

//variables for log file 
string c = "Client "; 
string hc = " has connected"; 
string sock = "";
string exit =" "; 

int bytesToSend; 
string welcome = "Welcome to the Super Spellchecker 5000 \nEnter a word to see if it's spelled right\nEnter xx to quit";
char c_arr [welcome.size()+1]; 
strcpy(c_arr, welcome.c_str()); 
c_arr[welcome.size()+1] = '\n'; 
bytesToSend=sizeof(c_arr);




/*

server loop 

*/ 
cout << "server listening" << endl; 
    while(true){
       string s;
       client_Socket = accept(listener, (struct sockaddr*) &client, &clientSize);
       send(client_Socket,c_arr, bytesToSend+1, 0);
            if(client_Socket == -1){
                cerr << "error connecting to client" << endl; 
            }
        
        cout << "client " << client_Socket <<" has connected" << endl; 
        s = c + to_string(client_Socket) + hc; 
        pthread_mutex_lock(&log_mute);
        log_entrys.push(s);
        pthread_mutex_unlock(&log_mute);
        pthread_cond_signal(&log_cond);
            
        
        //when a new connection comes in we add it the que of work

        if(pthread_mutex_lock(&mute) == 0){
            //cout << "lock acquired in server"<<endl;
        } 
        work.push(client_Socket);
        //cout << "client " << client_Socket << " has been added to the queue"<<endl; 

        if(pthread_mutex_unlock(&mute) == 0){
           // cout << "lock revoked in server"<<endl;
        } 
        pthread_cond_signal(&cond_Var);
        
        
        

        
      
       
    }




close(listener);





return 0;
}



