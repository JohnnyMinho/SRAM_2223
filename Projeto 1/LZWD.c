#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define basic_block_size 65536
#define num_filhos 256
#define dictionary_limit 65536
//Macros
#define CHAR_TO_INT(c) ((unsigned int)c)
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
//-----

//Estruta de dados usado, trie.

int indice_global = 1; //Também pode ser usado para controlar o tamanho do dicionário
int full_dictionary = 0; //We make it global for a simplicity sake
int tamanho_total_padroes = 0;
int num_filhos_a_libertar = 0;
int added_patterns = 0;
int pattern_adding_attempts = 0;
long total_num_of_bits = 0;
long total_num_of_chars = 0;

struct trie_node{
    int e_leaf; //This one tells when the pattern that was read when arriving at the node is a full pattern
    int dados;
    int indice;
    int output;
    int searched;
    struct trie_node *filhos[num_filhos];
};

struct statistics{
    int average_pattern_size;
    int total_repeated_patterns_added;
    int total_new_patterns;
    int total_patterns_added;
};

struct file_info{
    size_t file_size;
    int num_of_blocks;
    unsigned long size_of_last_block;
};

void bits_per_output(unsigned int num) {
    int comparator = 256;
    int tester = 0;
    int counter = 1;
    int initial_bit_value = 8;
    //int n = num;
    /*while (num) {
        num = num >> 1;
        total_num_of_bits++;
    }*/
    while(num >= comparator){ //This should be able to group in valid char values since we are forced to start at 8 bits to form an entire char
        comparator = comparator*2;
        initial_bit_value++;
    }
    total_num_of_bits = total_num_of_bits+initial_bit_value;
    //total_num_of_bits = total_num_of_bits + (int)log2(num)+1; //Gives us the base + x0 bit;
}

int commonDivisor(unsigned long n, unsigned long m) {
    int gcd, remainder;

    while (n != 0)
    {
        remainder = m % n;
        m = n;
        n = remainder;
    }

    gcd = m;
    return gcd;
}

int getsize_of_array(unsigned char* array_to_check){
    int i = 0;
    //printf("%s\n",array_to_check);
    while(array_to_check[i]!=NULL){
        i++;
    }
    i++; //Inclui a posição de encerramento do array
    return i;
}

struct trie_node* criar_node(unsigned char data,int output_a_colocar){
    struct trie_node* novo_node = (struct trie_node*)calloc(1, sizeof(struct trie_node));
    novo_node -> dados = CHAR_TO_INT(data);
    novo_node -> output = output_a_colocar;
    novo_node -> indice = indice_global;
    //novo_node->filhos = (trie_node**) malloc(n_filhos * sizeof(trie_node*));
    for (int i=0; i<num_filhos; i++){
        novo_node->filhos[i] = NULL;
    //novo_node->num_filhos = 0;
    }
    novo_node->e_leaf = 0;
    return novo_node;
}

void mostrar_est(){ 

}

void print_time(){
    time_t current_time;
    struct tm *local_time;
    char time_string[80];

    current_time = time(NULL);

    local_time = localtime(&current_time);

    strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", local_time);

    printf("%s\n", time_string);
}
/*bool reached_eof(char* str){
    for(int i = 0; str[i] != '\0'; i++){
        if(str[i] == EOF){
            return true;
        }
    }
}*/

void libertar_nodes(struct trie_node* node, int n_filhos){
    //printf("%d\n",node->dados);
    for(int i=0; i<n_filhos; i++) {
        if (node->filhos[i] != NULL) {
            libertar_nodes(node->filhos[i],n_filhos);
            free(node->filhos[i]);
        }
        else {
            return;
        }
    }
}

int encontrar_pos_livre(struct trie_node* node,unsigned char* padrao){
    //printf("À procura de posições\n");
    for(int i = 0; i< num_filhos;i++){
        if(node->filhos[i] != NULL){
            //printf("Encontrei uma posição livre: %d\n",i);
        }else{
            /*if(memcmp(node->filhos[i]->dados,padrao,getsize_of_array(padrao)) == 0 ){
            return -1;
            }*/
            
            return i;
        }
    }
    return -1;
}

//Searches for a pattern match in the trie, if it exists, it returns a pointer to the memory position. If no pattern is found, NULL is used to indicate that
//This can be used to confirm if a pattern exists, but also to find the correct position to add it to the trie.
struct trie_node* procurar_na_trie_total(struct trie_node* root,unsigned char* padrao,int size) {
    struct trie_node* temp_root = root;
    int padrao_size = size;
    int pattern_found = 0; //If the pattern is found we use this to indicate that. If the pattern isn't found after the entire search of the sons array, we can use it to return NULL
    int char_to_search = 0;
    int no_more_sons_to_search = 0;
    for (int level_counter = 0; level_counter < padrao_size; level_counter++) {
        pattern_found = 0;
        no_more_sons_to_search = 0;
        char_to_search = CHAR_TO_INT(padrao[level_counter]);
        
        for(int index_counter = 0; index_counter < num_filhos && pattern_found != 1 && no_more_sons_to_search != 1;index_counter++){
            if (temp_root->filhos[index_counter] != NULL) {
                int son_data = temp_root->filhos[index_counter]->dados;
                if (son_data == char_to_search){
                    if(level_counter+1 == padrao_size){
                        //We reached the end of the pattern. We can return the pointer of the tail of the pattern that was searched
                        temp_root = temp_root->filhos[index_counter];
                        return temp_root;
                    }else{
                        temp_root = temp_root->filhos[index_counter];
                        pattern_found = 1;
                    }
                }
            }else{
                no_more_sons_to_search = 1;
            }
        }
        if(temp_root->e_leaf == 1 && level_counter+1 == padrao_size && pattern_found != 1){
            //printf("Test_it'sanull\n");
            return NULL;
        }
    }
    //printf("NULL returned\n");
    return NULL;
}

//Adapted search function to insert new patterns into the dictionary.
void inserir(struct trie_node* root,unsigned char* padrao,int outputs_novo,int size){
    //printf("Test\n");
    struct trie_node *point_to_new_node = root;
    int padrao_size = size;
    int pattern_added = 0;
    int char_to_search = 0;
    //printf("Padrão chegado -> %s\n",padrao);
    //printf("Pattern size -> %d\n",padrao_size);
    for (int level_counter = 0; level_counter < padrao_size && pattern_added != 1; level_counter++){
        //printf("level_counter->%d\n",level_counter);
        //printf("Pai->%c\n",point_to_new_node->dados);
        char_to_search = CHAR_TO_INT(padrao[level_counter]);
        for(int index_counter = 0; index_counter < num_filhos && pattern_added != 1;index_counter++){
            //printf("index_counter->%d\n",index_counter);
            if (point_to_new_node->filhos[index_counter] != NULL) {
                //printf("Test_No_NULL\n");
                int son_data = point_to_new_node->filhos[index_counter]->dados;
                if (son_data == char_to_search) {
                    //printf("Equal_son -> %c\n",son_data);
                    point_to_new_node = point_to_new_node->filhos[index_counter];
                    pattern_added = 1;
                    if(level_counter+1 == padrao_size){
                        //If the new pattern already had it's full content inserted for some reason, it just updates the e_leaf stat 
                        point_to_new_node->e_leaf = 1;
                    }
                }
            }else{
                if(level_counter+1 == padrao_size){
                    point_to_new_node->filhos[index_counter] = criar_node(padrao[level_counter],outputs_novo);
                    point_to_new_node->filhos[index_counter]->e_leaf = 1;
                    point_to_new_node = point_to_new_node->filhos[index_counter];
                    indice_global++;
                    //sleep(1);
                    pattern_added = 1;
                }
            }
        }
        if(level_counter+1 != padrao_size){
            pattern_added = 0;
        }
    }
}
void inserir_new_dic(struct trie_node* root,unsigned char* padrao,int size){
    //printf("Test\n");
    struct trie_node *point_to_new_node = root;
    int padrao_size = size;
    int pattern_added = 0;
    int char_to_search = 0;
    int outputs_novo = indice_global;
    //printf("Padrão chegado -> %s\n",padrao);
    //printf("Pattern size -> %d\n",padrao_size);
    for (int level_counter = 0; level_counter < padrao_size && pattern_added != 1; level_counter++){
        //printf("level_counter->%d\n",level_counter);
        //printf("Pai->%c\n",point_to_new_node->dados);
        char_to_search = CHAR_TO_INT(padrao[level_counter]);
        for(int index_counter = 0; index_counter < num_filhos && pattern_added != 1;index_counter++){
            //printf("index_counter->%d\n",index_counter);
            if (point_to_new_node->filhos[index_counter] != NULL) {
                //printf("Test_No_NULL\n");
                int son_data = point_to_new_node->filhos[index_counter]->dados;
                if (son_data == char_to_search) {
                    //printf("Equal_son -> %c\n",son_data);
                    point_to_new_node = point_to_new_node->filhos[index_counter];
                    pattern_added = 1;
                    if(level_counter+1 == padrao_size){
                        //If the new pattern already had it's full content inserted for some reason, it just updates the e_leaf stat 
                        point_to_new_node->e_leaf = 1;
                    }
                }
            }else{
                if(level_counter+1 == padrao_size){
                    point_to_new_node->filhos[index_counter] = criar_node(padrao[level_counter],outputs_novo);
                    point_to_new_node->filhos[index_counter]->e_leaf = 1;
                    point_to_new_node = point_to_new_node->filhos[index_counter];
                    indice_global++;
                    //sleep(1);
                    pattern_added = 1;
                }else{
                    point_to_new_node->filhos[index_counter] = criar_node(padrao[level_counter],outputs_novo);
                    point_to_new_node = point_to_new_node->filhos[index_counter];
                    indice_global++;
                    //sleep(1);
                    pattern_added = 1;
                }
            }
        }
        if(level_counter+1 != padrao_size){
            pattern_added = 0;
        }
    }
}

void lzwd(struct trie_node* root,unsigned char* bloco, int dic_limit,struct statistics *statistics_store, FILE* outputfile, int dic_mode, int max_size){
    //Optimization 1-> Reduce number of reallocs, theoretically we can't have patterns bigger than the max number of sons in a trie, so if we define an array with just 256 in space for
    //Pa and Pb and one that is at least
    //printf("Test->%d\n",max_size);
    struct trie_node* temp = root;
    struct trie_node* temp_Pb = root; //Precisamos de um extra para ser usado para a procura do Pb
    struct trie_node* temp_final = root;
    int i = 0;
    int filho_pa_encontrado = -1;
    int filho_pb_encontrado = -2;
    int pos_valida = 0;
    int output = 0;
    int primeira_it = 1;
    int pos_avancar = 1;
    int pos_avancar_pa =1;
    int preciso_encontrar_temp_Pb = 0;
    int maximo_buffer_indices = 3;
    int dictionary_fill_counter = 0;
    int total_pattern_size = 0;
    int pos_indices_recentes_colocados = 0;
    int output_anterior = 0;
    int Pa_size = 1;
    int Pb_size = 1;
    int Pb_Pa_size = 0;
    
    unsigned char* Pb_final = (unsigned char*)malloc((int)(round(max_size/2))*sizeof(unsigned char));
    unsigned char* Pa = (unsigned char*)malloc((int)(round(max_size/2))*sizeof(unsigned char));
    unsigned char* Pb = (unsigned char*)malloc((int)(round(max_size/2))*sizeof(unsigned char));
    unsigned char* pattern_to_dictionary = (unsigned char*)malloc(max_size*sizeof(unsigned char)); //Padrão que vai ser adicionado ao dicionário. Este Aumenta o seu tamanho consoante o aumento de Pa ou Pb
    memcpy(Pa, bloco,1);
    int block_end = 0;
    //For extreme edge cases where the block only contains 1 byte we include a condition that the max_size of the array must be greater than 1
    if(max_size <= 2){
        //If the block only has one char the output will always be 0 since in most of the cases tested the block was simply a NULL
        if(max_size == 1){
            fprintf(outputfile,"0,");
        }else{
            Pa[0] = bloco[0];
            temp = procurar_na_trie_total(root,Pa,Pa_size);
            fprintf(outputfile,"%d,",temp->indice);
        }
    }
    for(i = 0; i<max_size && block_end == 0 && max_size > 2; i++){

        int index_ascii = (int) bloco[i];
        int a = i+1;
        
        memcpy(Pb, (bloco+a), 1);
        //PA processing part
        if(primeira_it == 1){
            pattern_to_dictionary[0] = Pa[0];
            temp = root->filhos[index_ascii];
            primeira_it = 0;
            output = temp->indice;
        }else{
            temp = temp_final;
            output = temp->indice;
        }
        pos_avancar_pa = Pa_size;
        
        // -------------------------------------------------------------
        if(pos_avancar_pa >= 2){
            preciso_encontrar_temp_Pb = 1;
            i = i+pos_avancar_pa-1;
            pos_avancar = 1;
        }else{
            pos_avancar = 1;
        }
        //Pb processing part
        while(filho_pb_encontrado != -1 && block_end != 1){
            if(pos_avancar!=1){
                Pb[pos_avancar]='\0';
                memcpy(Pb,bloco+i+1,pos_avancar);
                
                preciso_encontrar_temp_Pb = 0;
            }
            
            if(preciso_encontrar_temp_Pb == 1){
                memcpy(Pb,bloco+i+1,pos_avancar);
                preciso_encontrar_temp_Pb = 0;
            }

            temp_Pb = procurar_na_trie_total(root,Pb,Pb_size);
            
            if(temp_Pb != NULL){
                int tamanho_Pb = Pb_size;
                filho_pb_encontrado = temp_Pb->indice;
                temp_final = temp_Pb;
                Pb_Pa_size = Pb_size;
                memcpy(Pb_final,Pb,(tamanho_Pb));
            }else{
                filho_pb_encontrado = -1;
            }
            
            if((i+Pb_size+1) >= max_size){
                block_end = 1;
            }else{

            }
            //Pattern insertion start
            if(filho_pb_encontrado != -1){
                
                int tamanho_Pa = Pa_size;
                int tamanho_Pb = Pb_size;
                int final_size = tamanho_Pa+tamanho_Pb;

                memcpy(pattern_to_dictionary,Pa,(Pa_size));
                memcpy(pattern_to_dictionary+(tamanho_Pa),Pb,(tamanho_Pb));
                struct trie_node* NP_A_PROCURAR = procurar_na_trie_total(root,pattern_to_dictionary,final_size);

                if(NP_A_PROCURAR != NULL){
                    //There aren't any patterns that are equal to this one
                }else{
                    //The pattern is valid and it isn't already present at the current dictionary
                    
                    if(statistics_store->total_new_patterns <= dic_limit){
                        inserir(root,pattern_to_dictionary,temp->indice,final_size);
                        statistics_store->total_new_patterns++;
                    }else{
                        //printf("FULL DIC\n");
                    }
                    total_pattern_size += (final_size-2); //We need to remove the two closing bytes present in the Pa and Pb size
                    if(dic_mode == 3 && statistics_store->total_new_patterns == dic_limit){
                        full_dictionary = 1;
                    }
                }
                
                if(output_anterior != output){
                    fprintf(outputfile,"%d,",temp->indice);
                    statistics_store->total_patterns_added++;
                    added_patterns++;
                    output_anterior = output;
                    bits_per_output(temp->indice);
                    if(temp_Pb->indice == output){
                        output_anterior = 0; //Edge-case where this might occur
                    }
                }else{

                }
                pattern_adding_attempts++;
                Pb_size++;
            }
            pos_avancar++;
        }
        memcpy(Pa,Pb_final,(Pb_Pa_size+1));
        Pa_size = Pb_Pa_size;
        
        //---------------------------------
        temp_Pb = root;
        
        filho_pb_encontrado = -2;
        pos_avancar_pa = 1;
        if((i+Pb_size+1) >= max_size){
            block_end = 1;
            
        }
        Pb_size = 1;
        
        pos_indices_recentes_colocados = 0;
    }
    if(max_size >1){
        temp = procurar_na_trie_total(root,Pa,Pa_size);
        fprintf(outputfile,"%d,",temp->indice);
    }
    free(Pa);
    free(Pb);
    free(Pb_final);
    free(pattern_to_dictionary);
    
    tamanho_total_padroes += total_pattern_size;
    //temp -> e_leaf = 1;
}

struct trie_node* criar_dicionario(struct trie_node* root){ //Funciona como esperado
    for(int i = 0; i < 256; i++){
        unsigned char caracter = (unsigned char)i;
        root->filhos[i] = criar_node(caracter,0);
        indice_global++;
    }
    return root;
}

struct trie_node* criar_dicionario_fromfile(struct trie_node* root,FILE* file){
    //This function was mode for language based dictionaries like 1000 most words of portuguese, english, etc.
    //The dictionaries should be formatted in such a way that each word or character is followed by a newline
    //If possible add common special characters like spaces and such
    //To avoid long periods to form dictionaries use smaller ones 
    struct stat st;
    int fd = fileno(file);
    int number_of_chars_read = 0;
    int last_number_read = 0;
    fstat(fd,&st);
    struct trie_node* temp_root = root;
    size_t dictionary_full_size = st.st_size;
    unsigned char buffer_for_dic[dictionary_full_size+1];
    unsigned char delimiter = 10;
    fread(buffer_for_dic,sizeof(unsigned char),dictionary_full_size,file);
    while(number_of_chars_read < dictionary_full_size+1){
        while(buffer_for_dic[number_of_chars_read] != delimiter){
            number_of_chars_read++;
        }
        unsigned char temp_buffer[number_of_chars_read-last_number_read];
        memcpy(temp_buffer,buffer_for_dic+last_number_read,(number_of_chars_read-last_number_read));
        inserir_new_dic(root,temp_buffer,(number_of_chars_read-last_number_read));
        //printf("%s\n",temp_buffer);
        last_number_read = number_of_chars_read;
    }
    return root;
}

int main(int argc, char *argv[]) {
    int dicionario_base = 1; //Dicionário é o báscio (Tabela ASCII), se não for ativada a opção de um dicionário custom | 0 -> Dicionário Custom | 1 -> Dicionário Base 
    unsigned char content_raiz = '\0';
    struct trie_node* raiz_original = criar_node(content_raiz,0);
    struct file_info File_info;
    struct statistics statistics_store;
    struct statistics* statistics_ptr = &statistics_store;
    int fd;
    int outputfd;
    int dic_limit = 0;
    int chars_read = 0;
    int dic_mode = 1; //default mode
    int iterations = 0;
    //bool end_file = false;
    int nread = 0;
    unsigned long block_size;
    int num_of_blocks = 0;
    FILE *Dic_file;
    if(argc > 3){ //Existem argumentos para além da exc da app e o nome do ficheiro a comprimir, logo têmos de verificar que argumentos são usados, para já isto não vai fazer nada
        switch(argc){
            case 4:
                //Tamanho dos blocos
                block_size = atoi(argv[3]);
                printf("%s\n",argv[3]);
                //printf("%d\n",block_size);
                break;
            case 5:
                block_size = atoi(argv[3]);
                dic_limit = atoi(argv[4]);
                //Outra op
                break;
            case 6:
                //Dictionary erasure mode 1-> Everytime we go to a new block, 2-> Keep it until we read the entire fire, 3-> reset once it is full
                block_size = atoi(argv[3]);
                dic_limit = atoi(argv[4]);
                dic_mode = atoi(argv[5]);
                if(dic_mode < 1 || dic_mode >3){
                    perror("This dictionary erasure option is not allowed");
                    exit(-1);
                }
                //Outra op
                break;
            case 7: 
                block_size = atoi(argv[3]);
                dic_limit = atoi(argv[4]);
                dic_mode = atoi(argv[5]);
                if(dic_mode < 1 || dic_mode >3){
                    perror("This dictionary erasure option is not allowed");
                    exit(-1);
                }
                Dic_file = fopen(argv[6],"rb"); //We use a format were each word to be added is separated by commas
                if(Dic_file == NULL){
                    perror("This dictionary file is invalid");
                    exit(-1);
                }
            default:
                break;
            //Se for preciso mais pomos mais
        }
    }else{
        block_size = basic_block_size;
        dic_limit = dictionary_limit;
    }
    
    indice_global = 0;
    FILE* file = fopen(argv[1], "rb");
    FILE* outputfile = fopen(argv[2],"w");
    //Obtain the filedescriptor of the opened file to use on fstat
    fd = fileno(file);
    outputfd = fileno(outputfile);
    if (fd == -1 || outputfd == -1) {
        perror("Error while retrieving the file descriptor for the file to be encoded or the output file");
        fclose(file);
        return 1;
    }
    struct stat st;
    fstat(fd,&st);
    File_info.file_size = st.st_size;
    //Treat the file to learn about how many blocks are there to process but to also limit the size of the last block that has to be read
    if(File_info.file_size%block_size != 0){
        unsigned int size_compare = 0;
        while(File_info.file_size > size_compare+block_size){
            size_compare += block_size;
            num_of_blocks++;
        }
        int last_block_size = File_info.file_size - size_compare;
        File_info.size_of_last_block = last_block_size;
        num_of_blocks++;
    }
    
    File_info.num_of_blocks = num_of_blocks;
    //Initial report on the file and program
    printf("Program authors: João Moura A93099 Vitor Sá A88606 \nServiços de Rede & Aplicações Multimédia \nCompressor LZWd \n");
    printf("Size of the file to apply LZWd on: %ld\n",File_info.file_size);
    printf("Number of blocks to encode:%d\n",File_info.num_of_blocks);
    printf("Size of block used to partition the file: %ld\n",block_size);
    printf("Dictionary used: Full 256 ASCII table\n");
    printf("Name of file to apply LZWd on, %s \n",argv[1]);
    printf("Name of output file, %s \n",argv[2]);
    printf("Type of dictionary clearing used: %d\n",dic_mode); //Dictionary erasure mode 1-> Everytime we go to a new block, 2-> Keep it until we read the entire fire, 3-> reset once it is full
    printf("Start of the encoding:");
    print_time();
    //End of report
    long tamanho_ultimo;
    if(dic_mode != 1){
        raiz_original = criar_dicionario(raiz_original);
    }
    while(nread < File_info.num_of_blocks){
        if(nread == File_info.num_of_blocks){
            block_size = File_info.size_of_last_block;
        }
        if(File_info.num_of_blocks == 1){
            block_size = File_info.size_of_last_block;
        }
        if(dic_mode != 1){
            if(dic_mode == 3 && full_dictionary == 1){
                full_dictionary = 0;
                free(raiz_original);
                struct trie_node* raiz_original = criar_node(content_raiz,0);
                raiz_original = criar_dicionario(raiz_original);
                statistics_store.total_new_patterns = 0;
                statistics_store.total_repeated_patterns_added = 0;
            }
            unsigned char *text_block = (unsigned char*)malloc((block_size+1)*sizeof(unsigned char));
            text_block[block_size]='\0';
            fseek(file,nread*block_size,SEEK_SET);
            int bloco_n = fread(text_block,sizeof(unsigned char), block_size,file);
            
            nread++;
            chars_read += block_size;
            lzwd(raiz_original,text_block,dic_limit,statistics_ptr,outputfile,dic_mode,block_size);
            indice_global = 0;
            free(text_block);
        }else{
            struct trie_node* raiz = criar_node(content_raiz,0);
            raiz = criar_dicionario(raiz);
            /*if(Dic_file == NULL){
                raiz = criar_dicionario(raiz);
            }else{
                raiz =  criar_dicionario_fromfile(raiz,Dic_file);
            }*/
            unsigned char *text_block = (unsigned char*)malloc((block_size+1)*sizeof(unsigned char));
            text_block[block_size]='\0';
            fseek(file,nread*block_size,SEEK_SET);
            int bloco_n = fread(text_block,sizeof(unsigned char), block_size,file);
            nread++;
            chars_read += block_size;
            lzwd(raiz,text_block,dic_limit,statistics_ptr,outputfile,dic_mode,block_size);
            indice_global = 0;
            statistics_store.total_new_patterns = 0;
            libertar_nodes(raiz,num_filhos);
            free(raiz);
            free(text_block);
        }
    }
    fclose(file);
    printf("End of enconding:");
    print_time();
    fstat(outputfd,&st);
    printf("Statistics*********\n");
    long outputfile_size = st.st_size-added_patterns;
    float average_pattern_size = (float)tamanho_total_padroes/(float)added_patterns;
    printf("Average size of patterns: %d\n",(int)round(average_pattern_size));
    printf("true average-> %f\n",average_pattern_size);
    //printf("Output file size:%ld\n",st.st_size);
    //printf("Size of the compressed file: (por caracter de código): %ld\n",total_num_of_chars);
    printf("Size of the compressed file: %ld\n", total_num_of_bits / 8);
    printf("Total amount of patterns formed:%d\n",added_patterns);
    printf("How many patterns where tried to be added:%d\n",pattern_adding_attempts);
    printf("How many attemtps failed:%d\n",pattern_adding_attempts-added_patterns);
    float compression_rate = ((float)st.st_size)/(float)File_info.file_size;
    int commonDivisor_value = commonDivisor(st.st_size,File_info.file_size);
    printf("%d\n",commonDivisor_value);
    printf("Compression ratio->%ld%%\n",100 - (((total_num_of_bits / 8) * 100) / File_info.file_size));
    //printf("Compression Ratio simplified->%ld:%ld\n",(File_info.file_size/commonDivisor_value),(st.st_size/commonDivisor_value));
    printf("*****End of Statistics\n");
    /*printf("Repeated->%d\n",statistics_store.total_repeated_patterns_added);
    printf("New->%d\n",statistics_store.total_patterns_added);
    ;*/
    fclose(outputfile);
    /*if(Dic_file != NULL){
        fclose(Dic_file);
    }*/
  
    return 0;
}