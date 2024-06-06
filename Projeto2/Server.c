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

#define MAXLINE 70 //Maximum size of the PDU1
#define MaximumNumberOfCredits 100000 //Amount of credits a client has until it is disconnected, each credit represents a message to be sent
#define Source_timeout_time 7 //Time, in seconds, that the source has to send another message to the SM until it is disconnected, we also set a timeout for the socket although a little bit higher to avoid any problems

//Default number of sources and clients per source is 10 
//Since this are  variables that are only altered once and need to be used in lots of functions
//There should be no problem in using them as global variables to reduce the function typing work

int max_sources = 0;
int max_subs_per_source = 0;
int last_source_registered_pos = 0;
int last_source_subscribed = 0;
int PORT = 0;
int testing_turned_on = 0; //If we have an extra argument in the 
char *Server_Ip;

pthread_mutex_t mutex;
//pthread_mutex_t source_checker_mutex;

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

typedef struct client{
    struct sockaddr_in client_address;
    int credits_to_disconnect;
    char ID[5];
}client;

typedef struct Source{ //Each source stores the arguments to it's own thread
    char ID[10];
    int client_count;
    struct sockaddr_in *clients_addresses;
    client *client_storage;
    PDU1 Source_PDU1;
    int last_time_received;
    char last_message_received[10];
} Source;

typedef struct { //Arguments for the thread responsible to send data to each client subscribed to a source
    int socket_fd;
    Source Sources;
    PDU1 PDU1_used;
    struct sockaddr_in server_add;
    client cliente_sub;
    int Server_Port;
} Sub_ThreadArgs;

typedef struct{ //Arguments for the request thread. We open one of those thread for each requests that arrives to the server. They are detachable so we don't need to worry about receiving lost of requests.
    int socket_to_use;
    int case_type;
    client client_used;
    int Source_ID;
    struct sockaddr_in client_addr_arg;
} Request_ThreadArgs;

Source *Source_Storage; //All of this arrays need to be access in both the thread and main thus we make them global to avoid making multiple copies in structs
int *Threaded_Sources_ID; //Since we can have race conditions we need to implement mutexes
pthread_t *Source_Threads;

//Just helps initilize the client storage in each source
void Initalize_Storage(Source *Storage_pointer){
    for(int i = 0; i<max_sources; i++){
        Storage_pointer[i].client_count = 0;
        Storage_pointer[i].clients_addresses = (struct sockaddr_in*)malloc(max_subs_per_source*sizeof(struct sockaddr_in));
        Storage_pointer[i].client_storage = (client*)malloc(max_subs_per_source*sizeof(client));
    }
}

/*As the name says, it simply adds sources
Possible returns->  2 -> The source is already registered, last message received and it's time of arrival updated
                    1 -> The source was add sucessfully, we also register the message and time received from it
                    0 -> The source was not added by lack of space
                    */
int add_source_or_update_message(Source *Strg_ptr,char Source_ID[10],char amostra_cod[10],PDU1 PDU_Used){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    int first_available_position = -1;
    for(int i = 0; i<max_sources ;i++){
        if(strcmp(Strg_ptr[i].ID,Source_ID) == 0){
            //printf("Hello %d\n",i);
            //sleep(2);
            memmove(Strg_ptr[i].last_message_received,amostra_cod,sizeof(char)*10);
            Strg_ptr[i].last_time_received = tv.tv_sec;
            if(testing_turned_on == 2){
                printf("Source %s Received Message -> %s\n",Source_ID,amostra_cod);
            }
            return 2;
        }
        if(atoi(Strg_ptr[i].ID) == 0 && first_available_position == -1){
            first_available_position = i;
        }
    }
    if(first_available_position != -1){
        last_source_registered_pos = first_available_position;
        memmove(Strg_ptr[first_available_position].ID,Source_ID,sizeof(char)*10);
        memmove(Strg_ptr[first_available_position].last_message_received,amostra_cod,sizeof(char)*10);
        Strg_ptr[first_available_position].Source_PDU1 = PDU_Used;
        Strg_ptr[first_available_position].last_time_received = tv.tv_sec;
        if(testing_turned_on == 2){
            printf("Source %s Received Message -> %s\n",Source_ID,amostra_cod);
        }
        return 1;
    }
    return 0;
}

/*When a source stops contacting the multimedia server or informs it that it will be closing
The source in question should be removed and it's element in the storage reseted so as to permit
The addition of new sources*/
void remove_source(Source *Strg_ptr,char Source_ID[10]){
    for(int  i = 0; i<max_sources ;i++){
        if(atoi(Strg_ptr[i].ID) != 0){
            if(strcmp(Strg_ptr[i].ID,Source_ID) == 0){
                memset(Strg_ptr[i].ID, 0,10);
                memset(Strg_ptr[i].clients_addresses,0,sizeof(struct sockaddr_in)*max_subs_per_source);
                memset(Strg_ptr[i].last_message_received,0,10);
                Strg_ptr[i].last_time_received = 0;
                memset(Strg_ptr[i].client_storage,0,sizeof(client)*max_subs_per_source);
                Strg_ptr[i].client_count = 0;
            }
        }
    }
}

/*Registers a client as a source subscriber
If the source be maxed out in terms of subscribers the client must be informed
1-> Client was subscribed
2-> Maxed out source
3-> Client is already subscribed to this source
0-> No valid Source ID*/
int subscribe_to_source(Source *Strg_ptr,int Source_ID,client client_used,struct sockaddr_in clt_add){
    int client_address_pos = 0;
    for(int i = 0; i < max_sources; i++){
        if(atoi(Strg_ptr[i].ID) == Source_ID && Source_ID != 0){
            printf("Same source ID\n");
            if(Strg_ptr[i].client_count < max_subs_per_source){
                //  printf("->%d",Strg_ptr[i].client_count);
                printf("Still have available slots\n");
                while(Strg_ptr[i].client_storage[client_address_pos].client_address.sin_addr.s_addr != 0){
                    //printf("ID_COMPARE_TO->%s\n",Strg_ptr[i].client_storage[client_address_pos].ID);
                    //printf("ID_COMPARED->%s\n",client_used.ID);
                    if(strcmp(Strg_ptr[i].client_storage[client_address_pos].ID,client_used.ID) == 0){
                        printf("This client is already subscribed to this source\n");
                        return 3;
                    }
                    client_address_pos++;
                }
                //printf("%s\n",client_used.ID);
                printf("i->%d\n",i);
                printf("client_n->%d\n",client_address_pos);
                Strg_ptr[i].client_storage[client_address_pos] = client_used;
                printf("%d\n",Strg_ptr[i].client_storage[client_address_pos].client_address.sin_addr.s_addr);
                Strg_ptr[i].client_storage[client_address_pos].ID[5] = '\0';
                printf("%s\n",Strg_ptr[i].client_storage[client_address_pos].ID);
                Strg_ptr[i].client_storage[client_address_pos].credits_to_disconnect = MaximumNumberOfCredits;
                Strg_ptr[i].client_count++;
                last_source_subscribed = i;
                return 1;
            }else{
                return 2;
            }
        }
    }
    return 0;
}

/*If a client wants to unsubscribe from a source
We simply reset the IP associated with the client to 0
1-> Client was unsubscribed
0-> Error*/
int unsubscribe_to_source(Source *Strg_ptr,int Source_ID,client client_used){
    for(int i = 0; i < max_sources; i++){
        if(atoi(Strg_ptr[i].ID) == Source_ID){
            for(int client_n = 0; client_n < max_subs_per_source;client_n++){
                if(Strg_ptr[i].client_storage[client_n].ID[0] != '\0'){
                    //printf("ID->%s\n",Strg_ptr[i].client_storage[client_n].ID);
                    //printf("ID_tent->%s\n",client_used.ID);
                    if(strcmp(Strg_ptr[i].client_storage[client_n].ID,client_used.ID) == 0){
                        Strg_ptr[i].client_count--;
                        Strg_ptr[i].client_storage[client_n].client_address.sin_addr.s_addr = 0;
                        memset(Strg_ptr[i].client_storage[client_n].ID,'\0',sizeof(Strg_ptr[i].client_storage[client_n].ID));
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

/*When a client credits reach 0 it is assumed that it has lost interest in the content of the source
Thus the client is required to send an datagram requesting to reset it's credits
This datagram should be sent after every 10 datagrams received by it calling this function to reset the credits associated with the client and the source
Since the only time this function is called is when everything is set and running, we don't need any return values*/
void reset_credits(Source *Strg_ptr,int Source_ID,client client_used){
    //printf("Client %s wished to reset it's credits\n", client_used.ID);
    for(int i = 0; i < max_sources; i++){
        if(atoi(Strg_ptr[i].ID) == Source_ID){
            for(int client_n = 0; client_n < max_subs_per_source;client_n++){
                if(Strg_ptr[i].client_storage[client_n].ID[0] != '\0'){
                    if(strcmp(Strg_ptr[i].client_storage[client_n].ID,client_used.ID) == 0){
                        Strg_ptr[i].client_storage[client_n].credits_to_disconnect = MaximumNumberOfCredits;
                        break;
                    }
                }
            }
        }
    }
}

//Same function as the source_message_splitter but for messages to subscribe to a client to a specific source
//This is also used to retrieve the client ID while also returning the source ID
//This function is used for all types of request since they all use the same datagram composition
int Request_message_splitter(char *message, client *client_register){
    char buffer[10];
    int word_size = 0;
    PDU1 new_PDU;
    int what_section = 0;
    int ID = 0;
    for(int i = 0; i != strlen(message);i++){
        if(message[i] != '|'){
            buffer[word_size] = message[i];
            word_size++;
        }
        if(message[i] == '|'){
            if(what_section == 1){
                memcpy(message,buffer,sizeof(char)*word_size);
                ID = atoi(buffer);
            }
            if(what_section == 2){
                client_register->ID[5] = '\0';
                memmove(client_register->ID,buffer,strlen(buffer));
                break;
            }
            memset(buffer,'\0',10);
            word_size = 0;
            what_section++;
        }
    }
    return ID;
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
                    //memcpy(new_PDU.Fonte_Tipo,buffer,sizeof(char)*word_size);
                    break;
                case 1:
                    memcpy(new_PDU.Fonte_Tipo,buffer,sizeof(char)*word_size);
                    break;
                case 2:
                    memcpy(new_PDU.Identificador,buffer,sizeof(char)*word_size);
                    new_PDU.Identificador[word_size]='\0';
                    break;
                case 3:
                    memcpy(new_PDU.Val_amostra,buffer,sizeof(char)*word_size);
                    new_PDU.Val_amostra[word_size]='\0';
                    break;
                case 4:
                    memcpy(new_PDU.Val_amostra_cod,buffer,sizeof(char)*word_size);
                    new_PDU.Val_amostra_cod[word_size]='\0';
                    break;
                case 5:
                    memcpy(new_PDU.Periodo,buffer,sizeof(char)*word_size);
                    new_PDU.Periodo[word_size]='\0';
                    break;
                case 6:
                    memcpy(new_PDU.Frequencia,buffer,sizeof(char)*word_size);
                    new_PDU.Frequencia[word_size]='\0';
                    break;
                case 7:
                    memcpy(new_PDU.Num_Amostras,buffer,sizeof(char)*word_size);
                    new_PDU.Num_Amostras[word_size]='\0';
                    break;
                case 8:
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

//This function just returns the position of a source in the source array
int get_source_pos(int Source_ID){
    for(int i = 0; i < max_sources; i++){
        if(Source_Storage[i].ID[0] != '\0'){
            if(atoi(Source_Storage[i].ID) == Source_ID){
                return i;
            }
        }
    }
    return -1;
}

//This thread takes care of sending the last message associated with a source to the subscriber to which the thread is associated
//We use this method to avoid any latency associated with searching for the subscriber before sending the message 
//Each source has a thread for itself after it receives it's first subscriber
void *handle_source_thread(void *arg){
    struct timeval sent_time;
    Request_ThreadArgs Args_For_CreditRunout;
    struct sockaddr_in client_addr_arg;
    int tester = 0;
    Sub_ThreadArgs *args = (Sub_ThreadArgs *)arg;
    Source Used_Source = args->Sources;
    PDU1 Used_PDU = args->PDU1_used;
    client myclient = args->cliente_sub;
    int Data_Socket;
    int clients = 0;
    int sent_to_all_clients = 0;
    if ((Data_Socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Failed to create Socket");
        exit(EXIT_FAILURE);
    }
    char message_buffer[40];
    for(int i = 0; i<max_sources;i++){
        if(strcmp(Used_Source.ID,Source_Storage[i].ID) == 0){
            for(int client_n = 0; client_n < max_subs_per_source; client_n++){
                if(Used_Source.client_storage[client_n].client_address.sin_addr.s_addr != 0){
                    if(strcmp(myclient.ID,Used_Source.client_storage[client_n].ID) == 0){
                        client_addr_arg = Used_Source.client_storage[client_n].client_address;
                        while(Used_Source.client_storage[client_n].credits_to_disconnect != 0){
                            
                            //printf("%s\n",Source_Storage[i].last_message_received);
                            sprintf(message_buffer,"7|%s|",Source_Storage[i].last_message_received);
                            Used_Source.client_storage[client_n].credits_to_disconnect--;
                            if (sendto(Data_Socket, message_buffer, strlen(message_buffer), 0, (struct sockaddr * ) & Used_Source.client_storage[client_n].client_address, sizeof(Used_Source.client_storage[client_n].client_address)) < 0) {
                                perror("Data couldn't be sent to the client");
                            }else{
                                if(testing_turned_on == 2){
                                    printf("ID-> %s Message sent -> %s\n",Used_Source.ID,Source_Storage[i].last_message_received);
                                }
                                if(testing_turned_on == 1){
                                    gettimeofday(&sent_time,NULL);
                                    printf("%s,Time of sending->%ld\n",Source_Storage[i].last_message_received,(sent_time.tv_sec * 1000000 + sent_time.tv_usec));
                                }
                            }
                            memset(message_buffer,'\0',40);
                            sleep(1/(atoi(Used_PDU.Frequencia)*atoi(Used_PDU.Num_Amostras)));
                        }
                    }
                }else{
                    printf("%s\n",Used_Source.client_storage[client_n].ID);
                    if(Source_Storage[i].ID[0] == 0){
                        sprintf(message_buffer,"2|%s|",Used_Source.ID);
                        printf("%s\n",message_buffer);
                        if (sendto(Data_Socket, message_buffer, strlen(message_buffer), 0, (struct sockaddr * ) & client_addr_arg, sizeof(client_addr_arg)) < 0) {
                            perror("Data couldn't be sent to the client");
                        }
                    }
                    perror("This client doesn't exist in this source");
                    pthread_exit(NULL);
                }
            }
        }
    }
    pthread_exit(NULL);
    return NULL;
}

/*Since some operations are time consuming we transfer all other communications besides sending the data to clients and data collection from sources (and other operations associated to them)
to a thread, thus allowing each request to be handled independently, augmenting the SM flexibility
*/
void *handle_request_thread(void *arg){

    Request_ThreadArgs *args = (Request_ThreadArgs *)arg;
    int socket_fd = args->socket_to_use;
    Source *SrcStr_requesthread_ptr = Source_Storage;
    client request_client = args->client_used;
    int Source_Requested_ID = args->Source_ID;
    int type_request = args->case_type;
    struct sockaddr_in Client_Addr_2 = args->client_addr_arg;
    char request_answer_buffer[1024];

    switch(type_request){
        case 2: //Client->SM Source Listing Request
            strcat(request_answer_buffer,"6|");
            for(int source_counter = 0; source_counter<max_sources ; source_counter++){
                if(atoi(Source_Storage[source_counter].ID) != 0){
                    
                    strcat(request_answer_buffer,Source_Storage[source_counter].ID);
                    strcat(request_answer_buffer,"\n");
                }
            }
            //printf("message->%s\n",Source_Listing_Message);
            if (sendto(socket_fd, request_answer_buffer, strlen(request_answer_buffer), 0, (struct sockaddr * ) &request_client.client_address, sizeof(request_client.client_address)) < 0) {
                    perror("Data couldn't be sent to the client");
                    break;
                }
            memset(request_answer_buffer,'\0',strlen(request_answer_buffer));
            break;
        case 3: ;
                printf("Client %s is attempting to Subscribe to source %d\n",request_client.ID,Source_Requested_ID);
                pthread_mutex_lock(&mutex);
                int sub_report = subscribe_to_source(SrcStr_requesthread_ptr,Source_Requested_ID,request_client,Client_Addr_2);
                pthread_mutex_unlock(&mutex);
                sprintf(request_answer_buffer,"8|%d|",Source_Requested_ID);
                printf("%d\n",sub_report);
                if(sub_report == 1){ //We update the source_storage that was sent to the thread since we just added a new subscriber to it
                    if(Source_Storage[last_source_subscribed].ID[0] != '\0'){
                        //If the Stored_source_args already has info in that position we just need to update it
                            Sub_ThreadArgs sub_args;
                            pthread_t source_thread;
                            sub_args.socket_fd = socket_fd;
                            sub_args.Sources = Source_Storage[last_source_subscribed];
                            sub_args.PDU1_used = Source_Storage[last_source_subscribed].Source_PDU1;
                            sub_args.cliente_sub = request_client;
                            int threadCreateResult = pthread_create(&source_thread, NULL, handle_source_thread, (void *)&sub_args);
                            //Source_Threads[last_source_subscribed] = source_thread;
                            //printf("%s\n",request_answer_buffer);
                            if(sendto(socket_fd, request_answer_buffer, strlen(request_answer_buffer), 0, (struct sockaddr * ) &request_client.client_address, sizeof(request_client.client_address))<0){
                                perror("Data couldn't be sent to the client");
                                break;
                            }
                            //Threaded_Sources_ID[last_source_subscribed] = atoi(Source_Storage[last_source_subscribed].ID);
                            if(threadCreateResult != 0){
                                perror("Couldn't create the source thread");
                            }else{
                                pthread_detach(source_thread); //Since the request thread closes as soon as the request is fulfilled we need to detach the thread associated with the source
                            }
                    }else{
                            
                    }
                }
                if(sub_report == 0){
                    printf("Couldn't subcribe the client %s from the source %d, reason:No valid Source ID\n",request_client.ID,Source_Requested_ID);
                    memset(request_answer_buffer,'\0',1024);
                    sprintf(request_answer_buffer,"12|%d|No valid Source ID|",Source_Requested_ID);
                    if(sendto(socket_fd, request_answer_buffer, strlen(request_answer_buffer), 0, (struct sockaddr * ) &request_client.client_address, sizeof(request_client.client_address))<0){
                        perror("Data couldn't be sent to the client");
                        break;
                    }
                }
                if(sub_report == 3){
                    printf("Couldn't subcribe the client %s from the source %d, reason:Client is already subscribed to this source\n",request_client.ID,Source_Requested_ID);
                    memset(request_answer_buffer,'\0',1024);
                    sprintf(request_answer_buffer,"12|%d|Client is already subscribed to this source|",Source_Requested_ID);
                    if(sendto(socket_fd, request_answer_buffer, strlen(request_answer_buffer), 0, (struct sockaddr * ) &request_client.client_address, sizeof(request_client.client_address))<0){
                        perror("Data couldn't be sent to the client");
                        break;
                    }
                }
                if(sub_report == 2){
                    printf("Couldn't subcribe the client %s from the source %d, reason:Maxed out source\n",request_client.ID,Source_Requested_ID);
                    memset(request_answer_buffer,'\0',1024);
                    sprintf(request_answer_buffer,"12|%d|Maxed out source|",Source_Requested_ID);
                    if(sendto(socket_fd, request_answer_buffer, strlen(request_answer_buffer), 0, (struct sockaddr * ) &request_client.client_address, sizeof(request_client.client_address))<0){
                        perror("Data couldn't be sent to the client");
                        break;
                    }
                }
            break;
        case 4: ;
            printf("Client %s is attempting to Unsubscribe from source %d\n",request_client.ID,Source_Requested_ID);
            pthread_mutex_lock(&mutex);
            int unsub_report = unsubscribe_to_source(SrcStr_requesthread_ptr,Source_Requested_ID,request_client);
            pthread_mutex_unlock(&mutex);
            sprintf(request_answer_buffer,"9|%d|",Source_Requested_ID);
            if(unsub_report == 1){
                printf("Unsubbed the client %s from the source %d\n",request_client.ID,Source_Requested_ID);
                if(sendto(socket_fd, request_answer_buffer, strlen(request_answer_buffer), 0, (struct sockaddr * ) &request_client.client_address, sizeof(request_client.client_address))<0){
                    perror("Data couldn't be sent to the client");
                    break;
                }
            }else{ //Since suscribe can only return 2 states, if it is different from 1 the SM wasn't able to unsubscribe the client
                printf("Couldn't unsub the client %s from the source %d\n",request_client.ID,Source_Requested_ID);
                memset(request_answer_buffer,'\0',1024);
                sprintf(request_answer_buffer,"11|%d|Couldn't Unsubscribe client",Source_Requested_ID);
                if(sendto(socket_fd, request_answer_buffer, strlen(request_answer_buffer), 0, (struct sockaddr * ) &request_client.client_address, sizeof(request_client.client_address))<0){
                    perror("Data couldn't be sent to the client");
                    break;
                }
            }
            break;
        case 5: ;
            pthread_mutex_lock(&mutex);
            reset_credits(SrcStr_requesthread_ptr,Source_Requested_ID,request_client);
            pthread_mutex_unlock(&mutex);
            break;
        case 6: ;
            int pos_to_be_used = get_source_pos(Source_Requested_ID);
            printf("Client %s wants to get information about the source -> %d\n",request_client.ID,Source_Requested_ID);
            printf("%d\n",pos_to_be_used);
            sprintf(request_answer_buffer,"1|%d|%s|%s|%s|%s|",Source_Requested_ID,Source_Storage[pos_to_be_used].Source_PDU1.Fonte_Tipo,
            Source_Storage[pos_to_be_used].Source_PDU1.Frequencia,Source_Storage[pos_to_be_used].Source_PDU1.Num_Amostras,Source_Storage[pos_to_be_used].Source_PDU1.Periodo_Max);
            if(pos_to_be_used != -1){
                if(sendto(socket_fd, request_answer_buffer, strlen(request_answer_buffer), 0, (struct sockaddr * ) &request_client.client_address, sizeof(request_client.client_address))<0){
                    perror("Data couldn't be sent to the client");
                    break;
                }
            }else{
                memset(request_answer_buffer,'\0',1024);
                sprintf(request_answer_buffer,"13|%d|",Source_Requested_ID);
                if(sendto(socket_fd, request_answer_buffer, strlen(request_answer_buffer), 0, (struct sockaddr * ) &request_client.client_address, sizeof(request_client.client_address))<0){
                    perror("Data couldn't be sent to the client");
                    break;
                }
            }
            break;
        default:
            break;
    }
    
    pthread_exit(NULL);
}

/*Since managing the credits is too time consuming to be done when the data is about to be sent
We do it in an independent thread that is always checking the sources for clients that might have runned out of credits
Since the unsubscribe option might occur in a race condition we need to use mutexes from now on*/
//This thread is unique and is always running until the program closes and doesn't need arguments since if a client has runned of credit it is probably dead
void* credit_manager_handler(void *arg){
    int Source_ID_nocredits = 0;
    Source *Storage_ptr_forcredits = Source_Storage;
    client client_to_disconnect;
    while(1){
        for(int source_num = 0; source_num < max_sources; source_num++){
            if(Source_Storage[source_num].ID[0] != '\0'){
                for(int num_client = 0; num_client < max_subs_per_source; num_client++){
                    if(Source_Storage[source_num].client_storage[num_client].client_address.sin_addr.s_addr != 0){
                        if(Source_Storage[source_num].client_storage[num_client].credits_to_disconnect == 0){
                            Source_ID_nocredits = atoi(Source_Storage[source_num].ID);
                            client_to_disconnect = Source_Storage[source_num].client_storage[num_client];
                            printf("The client with the ID -> %s, has runned out of credits in the source -> %s\n",client_to_disconnect.ID,Source_Storage[source_num].ID);
                            pthread_mutex_lock(&mutex); //Since this part disconnects a client for lacking credits we should consider this a critical part and thus we should apply a mutex since a request might come in while a user is being disconnected in this thread
                            unsubscribe_to_source(Storage_ptr_forcredits,Source_ID_nocredits,client_to_disconnect);
                            pthread_mutex_unlock(&mutex);
                        }
                    }
                }
            }
        }
    }
}

//There should always be a thread checking if a source is sending messages inside an allowed timeout time.
//If the SM hasn't received a message from a source for more than 3 seconds we can assume that the source has been either shutted down or crashed.
//This thread just compares the current time and the time of the last message received from a source
void* source_timeout_manager_handler(void *arg){
    Source *Storage_ptr_forremove = Source_Storage;
    int warn_client_of_source_socket;
    char warning_buffer[40];
    if ((warn_client_of_source_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Failed to create Socket");
        exit(EXIT_FAILURE);
    }
    while(1){
        for(int source_num = 0; source_num < max_sources; source_num++){
            if(Source_Storage[source_num].ID[0] != '\0'){
                if(time(NULL)-Source_Storage[source_num].last_time_received > Source_timeout_time ){
                    printf("Source %s, has been disconnected do to a timeout\n",Source_Storage[source_num].ID);
                    //If we get here we can be 100% certain that the source really has been disconnected thus a mutex shouldn't be needed here
                    remove_source(Storage_ptr_forremove,Source_Storage[source_num].ID);
                    }
                }
            }
        }
    }

//Returns the message type from any message that arrives to the SM
int type_getter(char* buffer){
    int char_counter = 0;
    char type_buffer[10];
    type_buffer[10] = '\0';
    while(buffer[char_counter] != '|'){
        type_buffer[char_counter] = buffer[char_counter];
        char_counter++;
    }
    int result = atoi(type_buffer);
    return result;
}

int main(int argc,char* argv[]) { //Args necessários, IP_servidor,port, número máximo de fontes,número máximo de subscritores por fontes
    
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
        if(argc>5){
            if(atoi(argv[5]) == 1){
                //Delay Testing [This is just going to printout the time (in micros) at which a message was sent [Try to use the least number of sources for this]]
                testing_turned_on = 1;
            }
            if(atoi(argv[5]) == 2){
                //Received | Sent error detection [We see what is received by a source and what the SM sends to it's clients [USE A MINIMAL NUMBER OF SOURCES FOR THIS]
                testing_turned_on = 2;
            }
        }
    }

    //Source_Storage = (Source*)malloc(sizeof(Source)*max_sources);

    Source_Storage = (Source*)malloc(max_sources*sizeof(Source));
    
    Source *Source_Storage_ptr = Source_Storage;
    Threaded_Sources_ID = (int *)malloc(max_sources*sizeof(int));
    Source_Threads = (pthread_t *)malloc(max_sources*sizeof(pthread_t));

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
    -> 3  Client->SM Source Subscription Request
    -> 4  Client->SM Source Unsubscription Request*/
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

    /*If the source_storage is filled up we send one warning turning this variable to 1 so as to not hinder the server perfomance with useless multiple warnings,
     but if a flag with the value of 1 is received it is turned back to 0 since a new source was added */
    int full_source_storage_Warning = 0; 
    int source_flag = 0;

    char Source_Listing_Message[10*max_sources+1];//New line space is the max_sources added

    Initalize_Storage(Source_Storage_ptr);

    pthread_t CreditsManager_Thread;
    pthread_create(&CreditsManager_Thread,NULL,credit_manager_handler,NULL);
    pthread_t SourceTimeoutManagerThread;
    pthread_create(&SourceTimeoutManagerThread,NULL,source_timeout_manager_handler,NULL);

    int Rq_threadCreateResult = -1;

    while(1) {
        int len, n;
        len = sizeof(client_addr);
        n = recvfrom(UDP_Socket_fd,(char *)buffer ,MAXLINE, MSG_WAITALL,(struct sockaddr *)&client_addr ,&len); //
        buffer[n] = '\0';
        type_source = type_getter(buffer);
        Request_ThreadArgs Request_Args;
        pthread_t request_thread;
        switch(type_source){
            case 1: ;
                PDU_1 = source_message_splitter(buffer);
                source_flag = add_source_or_update_message(Source_Storage_ptr,PDU_1.Identificador,PDU_1.Val_amostra_cod,PDU_1);
                if(source_flag == 0 && full_source_storage_Warning == 0){
                    full_source_storage_Warning = 1;
                    perror("Limit of Sources hitted");
                }
                if(source_flag == 1 && full_source_storage_Warning == 1){
                    full_source_storage_Warning = 0;
                }
                if(source_flag == 1){
                    
                }
                break;
            case 2: //Client->SM Source Listing Request
                strcat(Source_Listing_Message,"6|");
                for(int source_counter = 0; source_counter<max_sources ; source_counter++){
                    if(atoi(Source_Storage[source_counter].ID) != 0){
                        
                        strcat(Source_Listing_Message,Source_Storage[source_counter].ID);
                        strcat(Source_Listing_Message,"\n");
                    }
                }
                if (sendto(UDP_Socket_fd, Source_Listing_Message, strlen(Source_Listing_Message), 0, (struct sockaddr * ) &client_addr, sizeof(client_addr)) < 0) {
                        perror("Data couldn't be sent to the client");
                        break;
                    }
                memset(Source_Listing_Message,'\0',10*max_sources+max_sources+1);
                break;
            case 3: ;//Client->SM Source Subscription, when receiving messages from the client we only care about the ID he chooses
                client temp_client_struct;
                temp_client_struct.client_address = client_addr;
                client *temp_client_ptr;
                temp_client_ptr = &temp_client_struct;
                int ID = Request_message_splitter(buffer,temp_client_ptr);
                Request_Args.socket_to_use = UDP_Socket_fd;
                Request_Args.client_used = temp_client_struct;
                //printf("ID->%d\n",ID);
                Request_Args.Source_ID = ID;
                Request_Args.case_type = type_source;
                Request_Args.client_addr_arg = client_addr;
                Rq_threadCreateResult = pthread_create(&request_thread, NULL, handle_request_thread, (void *)&Request_Args);
                if(Rq_threadCreateResult != 0){
                    perror("Couldn't create the request thread");
                }else{
                    
                }
                break;
            case 4: ;
                client temp_client_struct2;
                temp_client_struct2.client_address = client_addr;
                client *temp_client_ptr2;
                temp_client_ptr2 = &temp_client_struct2;
                int unsub_ID = Request_message_splitter(buffer,temp_client_ptr2);
                Request_Args.socket_to_use = UDP_Socket_fd;
                Request_Args.client_used = temp_client_struct2;
                Request_Args.Source_ID = unsub_ID;
                Request_Args.case_type = type_source;
                Request_Args.client_addr_arg = client_addr;
                Rq_threadCreateResult = pthread_create(&request_thread, NULL, handle_request_thread, (void *)&Request_Args);
                if(Rq_threadCreateResult != 0){
                    perror("Couldn't create the request thread");
                }else{
                    
                }
                break;
            case 5: ;
                client temp_client_struct3;
                temp_client_struct3.client_address = client_addr;
                client *temp_client_ptr3;
                temp_client_ptr3 = &temp_client_struct3;
                int creditreset_ID = Request_message_splitter(buffer,temp_client_ptr3);
                Request_Args.socket_to_use = UDP_Socket_fd;
                Request_Args.client_used = temp_client_struct3;
                Request_Args.Source_ID = creditreset_ID;
                Request_Args.case_type = type_source;
                Request_Args.client_addr_arg = client_addr;
                Rq_threadCreateResult = pthread_create(&request_thread, NULL, handle_request_thread, (void *)&Request_Args);
                if(Rq_threadCreateResult != 0){
                    perror("Couldn't create the request thread");
                }else{
                    
                }
                break;
            case 6: ;
                client temp_client_struct4;
                temp_client_struct4.client_address = client_addr;
                client *temp_client_ptr4;
                temp_client_ptr4 = &temp_client_struct4;
                int source_info_ID = Request_message_splitter(buffer,temp_client_ptr4);
                Request_Args.socket_to_use = UDP_Socket_fd;
                Request_Args.client_used = temp_client_struct4;
                Request_Args.Source_ID = source_info_ID;
                Request_Args.case_type = type_source;
                Request_Args.client_addr_arg = client_addr;
                Rq_threadCreateResult = pthread_create(&request_thread, NULL, handle_request_thread, (void *)&Request_Args);
                if(Rq_threadCreateResult != 0){
                    perror("Couldn't create the request thread");
                }else{
                    
                }
                break;
            default:
                printf("%d\n",type_source);
                perror("Non valid type of case request, discarded");
                break;
        }
        Rq_threadCreateResult = -1;
        memset(buffer,'\0',sizeof(buffer));
    }
    return 0;
}