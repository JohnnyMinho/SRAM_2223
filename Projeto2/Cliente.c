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

void print_menu(char *buffer){
    printf("Multimedia Client MENU\n");
    printf("1-> Get media sources listing\n");
    printf("2-> Unsubscribe from source\n");
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

int main(int argc,char* argv[]){

    int SM_Port;
    char *SM_IP_address;
    char input_buffer[3];
    input_buffer[3] = '\0';
    char udp_buffer[1024];
    int chosen_option;
    char cleaner_helper;
    char comm_buffer[10];

    if(argc>2){
        SM_IP_address = (char *)malloc(sizeof(char)*strlen(argv[1]));
        memcpy(SM_IP_address,argv[1],strlen(argv[1]));
        SM_Port = atoi(argv[2]);
    }

    int UDP_Socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    
    struct sockaddr_in server_addr;

    if (UDP_Socket_fd  < 0){
        perror("Failure to open an UDP socket, closing");
        exit(-1);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SM_Port);
    server_addr.sin_addr.s_addr = inet_addr(SM_IP_address);
    sleep(1);
    if (inet_pton(AF_INET, SM_IP_address, & (server_addr.sin_addr)) <= 0) {
        //remove_ID(new_id,id_file,file_length);
        perror("The IP in use is not valid");
        exit(-1);
    }

    while(1){
        print_menu(udp_buffer);
        if(fgets(input_buffer,sizeof(input_buffer),stdin)){
            if(strchr(input_buffer, '\n') == NULL){ //Input is bigger than then allowed buffer size, we must eliminate the remaining characters in the stdin
                while ((cleaner_helper = getchar()) != '\n' && cleaner_helper != EOF);
            }
            chosen_option = atoi(input_buffer);
            switch (chosen_option){
                case 1:
                    sprintf(comm_buffer,"%s","2|");
                    if (sendto(UDP_Socket_fd, comm_buffer, strlen(comm_buffer), 0, (struct sockaddr * ) & server_addr, sizeof(server_addr)) < 0) {
                        perror("Data couldn't be sent to the server");
                        break;
                    }
                    recv(UDP_Socket_fd,udp_buffer,sizeof(udp_buffer),MSG_WAITALL);
                    printf("%s",udp_buffer);
                    sleep(3);
                    memset(comm_buffer,'\0',100);
                    break;
                
                default:
                    printf("No valid option selected\n");
                    usleep(700000);
                    break;
            }
            clear_console();
            fflush(stdin);
            memset(input_buffer,'\0',sizeof(input_buffer));
        }else{
            perror("It appears that you typed something that was invalid in terms of size\n");
        }
    }
}