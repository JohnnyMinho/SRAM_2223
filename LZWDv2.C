#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>

//A procura do padrão dentro do dicionário é o que leva mais tempo , usar técnicas estilo hashtable
//Não vale a pena usar a previsão do tamanho do ficheiro

// Precisamos de uma função para dividir a informação lida do ficheiro de origem em blocos 
// Precisamos de uma função para enviar a informação em blocos para o ficheiro final
// Precisamos da função do algoritemo do LZWd em si

// Precisamos de definir os dicionários a usar para além do ASCII, o mais indicado seria das palavras portuguesas e inglesas mais comuns visto que são as línguas mais usadas em Portugal
// Precisamos de definir um método de mudar o tamanho dos blocos para o processamento do contéudo do ficheiro (podemos usar os argumentos da exe do ficheiro)
// Mesma coisa para todas as opcionais.
// Não sei se vamos precisar de processos pai-filho
// Quanto à otimização o melhor seria usar as funções mais simples do C.

#define basic_block_size 65536
#define num_filhos 256

//Estruta de dados usado, trie.

typedef struct trie_node trie_node;
int indice_global = 0;

struct trie_node {
    int e_leaf;
    char* dados;
    int indice;
    int output;
    trie_node* filhos[num_filhos];
};

int getsize_of_array(char* array_to_check){
    int i = 0;
    //printf("%s\n",array_to_check);
    while(array_to_check[i]!=NULL){
        i++;
    }
    i++;
    return i;
}

trie_node* criar_node(char* data,int output_a_colocar){
    trie_node* novo_node = (trie_node*)calloc(1, sizeof(trie_node));
    novo_node -> dados = (char*)malloc((strlen(data) + 1) * sizeof(char));
    strcpy(novo_node->dados,data);
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
/*

char* partir_blocos(int tamanho_bloco_txt, int tamanho_lido, FILE* file){
    char* bloco_text = (char*)malloc(tamanho_bloco_txt*sizeof(char));
    fseek(file,tamanho_lido,SEEK_SET);
    fread(bloco_text,tamanho_bloco_txt,sizeof(char),file);
    printf("bloco lido %s", &bloco_text);
    return bloco_text;
}

char* unir_blocos(){

}
*/

void mostrar_est(){ 

}

void print_time(){
    time_t current_time;
    struct tm *local_time;
    char time_string[80];

    current_time = time(NULL);

    local_time = localtime(&current_time);

    strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", local_time);

    printf("Tempo do início da execução: %s\n", time_string);
}
/*bool reached_eof(char* str){
    for(int i = 0; str[i] != '\0'; i++){
        if(str[i] == EOF){
            return true;
        }
    }
}*/

void libertar_nodes(trie_node* node, int n_filhos){
    for(int i=0; i<n_filhos; i++) {
        if (node->filhos[i] != NULL) {
            libertar_nodes(node->filhos[i],n_filhos);
        }
        else {
            continue;
        }
    }
    free(node->filhos);
    free(node);
}

int encontrar_pos_livre(trie_node* node, char* padrao){
    //printf("À procura de posições\n");
    for(int i = 0; i<num_filhos;i++){
       /*if(memcmp(node->filhos[i]->dados,padrao) == 0){
            printf("Este padrão %s já existe\n",padrao);
            return -1;
        }*/
        //printf("Posição atual: %d",i);
        if(node->filhos[i] != NULL){
            //printf("Encontrei uma posição livre: %d\n",i);
        }else{
            return i;
        }
    }
    return -1;
}

int procurar_na_trie(trie_node* root, char* padrao){ //Devolve o indice do padrão que queremos do dicionário
    //printf("Entrei\n");
    //sleep(1);
    //printf("%s\n",padrao);
    trie_node* temp = root;
    char* palavra_total;
    int padrao_encontrado = 0; //Serve como uma boolean
    int num_filho_com_padrao = 0;
    printf("%s\n",temp->dados);
    if(temp->filhos[0] != NULL){ //Fazemos isto para visto que todos os pontos que têm o primeiro filho a NULL não possuem qualquer filho.
    for(int x = 0; x < num_filhos; x++){
        //printf("%d\n",sizeof(root->filhos[x]));
        temp = root->filhos[x];
        //printf("Debug1Procura\n");
        //char* dados = temp->dados;
        /*printf("Debug\n");
        printf("%s\n", temp->dados);
        printf("NaoDebug\n");*/
                //printf("Debug2Procura\n");
                //printf("Entrei1Ciclo\n");
                //printf("%s\n", temp->dados);
                //printf("%ld\n",  strlen(padrao));
                //printf("Debug\n");
                if(temp != NULL && (getsize_of_array(padrao)-1)!=1){
                    //printf("Filho n:%d, conteudo:%s\n",x,temp->dados);
                }
                if(temp != NULL && memcmp(temp->dados,padrao,getsize_of_array(padrao))==0){ // && memcmp(root -> filhos[x] -> dados, padrao)
                    //printf("Debug3Procura\n");
                    //printf("%s\n",temp->dados);
                    if(strlen(padrao) == 1){
                        //printf("So1Char\n");
                        //printf("%d\n",x);
                        return x; //Caso o padrão a procurar apenas tenha um caracter sabemos que faz parte dos chars de 1 nível a partir da raiz, logo o indíce corresponde também ao valor de x.
                    }
                    padrao_encontrado = 1;
                    num_filho_com_padrao = x;
                    //printf("Numero de indice devolvio: %d\n",num_filho_com_padrao);
                    return x;
                    //temp = temp -> filhos[x];
                    //x = 0;
                    //Padrão + 1 posição do padrão existe,sendo esta sempre concactenada a um char já existente, e enquanto isto for possível, ou enquanto não chegarmos ao fim do padrão, continuamos a ver esta condição
                }else{
                    //printf("EncontreiNada\n");
                    padrao_encontrado = 0;
                }
        }
    }
    if(padrao_encontrado == 1){
        //printf("DevolviOIndice\n");
        return (num_filho_com_padrao);
    }else{
        printf("Dicionário Cheio ou não encontrei o padrão pedido\n");
        return -1;
    }
}

trie_node* procurar_na_trie_total(trie_node* raiz, char* padrao){
    //printf("TT");
    trie_node* temp = raiz;
    if(raiz == NULL){
        return NULL;
    }
    if(memcmp(raiz->dados,padrao,getsize_of_array(padrao)) == 0){
        return raiz;
    }
    for(int i = 0; i < num_filhos; i++){
            if(raiz->filhos[i] != NULL){
                
                temp = raiz->filhos[i];
                trie_node* resultado_proc = procurar_na_trie_total(temp,padrao);
                if(resultado_proc!=NULL){
                    //printf("raiz de onde vem o padrao,%s , procurado : %s\n",padrao,temp->dados);
                    //printf("Encontrei o P\n");
                    return temp;
                }
                }
            }
    return NULL;
}


void inserir_na_trie(trie_node* root, char* bloco, int n_filhos){
    //Como vamos inserir no dicionário, nós têmos de percorrer o dicionário da maneira mais eficiente
    //Basicamente ao percorrer têmos de tentar encontrar a raiz do mesmo
    //Podemos guardar o prefixo atual e no fim fazemos uma procura a partir da posição atual no bloco de texto até deixarmos de encontrar prefixos válidos ex:
    //AABBAB -> Este ao fim de 4 iterações pode usar AB como um prefixo
    trie_node* temp = root;
    trie_node* temp_Pb = root; //Precisamos de um extra para ser usado para a procura do Pb
    trie_node* temp_final = root;
    /*char* Pb = (char*)malloc(2*sizeof(char));
    char* Pa = (char*)malloc(2*sizeof(char));
    Pa[1] = '\0';
    Pb[1] = '\0';*/
    int i = 0;
    int filho_pa_encontrado = -1;
    //int filho_pa_encontrado_ant = 0; //Caso obtenhamos -1;
    int filho_pb_encontrado = 0;
    //int filho_pb_encontrado_ant = 0;
    int pos_valida = 0;
    int outputs_novo = 0;
    int primeira_it = 1;
    int outputs_anterior = 0;
    int pos_avancar = 1;
    int pos_avancar_pa =1;
    int preciso_encontrar_temp_Pb = 0;
    int maximo_buffer_indices = 3;
    int contador_idx = 0;
    char* Pa = (char*)malloc(2*sizeof(char));
    Pa[1] = '\0';
    memcpy(Pa, bloco,1);
    int max_size = getsize_of_array(bloco)-1;
    int block_end = 0;
    printf("Tamanho maximo do bloco: %d\n",max_size);
    for(i = 0; i<max_size && block_end == 0; i++){
        int indices_recentes[3] = {0,0,0};
        char* Pb = (char*)malloc(2*sizeof(char));
        char* pattern_to_dictionary = (char*)malloc(3*sizeof(char)); //Padrão que vai ser adicionado ao dicionário. Este Aumenta o seu tamanho consoante o aumento de Pa ou Pb
        Pb[1] = '\0';
        pattern_to_dictionary[2]='\0';
        int index_ascii = (int) bloco[i];
        int a = i+1;
        memcpy(Pb, (bloco+a), 1);
        printf("Posição no bloco : %d\n",i);
        //printf("Valor no bloco : %d\n",(i+(getsize_of_array(Pa)-1)));
        //PA
        printf("Pa atual: %s\n", Pa);
        if(primeira_it == 1){
            pattern_to_dictionary[0] = Pa[0];
            //primeira_it = 0;
        }else{
            //printf("Pa_final: %s\n",Pa);
            /*Pa = (char*)realloc(Pa, getsize_of_array(temp_final->dados)*sizeof(char));
            memcpy(Pa,temp_final->dados,getsize_of_array(temp_final->dados));*/
            temp = procurar_na_trie_total(root,Pa);
            //printf("Teste temp: %s\n",temp->dados);
            if(getsize_of_array(Pa)-1!=1){
                outputs_novo = temp -> indice;
            }else{
                temp = root;
                outputs_novo = index_ascii;
            }
        }
        outputs_anterior = outputs_novo;
            filho_pa_encontrado = procurar_na_trie(temp,Pa);    
            //printf("Padrão atual: %s\n",pattern_to_dictionary);
            //printf("Filho pb encontrado: %d\n",filho_pb_encontrado);
            //printf("DebugPbAposVerificarDic\n");
        if(filho_pa_encontrado != -1){
            pattern_to_dictionary = (char*)realloc(pattern_to_dictionary, pos_avancar*sizeof(char));
            pattern_to_dictionary[pos_avancar];
            memcpy(pattern_to_dictionary,Pa,(getsize_of_array(Pa)-1));
            pos_avancar_pa = getsize_of_array(Pa)-1;
            //printf("SAI DO Pa com este padrão: %s\n",pattern_to_dictionary);
            if(primeira_it == 0){
                temp = procurar_na_trie_total(root,Pa);
            }
        }
        if(primeira_it == 1){
            primeira_it = 0;
            temp = temp -> filhos[index_ascii];
        }
            //printf("Debug1_Inserir_procuraPa\n");
            //pos_avancar++;
        //printf("Temp à saida do Pa:%s\n",temp->dados);
        // -------------------------------------------------------------
        if(pos_avancar_pa >= 2){
            if((i+(getsize_of_array(Pa)))>=max_size){
                preciso_encontrar_temp_Pb = 1;
                i = i+pos_avancar_pa-1;
                pos_avancar = 1;
            }else{
            //printf("Entrei aqui");
            preciso_encontrar_temp_Pb = 1;
            i = i+pos_avancar_pa-1;
            pos_avancar = 1;
            }
        }else{
            pos_avancar = 1;
        }
        //Pb
        while(filho_pb_encontrado != -1){
            //printf("DEBUGPb\n");
            if(pos_avancar!=1){
                printf("Posições a avançar Pb: %d\n",pos_avancar);
                //printf("Proxima posição no padrão (PB):");
                Pb = (char*)realloc(Pb, pos_avancar+1*sizeof(char));
                Pb[pos_avancar]='\0';
                /*
                pattern_to_dictionary = (char*)realloc(pattern_to_dictionary, pos_avancar+1*sizeof(char));
                pattern_to_dictionary[strlen(pattern_to_dictionary)+1+pos_avancar];*/
                memcpy(Pb,bloco+i+1,pos_avancar);
                //printf("%s\n",Pb); //-> Imprime o padrão do Pb
                //strncpy(pattern_to_dictionary+(strlen(Pb)),Pb,(strlen(Pb)-1));
                //printf("DEBUGPbAposcopia\n");
                if(preciso_encontrar_temp_Pb == 0){
                    //temp_Pb = procurar_na_trie_total(root,Pb);
                    temp_Pb = temp_Pb -> filhos[filho_pb_encontrado]; //Para procurar o padrão nós descemos de nível.
                    if(procurar_na_trie_total(temp_Pb,Pb) == NULL){
                        temp_Pb = procurar_na_trie_total(root,Pb);
                        if(temp_Pb == NULL){
                            temp_Pb = root;
                        }
                    }
                }else{
                    preciso_encontrar_temp_Pb = 0;
                    temp_Pb = procurar_na_trie_total(root,Pb);
                    //printf("tagala:%s\n",temp_Pb->dados);
                    //printf("Ponto na árvore encontrado: %s\n",temp_Pb->dados);
                }
                //O erro está a acontecer porque ele está a bugar no temp que é enviado, logo há um erro quanto ao filho a ser revisto.
            }
            if(preciso_encontrar_temp_Pb == 1){
                memcpy(Pb,bloco+i+1,pos_avancar);
                preciso_encontrar_temp_Pb = 0;
                //temp_Pb = procurar_na_trie_total(root,Pb);
                //printf("Ponto na árvore encontrado: %s\n",temp_Pb->dados);
            }
            printf("Pb Atual: %s\n",Pb);
            filho_pb_encontrado = procurar_na_trie(temp_Pb,Pb);
            for(int o = 0;o<maximo_buffer_indices && filho_pb_encontrado != -1;o++){
                //printf("%d\n",indices_recentes[o]);
                if(temp_Pb->filhos[filho_pb_encontrado]->indice == indices_recentes[o]){
                    filho_pb_encontrado = -1; //O padrão que queriamos usar em Pb foi encontrado recentemente, logo não pode ser utilizado.
                }
            }
            //printf("Padrão atual: %s\n",pattern_to_dictionary);
            //printf("Filho pb encontrado: %d\n",filho_pb_encontrado);
            //printf("Vou procurar no temp,%s\n",temp->dados);

            if(filho_pb_encontrado != -1 && (pos_valida = encontrar_pos_livre(temp,pattern_to_dictionary)) != -1){
                pattern_to_dictionary = (char*)realloc(pattern_to_dictionary, (strlen(Pb)+strlen(Pa)));
                pattern_to_dictionary[(strlen(Pb)+strlen(Pa))] = '\0';
                //printf("Tamanho Pa e Pb: %ld , %ld \n",strlen(Pa),strlen(Pb));
                memcpy(pattern_to_dictionary+(strlen(Pa)),Pb,(strlen(Pb)));
                trie_node* NP_A_PROCURAR = procurar_na_trie_total(root,pattern_to_dictionary);
                if(NP_A_PROCURAR != NULL){
                    int NP_procurado = procurar_na_trie(NP_A_PROCURAR,pattern_to_dictionary);
                    //printf("Posição encontrada,%d\n",NP_procurado);
                }else{
                indice_global++;
                if(contador_idx<maximo_buffer_indices){
                    indices_recentes[contador_idx] = indice_global;
                    contador_idx++;
                }else{
                    maximo_buffer_indices = maximo_buffer_indices+3;
                }
                printf("Padrao para o dic: %s\n",pattern_to_dictionary);
                temp -> filhos[pos_valida] = criar_node(pattern_to_dictionary,outputs_novo);
                }
                //printf("Pa neste momento: %s\n",Pa);
            }
            //printf("Temp que chegou:%s\n",temp->dados);
            //printf("Pos livre: %d\n",pos_valida);
            //printf("Debug1_Inserir_procuraPb\n");
            pos_avancar++;
        }
        Pa = (char*)realloc(Pa, (getsize_of_array(Pb)-1)*sizeof(char));
        memcpy(Pa,Pb,getsize_of_array(Pb)-2);
        Pa[getsize_of_array(Pb)-1] = '\0';
        printf("Tamanho Pb: %d\n",getsize_of_array(Pb)-1);
        //---------------------------------
        contador_idx = 0;
        temp = root; //Quando passamos para um Pa novo têmos de voltar para o topo da trie.
        temp_Pb = root;
        //temp = temp -> filhos[index_ascii]; //Para procurar o padrão nós descemos de nível.
        
        free(pattern_to_dictionary);
        filho_pb_encontrado = 0;
        pos_avancar_pa = 1;
        if((i+getsize_of_array(Pb)) >= max_size){
            printf("FIM DO BLOCO");
            block_end = 1;
        }
        free(Pb);
        //free(indices_recentes);
    }
    free(Pa);
    temp -> e_leaf = 1;
}

trie_node* criar_dicionario(trie_node* root){ //Funciona como esperado
    for(int i = 0; i < 256; i++){
        char caracter[] = {i,'\0'};
        //printf("%s\n",caracter);
        root->filhos[i] = criar_node(caracter,0);
        //printf("Valor filho_pa_encontrado: %d\n",filho_pa_encontrado);
        //printf("Valor indice: %d\n", root->filhos[i]->indice);
        //printf("Valor dados -> %s\n", root->filhos[i]->dados);
        //printf("%s\n", root->filhos[i]->dados);
        indice_global++;
    }
    return root;
}

int main(int argc, char *argv[]) {
    int dicionario_base = 1; //Dicionário é o báscio (Tabela ASCII), se não for ativada a opção de um dicionário custom | 0 -> Dicionário Custom | 1 -> Dicionário Base 
    char content_raiz[] = {'\0'};
    trie_node* raiz = criar_node(content_raiz,0);
    int fd;
    int dicionario_default = 0;
    int position_in_file = 0;
    //bool end_file = false;
    int nread = 0;
    int block_size;
    if(argc > 3){ //Existem argumentos para além da exc da app e o nome do ficheiro a comprimir, logo têmos de verificar que argumentos são usados, para já isto não vai fazer nada
        switch(argc){
            case 4:
                //Tamanho dos blocos
                block_size = atoi(argv[2]);
                break;
            case 5:
                //Outra op
                break;
            case 6:
                //Outra op
                break;
            default:
                break;
            //Se for preciso mais pomos mais
        }
    }else{
        block_size = basic_block_size;
    }

    indice_global = 0;
    FILE* file = fopen(argv[1], "r");
    if(fd == -1){
        printf("O caminho até ao ficheiro está errado ou o ficheiro não existe");
        return 1;
    }
    struct stat st;
    fstat(fd,&st);
    
    //Impressões Iniciais
    printf("Autoria:%ld", &st.st_uid); //Impressão numérica do ID do autor do ficheiro
    print_time();
    printf("Nome do ficheiro de entrada, %s \n",argv[1]);
    printf("Nome do ficheiro de saída, %s \n",argv[2]);
    raiz = criar_dicionario(raiz);
    //Fim impressões iniciais
    //Têmos de ter uma maneira de introduzir um dicionário
    while(!feof(file)){
        char* text_block = (char*)malloc(sizeof(char)*block_size);
        fseek(file,position_in_file,SEEK_SET);
        position_in_file += block_size;
        fread(text_block,sizeof(char), block_size,file);
        printf("%s", text_block);
        printf("\n");
        inserir_na_trie(raiz,text_block,256);
    }
  
    return 0;
}