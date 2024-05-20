#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <pthread.h>

#define MAXLINE 70

//Default number of sources and clients per source is 10 
//Since this are  variables that are only altered once and need to be used in lots of functions
//There should be no problem in using them as global variables to reduce the function typing work

int max_sources = 0;
int max_subs_per_source = 0;

typedef struct PDU1{
    char Fonte_Tipo[10];
    char Identificador[10];
    char Periodo[10];
    char Frequencia[10];
    char Num_Amostras[10];
    char Val_amostra[10];
    char Val_amostra_cod[10];
    char Periodo_Max[10];
} PDU1;

typedef struct Source{
    char ID[10];
    int *client_socket_fd;
    int last_time_received;
    char last_message_received[10];
} Source;


//Just helps initilize the client storage in each source
void Initalize_Storage(Source *Storage_pointer){
    for(int i = 0; i<max_sources; i++){
        Storage_pointer[i].client_socket_fd = (int *)malloc(sizeof(int)*max_subs_per_source);
        memset(Storage_pointer[i].client_socket_fd,0,max_subs_per_source);
    }
}

/*As the name says, it simply adds sources
Possible returns->  2 -> The source is already registered, last message received and it's time of arrival updated
                    1 -> The source was add sucessfully, we also register the message and time received from it
                    0 -> The source was not added by lack of space
                    */
int add_source_or_update_message(Source *Strg_ptr,char Source_ID[10],char amostra_cod[10]){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    for(int i = 0; i<max_sources ;i++){
        if(strcmp(Strg_ptr[i].ID,Source_ID) == 0){
            //printf("Hello %d\n",i);
            //sleep(2);
            memmove(Strg_ptr[i].last_message_received,amostra_cod,sizeof(char)*10);
            Strg_ptr[i].last_time_received = tv.tv_sec;
            return 2;
        }
        if(atoi(Strg_ptr[i].ID) == 0){
            //printf("No Hello %d\n",i);
            memmove(Strg_ptr[i].ID,Source_ID,sizeof(char)*10);
            memset(Strg_ptr[i].client_socket_fd,0,max_subs_per_source);
            memmove(Strg_ptr[i].last_message_received,amostra_cod,sizeof(char)*10);
            Strg_ptr[i].last_time_received = tv.tv_sec;
            return 1;
        }
    }
    return 0;
}

/*When a source stops contacting the multimedia server or informs it that it will be closing
The source in question should be removed and it's element in the storage reseted so as to permit
The addition of new sources*/
void remove_source(Source *Strg_ptr,char Source_ID[10],int socket_fd,int clients_source){
    for(int  i = 0; i<max_sources ;i++){
        if(atoi(Strg_ptr[i].ID) == 0){
            if(strcmp(Strg_ptr->ID,Source_ID) == 0){
                memset(Strg_ptr->ID, 0,10);
                memset(Strg_ptr->client_socket_fd,0,clients_source);
            }
        }
    }
}

/*Registers a client as a source subscriber
If the source be maxed out in terms of subscribers the client must be informed
1-> Client was subscribed
2-> Maxed out source*/
int subscribe_to_source(Source *Strg_ptr,char Source_ID[10],int client_socket){
    for(int i = 0; i < max_sources; i++){
        if(strcmp(Strg_ptr[i].ID,Source_ID) == 0){
            for(int clt_counter = 0; clt_counter < max_subs_per_source; clt_counter++){
                if(Strg_ptr[i].client_socket_fd[clt_counter] == 0){
                    Strg_ptr[i].client_socket_fd[clt_counter] = client_socket;
                    return 1;
                }
            }
        }
    }
    return 0;
}

/*Since the message arrives to the server as a single string this function
retrieves the data between each "|".
When the entire message is processes a PDU1 with the retrieved data is stored in a array that stores the recently arrived messages
Possible improvement-> Use of a pointer to the storage array so as to not need to any returns*/
PDU1 source_message_splitter(char *message){
    char buffer[10];
    int word_size = 0;
    PDU1 new_PDU;
    int what_section = 0;
    for(int i = 0; i != strlen(message);i++){
        if(message[i] != '|'){
            buffer[word_size] = message[i];
            word_size++;
        }
        if(message[i] == '|'){
            switch(what_section){
                case 0:
                    memcpy(new_PDU.Fonte_Tipo,buffer,sizeof(char)*word_size);
                    break;
                case 1:
                    memcpy(new_PDU.Identificador,buffer,sizeof(char)*word_size);
                    new_PDU.Identificador[word_size]='\0';
                    break;
                case 2:
                    memcpy(new_PDU.Val_amostra,buffer,sizeof(char)*word_size);
                    new_PDU.Val_amostra[word_size]='\0';
                    break;
                case 3:
                    memcpy(new_PDU.Val_amostra_cod,buffer,sizeof(char)*word_size);
                    new_PDU.Val_amostra_cod[word_size]='\0';
                    break;
                case 4:
                    memcpy(new_PDU.Periodo,buffer,sizeof(char)*word_size);
                    new_PDU.Periodo[word_size]='\0';
                    break;
                case 5:
                    memcpy(new_PDU.Frequencia,buffer,sizeof(char)*word_size);
                    new_PDU.Frequencia[word_size]='\0';
                    break;
                case 6:
                    memcpy(new_PDU.Num_Amostras,buffer,sizeof(char)*word_size);
                    new_PDU.Num_Amostras[word_size]='\0';
                    break;
                case 7:
                    memcpy(new_PDU.Periodo_Max,buffer,sizeof(char)*word_size);
                    new_PDU.Periodo_Max[word_size]='\0';
                    break;
                default:
                    //End of message
                    break;
            }
            word_size = 0;
            what_section++;
        }
    }
    return new_PDU;
}

//Returns the message type
int type_getter(char* buffer){
    int char_counter = 0;
    char type_buffer[10];
    type_buffer[10] = '\0';
    while(buffer[char_counter] != '|'){
        type_buffer[char_counter] = buffer[char_counter];
        char_counter++;
    }
    int result = atoi(type_buffer);;
    return result;
}

int main(int argc,char* argv[]) { //Args necessários, IP_servidor,port, número máximo de fontes,número máximo de subscritores por fontes

    int PORT = 0;
    char *Server_Ip = (char *)malloc(sizeof(char)*15);

    if(argc > 4){
        if(strlen(argv[1]) != 14){
            Server_Ip = realloc(Server_Ip, strlen(argv[1])+1);
            Server_Ip[strlen(argv[1])+1]='\0';
            memcpy(Server_Ip,argv[1],strlen(argv[1]));
        }
        PORT = atoi(argv[2]);
        max_sources = atoi(argv[3]);
        max_subs_per_source = atoi(argv[4]);
    }

    Source Source_Storage[max_sources];
    Source *Source_Storage_ptr = Source_Storage;

    int UDP_Socket_fd;
    char buffer[MAXLINE];
    buffer[MAXLINE] = '\0';
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_size = sizeof(client_addr);
    char s[INET_ADDRSTRLEN];

    // Creating socket file descriptor
    if ((UDP_Socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Failed to create Socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    // Filling server information
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(Server_Ip);
    server_addr.sin_port = htons(PORT);
    sleep(1);
    // Bind the socket with the server address
    if (bind(UDP_Socket_fd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    int new_connection_fd = 0;

    /*Types of Message needed:
        -> 1, Source->SM comm 
        -> 2  Client->SM Source Listing Request
        -> 3  Client->SM Source Subscription*/
    int type_source = 0; 

    PDU1 PDU_1;
    PDU_1.Fonte_Tipo[10] = '\0';
    PDU_1.Identificador[10] = '\0';
    PDU_1.Periodo[10] = '\0';
    PDU_1.Frequencia[10] = '\0';
    PDU_1.Num_Amostras[10] = '\0';
    PDU_1.Val_amostra[10] = '\0';
    PDU_1.Val_amostra_cod[10] = '\0';
    PDU_1.Periodo_Max[10] = '\0';

    //PDU1 PDU_1_Array[max_sources];

    /*If the source_storage is filled up we send one warning turning this variable to 1 so as to not hinder the server perfomance with useless multiple warnings,
     but if a flag with the value of 1 is received it is turned back to 0 since a new source was added */
    int full_source_storage_Warning = 0; 
    int source_flag = 0;

    char Source_Listing_Message[10*max_sources+max_sources];//New line space is the max_sources added

    Initalize_Storage(Source_Storage_ptr);

    while(1) {
        //sin_size = sizeof client_addr_storage;
        int len, n;
        len = sizeof(client_addr);
        n = recvfrom(UDP_Socket_fd,(char *)buffer ,MAXLINE, MSG_WAITALL,(struct sockaddr *)&client_addr ,&len); //
        buffer[n] = '\0';
        type_source = type_getter(buffer);
        switch(type_source){
            case 1:
                PDU_1 = source_message_splitter(buffer);
                source_flag = add_source_or_update_message(Source_Storage_ptr,PDU_1.Identificador,PDU_1.Val_amostra_cod);
                if(source_flag == 0 && full_source_storage_Warning == 0){
                    full_source_storage_Warning = 1;
                    perror("Limit of Sources hitted");
                }
                if(source_flag == 1 && full_source_storage_Warning == 1){
                    full_source_storage_Warning = 0;
                }
                break;
            case 2: //Client->SM Source Listing Request
                for(int source_counter = 0; source_counter<max_sources ; source_counter++){
                    if(atoi(Source_Storage[source_counter].ID) != 0){
                        strcat(Source_Listing_Message,Source_Storage[source_counter].ID);
                        strcat(Source_Listing_Message,"\n");
                    }
                }
                //printf("message->%s\n",Source_Listing_Message);
                if (sendto(UDP_Socket_fd, Source_Listing_Message, strlen(Source_Listing_Message), 0, (struct sockaddr * ) &client_addr, sizeof(client_addr)) < 0) {
                        perror("Data couldn't be sent to the server");
                        break;
                    }
                memset(Source_Listing_Message,'\0',10*max_sources+max_sources);
            case 3: //Client->SM Source Subscription

                break;
            default:
                perror("Non valid type of case received, discarded");
                break;
        }
        
        memset(buffer,'\0',sizeof(buffer));
        //printf("Client: %s\n", buffer);
    }

    return 0;
}