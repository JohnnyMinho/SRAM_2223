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


struct trie_node {
    int e_leaf;
    char *dados;
    int indice;
    trie_node* filhos[num_filhos];
};

trie_node* criar_node(char data){
    trie_node* novo_node = (trie_node*) calloc (1, sizeof(trie_node));
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

trie_node* inserir_na_trie(trie_node* root, char* padrao, int n_filhos){
    //Como vamos inserir no dicionário, nós têmos de percorrer o dicionário da maneira mais eficiente
    //Basicamente ao percorrer têmos de tentar encontrar a raiz do mesmo
    //Podemos guardar o prefixo atual e no fim fazemos uma procura a partir da posição atual no bloco de texto até deixarmos de encontrar prefixos válidos ex:
    //AABBAB -> Este ao fim de 5 iterações pode usar AB como um prefixo
    trie_node* temp = root;
    
    for(int i = 0; padrao[i] != NULL; i++){
        int index_ascii = (int) padrao[i];
    //for(int i = 0; i<n_filhos; i++){
        //Procura pelo index do prefixo do padrao
        //Procurar mais padrões conhecidos
        if(temp -> filhos[index_ascii] == NULL){
            temp -> filhos[index_ascii] = criar_node(padrao[i]);
        }else{

        }
        temp = temp -> filhos[index_ascii]; //Para procurar o padrão nós descemos de nível.
    }
    temp -> e_leaf = 1;
    return temp;
}

int procurar_na_trie(trie_node* root, char* padrao){

}


int main(int argc, char *argv[]) {
    int dicionario_base = 1; //Dicionário é o báscio (Tabela ASCII), se não for ativada a opção de um dicionário custom | 0 -> Dicionário Custom | 1 -> Dicionário Base 
    trie_node* raiz = criar_node('\0');
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
    
    //Fim impressões iniciais
    //Têmos de ter uma maneira de introduzir um dicionário
    while(!feof(file)){
        char* text_block = (char*)malloc(sizeof(char)*block_size);
        fseek(file,position_in_file,SEEK_SET);
        position_in_file += block_size;
        fread(text_block,sizeof(char), block_size,file);
        inserir_na_trie(raiz,text_block,256);
    }
  
    return 0;
}