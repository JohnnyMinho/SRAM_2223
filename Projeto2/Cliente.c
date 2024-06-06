#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/time.h>

#define max_timeout_time 2 //Amount of seconds that a socket has to receive info from the SM until a timeout occurs
                        //If the request isn't answered in time by the server we stop waiting for a response and just go back to the menu, this check should be used before anything is registed in the client
int number_of_used_sources = 0;
int selected_thread = -1; //Since we need this variable to check which source the client wants to use for data visualization, we make it global, this indicated the position in the source storage array
int max_srcs_client = 10;
char **subscribed_sources_ID;
int testing_mode = 0; //Like in the server if we have this mode activated we won't see the prints of output of the data if it is with the value of 1, but we will see the times of arrival to try and check the delay

struct timeval sockets_timeout_timer;

pthread_mutex_t mutex_sub = PTHREAD_MUTEX_INITIALIZER;

struct sockaddr_in server_add_used;

typedef struct thread_args{
    int socket_fd_used;
    int source_position_in_storage;
    int credits_to_disconnect;
    char MY_ID[6];
    char Source_ID[10];
    struct sockaddr_in server_addr;
}thread_args;

void print_menu(char *ID){
    printf("My ID->%s\n",ID);
    printf("Multimedia Client MENU\n");
    printf("1-> Get media sources listing\n");
    printf("2-> Subscribe to Source\n");
    printf("3-> Unsubscribe from source\n");
    printf("4-> Show Data incoming from source\n");
    printf("5-> Get information about a source\n");
    printf("0-> Exit Program\n");
    printf("Choose an option: ");
}

//Used to make the use of the client
void clear_console() {
    #if defined(_WIN32) || defined(_WIN64)
        system("cls");
    #else
        system("clear");  
    #endif
}

//The only thing we care about when receiving data from the SM is the coded sample, so we only try to retrieve that
//We could use this as a way to retrieve the ID in a single function by using a struct
int get_number_of_symbols(char *buffer_received){
    char buffer[10];
    int word_size = 0;
    int what_section = 0;
    for(int i = 0; i != strlen(buffer_received);i++){
        if(buffer_received[i] != '|'){
            buffer[word_size] = buffer_received[i];
            word_size++;
        }
        if(buffer_received[i] == '|'){
            if(what_section == 1){
                int n_symb = atoi(buffer);
                return n_symb;
            }
            what_section++;
            word_size = 0;
            memset(buffer,'\0',10);
        }
    }
    return 0;
}

//Translates the number of symbols to the actual symbol (*)
void print_content(int num_of_symbols){
    //printf("Hello2_1\n");
    for(int symbol_n = 0; symbol_n < num_of_symbols; symbol_n++){
        printf("*");
    }
    printf("\n");
    //printf("Hello2_2\n");
}

//This function quickly gets all the available sources listed in the listing packet receiving from the server
void print_available_sources_from_server(char *buffer_received){
    //printf("Hello1_1\n");
    char buffer[10];
    int word_size = 0;
    int what_section = 0;
    printf("Currently Available Sources:\n");
    for(int i = 0; i != strlen(buffer_received);i++){
        if(buffer_received[i] != '|' || buffer_received[i] == '\n'){
            buffer[word_size] = buffer_received[i];
            word_size++;
        }
        if(buffer_received[i] == '|' || buffer_received[i] == '\n'){
            what_section++;
            if(what_section > 1){
                printf("%d->%s\n",what_section-1,buffer);
            }
            word_size = 0;
            memset(buffer,'\0',10);
        }
    }
    //printf("Hello1_3\n");
}

//Removes one source from the storage, thus unsubscribing the client from it
//An unsubscribe datagram should also be sent to the SM
void unsubscribe_from_source(int source_pos){
    for(int rows=0;rows<max_srcs_client;rows++){
        if(rows == source_pos && subscribed_sources_ID[rows][0] != '\0'){
            memset(subscribed_sources_ID[rows],'\0',10);
            printf("Source fully unsubscribed\n");
        }
    }
}

//Things Still needing to be done -> Array containing the subscribed sources, a argument struct to give the thread
//A way to confirm that the thread is the one the client want to see data from
//To achieve this we simply need to use the ID number as the condition to let the thread read from the socket
//And filter out the packets
void* Info_Thread_Handler(void *arg){
    struct timeval recv_time;
    int credits_used = 0;
    thread_args *args = (thread_args*) arg;
    int socket_fd = args->socket_fd_used;
    int my_source_pos = args->source_position_in_storage;
    char Client_ID[6];
    char Source_ID[10];
    Source_ID[10] = '\0';
    memcpy(Client_ID,args->MY_ID,6);
    memcpy(Source_ID,args->Source_ID,10);
    struct sockaddr_in server_addr_used_thread = args->server_addr;
    char request_buffer[25];
    char reset_credits_buffer[25];
    socklen_t len = sizeof(server_add_used);
    while(1){
        if(subscribed_sources_ID[my_source_pos][0] == '\0'){
            //If the thread isn't able to get a valid ID it is very likely that the source was unsubscribed by the client, ending the thread and closing the socket
            //printf("No source in this position or the connection was closed");
            close(socket_fd);
            pthread_exit(NULL); //Valid close
        }
        ssize_t num_bytes_recv = recvfrom(socket_fd,request_buffer,sizeof(request_buffer)-1, 0, NULL, NULL);
        if(num_bytes_recv > 0){
            credits_used++;
        }
        if(my_source_pos == selected_thread){
            if(request_buffer[0] == '7'){
                if(testing_mode == 0){
                    int symbols = get_number_of_symbols(request_buffer);
                    print_content(symbols);
                }
                if(testing_mode == 1){
                    printf("%d\n",get_number_of_symbols(request_buffer));
                    gettimeofday(&recv_time,NULL);
                    printf("Time of arrival->%ld\n",(recv_time.tv_sec * 1000000 + recv_time.tv_usec));
                }
            }
            memset(request_buffer,'\0',25);
        }else{
            if(request_buffer[0] == '2'){
                //Attempt to make a way for the client to know if the source is down 
                printf("This source %s is dead\n",Source_ID);
                pthread_mutex_lock(&mutex_sub);
                printf("\n");
                unsubscribe_from_source(my_source_pos);
                printf("\n");
                pthread_mutex_unlock(&mutex_sub);
            }
            memset(request_buffer,'\0',25);
        }
        if(credits_used>80000){  //The amount of credits is hardcoded to 100000
            sprintf(reset_credits_buffer,"5|%s|%s|",Source_ID,Client_ID);
            if (sendto(socket_fd, reset_credits_buffer, strlen(reset_credits_buffer), 0, (struct sockaddr * ) & server_addr_used_thread, sizeof(server_addr_used_thread)) < 0) {
                perror("Couldn't reset credits");
            }
            credits_used = 0;
        }
    }
}

//If the subscription process was accepted in the server side we register the new source so as to store it
int register_source(char *buffer_received){
    char buffer[10];
    buffer[10] = '\0';
    int word_size = 0;
    int what_section = 0;
    for(int i = 0; i != strlen(buffer_received);i++){
        if(buffer_received[i] != '|'){
            buffer[word_size] = buffer_received[i];
            word_size++;
        }
        if(buffer_received[i] == '|'){
            if(what_section == 1){
                for(int row = 0;row < max_srcs_client;row++){
                    if(subscribed_sources_ID[row][0] == '\0'){
                        strcpy(subscribed_sources_ID[row],buffer);
                        subscribed_sources_ID[10] = '\0';
                        return row;
                        break;
                    }
                }
            }
            what_section++;
            word_size = 0;
            memset(buffer,'\0',10);
        }
    }
    return 0;
}

//Just prints out the subscribed sources
void print_stored_sources(){
    for(int rows=0;rows<max_srcs_client;rows++){
        if(subscribed_sources_ID[rows][0] != '\0' && atoi(subscribed_sources_ID[rows]) != 0){
            printf("%d->%s\n",rows,subscribed_sources_ID[rows]);
        }
    }
}

//If something goes wrong in the server side we get a small justification with it
void error_message_splitter(char *message){
    char buffer[1024];
    int word_size = 0;
    int what_section = 0;
    for(int i = 0; i != strlen(message);i++){
        if(message[i] != '|'){
            buffer[word_size] = message[i];
            word_size++;
        }
        if(message[i] == '|'){
            if(what_section == 2){
                printf("Reason:%s\n",buffer);
                break;
            }
            memset(buffer,'\0',10);
            word_size = 0;
            what_section++;
        }
    }
}

void printout_source_info(char *message){ //ID, TIPO ; FREQ ; NUM_AMOSTRAS;PERD_MAX
    char buffer[1024];
    int word_size = 0;
    int what_section = 0;
    for(int i = 0; i != strlen(message);i++){
        if(message[i] != '|'){
            buffer[word_size] = message[i];
            word_size++;
        }
        if(message[i] == '|'){
            if(what_section == 2){
                printf("Source type:%s\n",buffer);
            }
            if(what_section == 3){
                printf("Source frequency:%s\n",buffer);
            }
            if(what_section == 4){
                printf("Source number of samples:%s\n",buffer);
            }
            if(what_section == 5){
                printf("Source maximum period:%s\n",buffer);
            }
            memset(buffer,'\0',10);
            word_size = 0;
            what_section++;
        }
    }
}

int main(int argc,char* argv[]){

    int SM_Port;
    char *SM_IP_address;
    char input_buffer[10];
    input_buffer[10] = '\0';
    char udp_buffer[1024];
    int chosen_option;
    char cleaner_helper;
    char comm_buffer[100];
    char ID[6];
    struct timeval request_delay_test;
    long int delay = 0; //Our delay will be measured in microseconds
    ID[6] = '\0';
    sockets_timeout_timer.tv_sec = max_timeout_time;

    if(argc>3){
        SM_IP_address = (char *)malloc(sizeof(char)*strlen(argv[1]));
        memcpy(SM_IP_address,argv[1],strlen(argv[1]));
        SM_Port = atoi(argv[2]);
        memcpy(ID,argv[3],sizeof(ID));
        if(argc>4){
            if(atoi(argv[4]) == 1){
                //Delay Testing [This is just going to printout the time (in micros) at which a message was sent [Try to use the least number of sources for this]]
                testing_mode = 1;
            }
            if(atoi(argv[4]) == 2){
                
                testing_mode = 2;
            }
        }
    }

    subscribed_sources_ID = (char **)malloc(max_srcs_client*sizeof(char*));
    for(int rows = 0; rows < max_srcs_client;rows++){
        subscribed_sources_ID[rows] = (char*)malloc(10*sizeof(char));
        memset(subscribed_sources_ID[rows],'\0',10); //To assist in checking if there is any source assigned to a x position in storage since starting a thread without anything to do is not right
    }

    int Request_UDP_Socket_fd = socket(AF_INET, SOCK_DGRAM, 0); //We use this socket to send request to the SM
    
    struct sockaddr_in server_addr;
    socklen_t server_addr_len;

    if (Request_UDP_Socket_fd  < 0){
        perror("Failure to open an UDP socket, closing");
        exit(-1);
    }
    if(setsockopt (Request_UDP_Socket_fd, SOL_SOCKET, SO_RCVTIMEO, &sockets_timeout_timer,sizeof(sockets_timeout_timer)) < 0){
        perror("Couldn't config the timeout of the socket");
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SM_Port);
    server_addr.sin_addr.s_addr = inet_addr(SM_IP_address);
    server_add_used = server_addr;
    server_addr_len = sizeof(server_addr);
    sleep(1);

    if (inet_pton(AF_INET, SM_IP_address, & (server_addr.sin_addr)) <= 0) {
        //remove_ID(new_id,id_file,file_length);
        perror("The IP in use is not valid");
        exit(-1);
    }
    

    while(1){
        //SINCE we are awaiting confirmations for the subscription of sources / unsubcription we use this socket to allow an uninturrupted stream of data
        //Plus we have a socket to receive data from each source.
        clear_console();
        printf("The delay for the last operation %d: was %ld\n",chosen_option,delay);
        print_menu(ID);
        memset(udp_buffer,'\0',1024);
        int Info_UDP_Socket_fd = socket(AF_INET, SOCK_DGRAM, 0); 
        if(setsockopt (Info_UDP_Socket_fd, SOL_SOCKET, SO_RCVTIMEO, &sockets_timeout_timer,sizeof(sockets_timeout_timer)) < 0){
            perror("Couldn't config the timeout of the socket");
        }
        if(Info_UDP_Socket_fd < 0){
            perror("Failure to open an UDP socket, closing");
            exit(-1);
        }
        if(fgets(input_buffer,sizeof(input_buffer),stdin)){
            if(strchr(input_buffer, '\n') == NULL){ //Input is bigger than then allowed buffer size, we must eliminate the remaining characters in the stdin
                while ((cleaner_helper = getchar()) != '\n' && cleaner_helper != EOF);
            }
            chosen_option = atoi(input_buffer);
            switch (chosen_option){
                case 1:  //We still need to implement the checking of the packets after the client starts receiving data
                    sprintf(comm_buffer,"%s","2|");
                    if(testing_mode == 1){
                        gettimeofday(&request_delay_test,NULL);
                        delay = request_delay_test.tv_sec * 1000000 + request_delay_test.tv_usec;
                    }
                    if (sendto(Request_UDP_Socket_fd, comm_buffer, strlen(comm_buffer), 0, (struct sockaddr * ) & server_addr, sizeof(server_addr)) < 0) {
                        perror("Data couldn't be sent to the server");
                        break;
                    }
                    while(udp_buffer[0] != '6'){
                        memset(udp_buffer,'\0',1024);
                        recvfrom(Request_UDP_Socket_fd,udp_buffer,sizeof(udp_buffer),MSG_WAITALL,(struct sockaddr *)&server_add_used , &server_addr_len);   
                    }
                    if(testing_mode == 1){
                        gettimeofday(&request_delay_test,NULL);
                        delay = (request_delay_test.tv_sec * 1000000 + request_delay_test.tv_usec)-delay;
                    }
                    
                    print_available_sources_from_server(udp_buffer);
                    getchar();
                    memset(comm_buffer,'\0',100);
                    break;
                case 2: ;
                    sprintf(comm_buffer,"%s","2|");
                    if(testing_mode == 1){
                        gettimeofday(&request_delay_test,NULL);
                        delay = request_delay_test.tv_sec * 1000000 + request_delay_test.tv_usec;
                    }
                    if (sendto(Request_UDP_Socket_fd, comm_buffer, strlen(comm_buffer), 0, (struct sockaddr * ) & server_addr, sizeof(server_addr)) < 0) {
                        perror("Data couldn't be sent to the server");
                        break;
                    }
                    while(udp_buffer[0] != '6'){
                        memset(udp_buffer,'\0',1024);
                        recvfrom(Request_UDP_Socket_fd,udp_buffer,sizeof(udp_buffer),MSG_WAITALL,(struct sockaddr *)&server_add_used , &server_addr_len);
                    }
                    if(testing_mode == 1){
                        gettimeofday(&request_delay_test,NULL);
                        delay = (request_delay_test.tv_sec * 1000000 + request_delay_test.tv_usec)-delay;
                    }
                    memset(comm_buffer,'\0',100);
                    print_available_sources_from_server(udp_buffer);
                    printf("Write the ID of the source to subscribe to:");
                    if(fgets(input_buffer,sizeof(input_buffer),stdin)){
                        if(strchr(input_buffer, '\n') == NULL){ //Input is bigger than then allowed buffer size, we must eliminate the remaining characters in the stdin
                            while ((cleaner_helper = getchar()) != '\n' && cleaner_helper != EOF);
                        }
                    }
                    sprintf(comm_buffer,"3|%s|%s|",input_buffer,ID);
                    if(testing_mode == 1){
                        gettimeofday(&request_delay_test,NULL);
                        delay = request_delay_test.tv_sec * 1000000 + request_delay_test.tv_usec;
                    }
                    if (sendto(Info_UDP_Socket_fd, comm_buffer, strlen(comm_buffer), 0, (struct sockaddr * ) & server_addr, sizeof(server_addr)) < 0) {
                        perror("Data couldn't be sent to the server");
                        break;
                    }
                    memset(udp_buffer,'\0',1024);
                    while(udp_buffer[0] != '8'){
                        if(strncmp(udp_buffer,"12",2) == 0){
                            printf("The SM wasn't able to fulfill the subscription request\n");
                            error_message_splitter(udp_buffer);
                            sleep(2);
                            break;
                        }
                        memset(udp_buffer,'\0',1024);
                        recvfrom(Info_UDP_Socket_fd,udp_buffer,sizeof(udp_buffer),MSG_WAITALL,(struct sockaddr *)&server_add_used , &server_addr_len);
                    }
                    if(testing_mode == 1){
                        gettimeofday(&request_delay_test,NULL);
                        delay = (request_delay_test.tv_sec * 1000000 + request_delay_test.tv_usec)-delay;
                    }
                    printf("Source sucessfully subscribed by the SM\n");
                    pthread_mutex_lock(&mutex_sub);
                    int pos = register_source(udp_buffer);
                    pthread_mutex_unlock(&mutex_sub);
                    thread_args args;
                    args.credits_to_disconnect = 0;
                    args.socket_fd_used = Info_UDP_Socket_fd;
                    args.source_position_in_storage = pos;
                    //args.credits_to_disconnect = CreditsToDisconnectMax;
                    args.server_addr = server_add_used;
                    memcpy(args.MY_ID,ID,6);
                    strcpy(args.Source_ID,input_buffer);
                    pthread_t request_thread;
                    int threadCreateResult = pthread_create(&request_thread, NULL, Info_Thread_Handler, (void *)&args);
                    if(threadCreateResult != 0){
                        perror("Couldn't create the source thread");
                    }else{
                        //We detach the thread since we can receive errors which will call the sleep function, causing a lack of credits reset
                        pthread_detach(request_thread);
                    }
                    memset(comm_buffer,'\0',100);
                    break;
                case 3:
                    print_stored_sources();
                    printf("Choose one of the sources which you would like to unsubcribe from:");
                    if(fgets(input_buffer,sizeof(input_buffer),stdin)){
                        if(strchr(input_buffer, '\n') == NULL){ //Input is bigger than then allowed buffer size, we must eliminate the remaining characters in the stdin
                            while ((cleaner_helper = getchar()) != '\n' && cleaner_helper != EOF);
                        }
                    }
                    int unsub_selected_thread = atoi(input_buffer);
                    sprintf(comm_buffer,"4|%s|%s|",subscribed_sources_ID[unsub_selected_thread],ID);
                    if(testing_mode == 1){
                        gettimeofday(&request_delay_test,NULL);
                        delay = request_delay_test.tv_sec * 1000000 + request_delay_test.tv_usec;
                    }
                    if (sendto(Info_UDP_Socket_fd, comm_buffer, strlen(comm_buffer), 0, (struct sockaddr * ) & server_addr, sizeof(server_addr)) < 0) {
                        perror("Data couldn't be sent to the server");
                        break;
                    }
                    memset(comm_buffer,'\0',sizeof(comm_buffer));
                    while(udp_buffer[0] != '9' ){ //Waiting for confirmation that the source was unsubscribed from the server
                        if(strncmp(udp_buffer,"11",2) == 0){
                            printf("The SM wasn't able to fulfill the subscription request\n");
                            error_message_splitter(udp_buffer);
                            sleep(2);
                            break;
                        }
                        memset(udp_buffer,'\0',1024);
                        recvfrom(Info_UDP_Socket_fd,udp_buffer,sizeof(udp_buffer),MSG_WAITALL,(struct sockaddr *)&server_add_used , &server_addr_len);
                        printf("%s\n",udp_buffer);
                    }
                    if(testing_mode == 1){
                        gettimeofday(&request_delay_test,NULL);
                        delay = (request_delay_test.tv_sec * 1000000 + request_delay_test.tv_usec)-delay;
                    }
                    printf("Source sucessfully unsubscribed by the SM");
                    pthread_mutex_lock(&mutex_sub);
                    unsubscribe_from_source(unsub_selected_thread);
                    pthread_mutex_unlock(&mutex_sub);
                    break;
                case 4: //Client selects one of the sources to which he is subscribed to see the data that is arriving to it, if the user attempts to use a position that doesn't exist nothing will happen
                    print_stored_sources();
                    printf("Choose one of the sources to see it's data (position on the list):");
                    if(fgets(input_buffer,sizeof(input_buffer),stdin)){
                        if(strchr(input_buffer, '\n') == NULL){ //Input is bigger than then allowed buffer size, we must eliminate the remaining characters in the stdin
                            while ((cleaner_helper = getchar()) != '\n' && cleaner_helper != EOF);
                        }
                    }
                    selected_thread = atoi(input_buffer);
                    getchar();
                    selected_thread = -1;
                    break;
                case 5:
                    sprintf(comm_buffer,"%s","2|");
                    if (sendto(Request_UDP_Socket_fd, comm_buffer, strlen(comm_buffer), 0, (struct sockaddr * ) & server_addr, sizeof(server_addr)) < 0) {
                        perror("Data couldn't be sent to the server");
                        break;
                    }
                    while(udp_buffer[0] != '6'){
                        memset(udp_buffer,'\0',1024);
                        recvfrom(Request_UDP_Socket_fd,udp_buffer,sizeof(udp_buffer),MSG_WAITALL,(struct sockaddr *)&server_add_used , &server_addr_len);
                    }
                    memset(comm_buffer,'\0',100);
                    print_available_sources_from_server(udp_buffer);
                    printf("Write the ID of the source to subscribe to:");
                    if(fgets(input_buffer,sizeof(input_buffer),stdin)){
                        if(strchr(input_buffer, '\n') == NULL){ //Input is bigger than then allowed buffer size, we must eliminate the remaining characters in the stdin
                            while ((cleaner_helper = getchar()) != '\n' && cleaner_helper != EOF);
                        }
                    }
                    sprintf(comm_buffer,"6|%s|%s|",input_buffer,ID);
                    if (sendto(Info_UDP_Socket_fd, comm_buffer, strlen(comm_buffer), 0, (struct sockaddr * ) & server_addr, sizeof(server_addr)) < 0) {
                        perror("Data couldn't be sent to the server");
                        break;
                    }
                    memset(udp_buffer,'\0',1024);
                    while(udp_buffer[0] != '1'){ //We reuse cases in terms of answers for simplicity sake
                        if(strncmp(udp_buffer,"13",2) == 0){
                            printf("The SM wasn't able to fulfill the source request\n");
                            error_message_splitter(udp_buffer);
                            sleep(2);
                            break;
                        }
                        memset(udp_buffer,'\0',1024);
                        recvfrom(Info_UDP_Socket_fd,udp_buffer,sizeof(udp_buffer),MSG_WAITALL,(struct sockaddr *)&server_add_used , &server_addr_len);
                    }
                    printout_source_info(udp_buffer);
                    getchar();
                    break;
                case 6:
                    sprintf(comm_buffer,"%s","2|");
                    if (sendto(Request_UDP_Socket_fd, comm_buffer, strlen(comm_buffer), 0, (struct sockaddr * ) & server_addr, sizeof(server_addr)) < 0) {
                        perror("Data couldn't be sent to the server");
                        break;
                    }
                    while(udp_buffer[0] != '6'){
                        memset(udp_buffer,'\0',1024);
                        recvfrom(Request_UDP_Socket_fd,udp_buffer,sizeof(udp_buffer),MSG_WAITALL,(struct sockaddr *)&server_add_used , &server_addr_len);
                    }
                    memset(comm_buffer,'\0',100);
                    print_available_sources_from_server(udp_buffer);
                    printf("Write the ID of the source to subscribe to:");
                    if(fgets(input_buffer,sizeof(input_buffer),stdin)){
                        if(strchr(input_buffer, '\n') == NULL){ //Input is bigger than then allowed buffer size, we must eliminate the remaining characters in the stdin
                            while ((cleaner_helper = getchar()) != '\n' && cleaner_helper != EOF);
                        }
                    }
                    sprintf(comm_buffer,"6|%s|%s|",input_buffer,ID);
                    if (sendto(Info_UDP_Socket_fd, comm_buffer, strlen(comm_buffer), 0, (struct sockaddr * ) & server_addr, sizeof(server_addr)) < 0) {
                        perror("Data couldn't be sent to the server");
                        break;
                    }
                    memset(udp_buffer,'\0',1024);
                    while(udp_buffer[0] != '1'){ //We reuse cases in terms of answers for simplicity sake
                        if(strncmp(udp_buffer,"13",2) == 0){
                            printf("The SM wasn't able to fulfill the source request\n");
                            error_message_splitter(udp_buffer);
                            sleep(2);
                            break;
                        }
                        memset(udp_buffer,'\0',1024);
                        recvfrom(Info_UDP_Socket_fd,udp_buffer,sizeof(udp_buffer),MSG_WAITALL,(struct sockaddr *)&server_add_used , &server_addr_len);
                    }
                    printout_source_info(udp_buffer);
                    getchar();
                    break;
                default:
                    printf("No valid option selected\n");
                    usleep(700000);
                    break;
            }
            fflush(stdin);
            memset(input_buffer,'\0',sizeof(input_buffer));
        }else{
            perror("It appears that you typed something that was invalid in terms of size\n");
        }
    }
}