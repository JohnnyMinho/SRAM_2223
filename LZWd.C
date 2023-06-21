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
    char* dados = (char*)malloc(256*sizeof(char));
    int indice;
    int output;
    trie_node* filhos[num_filhos];
};

trie_node* criar_node(char* data,int output_a_colocar){
    trie_node* novo_node = (trie_node*) calloc (1, sizeof(trie_node));
    novo_node -> dados = (char*)malloc((strlen(data) + 1) * sizeof(char));
    novo_node -> dados = data;
    novo_node -> output = output_a_colocar;
    //novo_node->filhos = (trie_node**) malloc(n_filhos * sizeof(trie_node*));
    for (int i=0; i<num_filhos; i++){
        novo_node->filhos[i] = NULL;
    //novo_node->num_filhos = 0;
    novo_node->e_leaf = 0;
    }
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

int encontrar_pos_livre(trie_node* node){
    for(int i; i<num_filhos;i++){
        if(node -> filhos[i] == NULL){
            return i;
        }
    }
    return -1;
}

int procurar_na_trie(trie_node* root, char* padrao){ //Devolve o indice do padrão que queremos do dicionário
    printf("Entrei\n");
    trie_node* temp = root;
    char* palavra_total;
    int padrao_encontrado = 0; //Serve como uma boolean
    int num_filho_com_padrao = 0;
    printf("DEBUGProcura0\n");
    for(int x = 0; x < num_filhos; x++){
        printf("%d\n",sizeof(root->filhos[x]));
        temp = root->filhos[x];
        char* dados = temp->dados;
        printf("%s\n", dados);
        for(int i = 0; padrao[i] != NULL; i++){
            printf("Entrei1Ciclo\n");
            printf("%s\n", temp->dados);
            printf("Debug");
            if(temp->dados == padrao){ // && strcmp(root -> filhos[x] -> dados, padrao)
                printf("EntreinoIf\n");
                if(sizeof(padrao) == 1){
                    printf("DEBUGProcura1\n");
                    return x; //Caso o padrão a procurar apenas tenha um caracter sabemos que faz parte dos chars de 1 nível a partir da raiz, logo o indíce corresponde também ao valor de x.
                }
                padrao_encontrado = 1;
                num_filho_com_padrao;
                //temp = temp -> filhos[x];
                //x = 0;
                //Padrão + 1 posição do padrão existe,sendo esta sempre concactenada a um char já existente, e enquanto isto for possível, ou enquanto não chegarmos ao fim do padrão, continuamos a ver esta condição
            }else{
                printf("EncontreiNada\n");
                padrao_encontrado = 0;
            }
        }
    }
    if(padrao_encontrado == 1){
        printf("DevolviOIndice\n");
        return (root -> filhos[num_filho_com_padrao] -> indice);
    }else{
        printf("Dicionário Cheio ou não encontrei o padrão pedido");
        return -1;
    }
}

void inserir_na_trie(trie_node* root, char* bloco, int n_filhos){
    //Como vamos inserir no dicionário, nós têmos de percorrer o dicionário da maneira mais eficiente
    //Basicamente ao percorrer têmos de tentar encontrar a raiz do mesmo
    //Podemos guardar o prefixo atual e no fim fazemos uma procura a partir da posição atual no bloco de texto até deixarmos de encontrar prefixos válidos ex:
    //AABBAB -> Este ao fim de 4 iterações pode usar AB como um prefixo
    trie_node* temp = root;
    trie_node* temp_Pb = root; //Precisamos de um extra para ser usado para a procura do Pb
    char* Pb = (char*)malloc(2*sizeof(char));
    char* Pa = (char*)malloc(2*sizeof(char));
    Pa[1] = '\0';
    Pb[1] = '\0';
    int i = 0;
    int filho_pa_encontrado = 0;
    int filho_pb_encontrado = 0;
    int pos_valida = 0;
    int outputs_novo = 0;
    int pos_avancar = 0;
    for(i = 0; bloco[i] != NULL; i++){
        printf("Debug1_Inserir\n");
        int index_ascii = (int) bloco[i];
        memcpy(Pa, bloco+i,1);
        //printf("DEBUG0");
        printf("%s\n",Pa);
        int a = i+1;
        //printf("DEBUG1");
        //printf("%c",bloco[a]);
        //printf("DEBUG2");
        //Pb[i] = bloco[a];
        memcpy(Pb, (bloco+a), 1); // -> Segmentation fault
        printf("%s\n",Pb);
        do{
            printf("DEBUG00\n");
            if(pos_avancar!=0){
                printf("DEBUG11\n");
                Pa = (char*)realloc(Pa, pos_avancar+1*sizeof(char));
                Pa[pos_avancar+1]='\0';
                memcpy(Pa+pos_avancar,bloco+i,i+pos_avancar);
                temp = temp -> filhos[filho_pa_encontrado]; //Para procurar o padrão nós descemos de nível.
                outputs_novo = temp -> indice;
            }else{
                printf("DEBUG22\n");
                outputs_novo = index_ascii;
            }
            printf("Debug1_Inserir_procuraPa\n");
            pos_avancar++;
        }while(filho_pa_encontrado = procurar_na_trie(temp,Pa) != -1);
        pos_avancar = 0;
        do{
            printf("DEBUG0");
            if(pos_avancar!=0){
                printf("DEBUG1");
                Pb = (char*)realloc(Pb, pos_avancar+1*sizeof(char));
                Pb[pos_avancar+1]='\0';
                memcpy(Pb+pos_avancar,bloco+a,a+pos_avancar);
                temp_Pb = temp_Pb -> filhos[filho_pb_encontrado]; //Para procurar o padrão nós descemos de nível.
            }
            printf("Debug1_Inserir_procuraPb\n");
            pos_avancar++;
        }while(filho_pb_encontrado = procurar_na_trie(temp,Pb) != -1);
        
        if(pos_valida = encontrar_pos_livre(temp) != -1){
            indice_global++;
            temp -> filhos[filho_pa_encontrado] = criar_node(Pb,outputs_novo);
        }
    //for(int i = 0; i<n_filhos; i++){
        //Procura pelo index do prefixo do padrao
        //Procurar mais padrões conhecidos
        /*
        if(temp -> filhos[index_ascii] == NULL){
            temp -> filhos[index_ascii] = criar_node(bloco[i]);
        }else{
            
        }*/
        temp = root; //Quando passamos para um Pa novo têmos de voltar para o topo da trie.
        //temp = temp -> filhos[index_ascii]; //Para procurar o padrão nós descemos de nível.
    }
    temp -> e_leaf = 1;
    free(Pb);
    free(Pa);
}

trie_node* criar_dicionario(trie_node* root){ //Funciona como esperado
    for(int i = 0; i < 256; i++){
        char caracter[] = {i,'\0'};
        //printf("%s\n",caracter);
        root->filhos[i] = criar_node(caracter,0);
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

    indice_global = 1;
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
    printf("Debug\n");
    printf("%s\n",raiz->filhos[66]->dados);
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