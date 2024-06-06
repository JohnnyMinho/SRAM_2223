#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <arpa/inet.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

typedef struct{
    char Fonte_Tipo[10];
    char Identificador[10];
    char Periodo[10];
    char Frequencia[10];
    char Num_Amostras[10];
    char Val_amostra[10];
    char Val_amostra_cod[10];
    char Periodo_Max[10];
}PDU;

#define VAL_PI 3.141592653589793
int interrupted_by_CtrlC = 0;
int id_to_Remove_holder = 0;

long getfile_length(FILE* file){
    fseek(file,0,SEEK_END);
    long length = ftell(file);
    fseek(file,0,SEEK_SET);
    return length;
}

//We use this function to generate random identifiers for each instance of data producers
int New_Id_finder(FILE* file){
    srand(time(NULL));
    int new_id = (rand() % (999999999 - 100000000 + 1)) + 100000000;
    int pos = 0;
    char buffer[10];
    buffer[10] = 0;
    while(fread(buffer,sizeof(char),9,file) > 0){
        
        if(atoi(buffer) == new_id){
            fseek(file,0,SEEK_SET);
            pos = 0;
            new_id = (rand() % (999999999 - 100000000 + 1)) + 100000000;
        }
        pos+=9;
        fseek(file,pos,SEEK_SET);
    }
    //fwrite(file,sizeof(char),9,buffer);
    fprintf(file,"%d",new_id);

    return new_id;
}

void remove_ID(int Id_to_remove, FILE* file,long file_length){
    int pos = 0;
    char buffer[10];
    int test = 0;
    buffer[10] = '\0';
    while(test = fread(buffer,sizeof(char),9,file) > 0){ 
        if(atoi(buffer) == Id_to_remove){
            pos+=9;
            fseek(file,pos,SEEK_SET);
            if(pos<file_length){
                while(fread(buffer,sizeof(char),9,file) > 0){
                    fprintf(file-9,"%s",buffer);
                }
            }else{
                //printf("H\n");
                ftruncate(fileno(file),9);
            }
        }

        pos+=9;
        fseek(file,pos,SEEK_SET);
    }
}

void ControlCHandler(int sig_num){
    interrupted_by_CtrlC = 1;
    printf("MANUALLY INTERRUPTED\n");
}

int enconde_sample(int i, int N,int encoding_type){
    int amostra_cod = 0;
    if(encoding_type == 1){
        //Sin wave
        float sin_calculation_value = sin((2*VAL_PI*i)/N);
        amostra_cod = (int)(1+(1+sin_calculation_value)*30);
    }
    if(encoding_type == 2){
        //Square wave
        int square_wave_value = 0;
        float sin_calculation_value = sin((2*VAL_PI*i)/N);
        if(sin_calculation_value > 0.0){
            square_wave_value = 1;
        }else{
            square_wave_value = 0;
        }
        amostra_cod = (int)((square_wave_value)*30);
    }
    if(encoding_type == 3){
        //Triangular wave
        float sin_calculation_value = sin((2*VAL_PI*i)/N);
        float triangular_wave_value = 2 * fabs(sin_calculation_value - floor(sin_calculation_value + 0.5)) - 1;
        amostra_cod = (int)(1+(1+triangular_wave_value)*30);
    }
    return amostra_cod;
}

int main(int argc,char *argv[]){

    signal(SIGINT,ControlCHandler);

    int server_ip_size = 0;
    int server_port_size = 0;
    int source_type = 0;
    if(argc>5){
        server_ip_size = strlen(argv[3]);
        server_port_size = strlen(argv[4]);
    }
    char Server_IP[server_ip_size];
    char Server_Port[server_port_size];
    int Freq_amostragem = 0;
    int N = 0;
    int Period = 0;
    int Period_Max = 0;
    char *Max_Period_ptr;
    char* PDU_pointers[10];
    PDU PDU1;
    FILE* id_file = fopen("Ids_Fonte.txt","a+");
    if(id_file == NULL){
        perror("Failed to open id config file");
        exit(3);
    }

    int new_id = New_Id_finder(id_file);
    fseek(id_file,0,SEEK_SET);
    long file_length = getfile_length(id_file);
    printf("%d\n",new_id);

    PDU1.Fonte_Tipo[10] = '\0';
    PDU1.Identificador[10] = '\0';
    PDU1.Periodo[10] = '\0';
    PDU1.Frequencia[10] = '\0';
    PDU1.Num_Amostras[10] = '\0';
    PDU1.Val_amostra[10] = '\0';
    PDU1.Val_amostra_cod[10] = '\0';
    PDU1.Periodo_Max[10] = '\0';

    sprintf(PDU1.Identificador,"%d",new_id);

    if(argc>5){ //argv[1] -> F ,argv[2]->N, argv[3]-> IP, argv[4]-> UDP, argv[5]-> Período Máximo
        memcpy(PDU1.Frequencia,argv[1],strlen(argv[1]));
        PDU1.Frequencia[strlen(argv[1])] = '\0';
        memcpy(PDU1.Num_Amostras,argv[2],(strlen(argv[2])));
        PDU1.Num_Amostras[strlen(argv[2])] = '\0';
        Server_IP[strlen(argv[3])] = '\0';
        memcpy(Server_IP,argv[3],strlen(argv[3]));
        Server_Port[strlen(argv[4])] = '\0';
        memcpy(Server_Port,argv[4],strlen(argv[4]));
        memcpy(PDU1.Periodo_Max,argv[5],strlen(argv[5]));
        Period_Max = strtol(argv[5],&Max_Period_ptr,10); //It is trusted that the user inputs a valid number
        source_type = atoi(argv[6]);
    }else{
        perror("Number of arguments should be 6, frequency , number of samples, IP, Port, Maximum Period ");
        exit(EXIT_FAILURE);
    }
    
    N = atoi(PDU1.Num_Amostras);
    sprintf(PDU1.Fonte_Tipo,"%d",source_type);
    Freq_amostragem = round(atoi(PDU1.Frequencia) * N);


    //Socket configuration
   
    int UDP_Socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    
    struct sockaddr_in server_addr;
    //socklen_t ; 
    if (UDP_Socket_fd  < 0){
        remove_ID(new_id,id_file,file_length);
        perror("Failure to open an UDP socket, closing");
        exit(-1);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(Server_Port));
    server_addr.sin_addr.s_addr = inet_addr(Server_IP);
    sleep(1);
    if (inet_pton(AF_INET, Server_IP, & (server_addr.sin_addr)) <= 0) {
        perror("The IP in use is not valid");
        exit(-1);
    }
    //PDU filling and sending (infinite for cycle for simplicity sake), | is used to separate the PDU values because it is outside the ASCII values generated for the encoded samples
    char concat_PDU[70];
    concat_PDU[70] = '\0';
    for(int i = 0; interrupted_by_CtrlC != 1 ;i++){
        memset(concat_PDU,0,70);
        sprintf(PDU1.Val_amostra, "%d", i);
        snprintf(PDU1.Periodo,10,"%d",Period);
        int amostra_cod = enconde_sample(i,N,source_type);
        sprintf(PDU1.Val_amostra_cod, "%d", amostra_cod);
        strcat(concat_PDU,"1");
        strcat(concat_PDU,"|");
        strcat(concat_PDU,PDU1.Fonte_Tipo);
        strcat(concat_PDU,"|");
        strcat(concat_PDU,PDU1.Identificador);
        strcat(concat_PDU,"|");
        strcat(concat_PDU,PDU1.Val_amostra);
        strcat(concat_PDU,"|");
        strcat(concat_PDU,PDU1.Val_amostra_cod);
        strcat(concat_PDU,"|");
        strcat(concat_PDU,PDU1.Periodo);
        strcat(concat_PDU,"|");
        strcat(concat_PDU,PDU1.Frequencia);
        strcat(concat_PDU,"|");
        strcat(concat_PDU,PDU1.Num_Amostras);
        strcat(concat_PDU,"|");
        strcat(concat_PDU,PDU1.Periodo_Max);
        strcat(concat_PDU,"|");
        
        printf("PDU Generated-> %s\n",concat_PDU);

        if (sendto(UDP_Socket_fd, concat_PDU, strlen(concat_PDU), 0, (struct sockaddr * ) & server_addr, sizeof(server_addr)) < 0) {
            perror("Data couldn't be sent to the server");
            break;
        }
        //Restoring values to keep things working
        if(i == Freq_amostragem){
            i = 0;
        }
        if(Period == Period_Max){
            Period = 1;
        }else{
            Period++;
        }
        //To avoid overwhelming the server a delay equivalent to the period per UDP datagram is set
        sleep(1/Freq_amostragem);
    } 

    remove_ID(new_id,id_file,file_length);
    fclose(id_file);
    close(UDP_Socket_fd);
    
    return 0;
}