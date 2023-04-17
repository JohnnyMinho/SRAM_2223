#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
// Precisamos de uma função para dividir a informação lida do ficheiro de origem em blocos 
// Precisamos de uma função para enviar a informação em blocos para o ficheiro final
// Precisamos da função do algoritemo do LZWd em si

// Precisamos de definir os dicionários a usar para além do ASCII, o mais indicado seria das palavras portuguesas e inglesas mais comuns visto que são as línguas mais usadas em Portugal
// Precisamos de definir um método de mudar o tamanho dos blocos para o processamento do contéudo do ficheiro (podemos usar os argumentos da exe do ficheiro)
// Mesma coisa para todas as opcionais.
// Não sei se vamos precisar de processos pai-filho
// Quanto à otimização o melhor seria usar as funções mais simples do C.

#define basic_block_size 65536

unsigned char* partir_blocos(){

}

unsigned char* unir_blocos(){

}

void mostrar_est(){ //Mostra as estatísticas

}

int main(int argc, char *argv[]) {
    int fd;
    if(argc > 2){ //Existem argumentos para além da exc da app e o nome do ficheiro a comprimir, logo têmos de verificar que argumentos são usados, para já isto não vai fazer nada
        switch(argc){
            case 1:
                //Tamanho dos blocos
                break;
            case 2:
                //Outra op
                break;
            case 3:
                //Outra op
                break;
            default:
                break;
            //Se for preciso mais pomos mais
        }
    }else{
        char file_content_block[basic_block_size];
    }
    fd = open(argv[1], O_RDONLY, 0666);
    if(fd == -1){
        printf("O caminho até ao ficheiro está errado ou o ficheiro não existe");
        return 1;
    }
    

    //
    return 0;
}
