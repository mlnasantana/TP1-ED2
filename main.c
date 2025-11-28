#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "item.h"
#include "index.h"
#include "arvoreb.h"
#include "arvorebe.h"
#include "arvorebin.h"
#include "geradorDeArquivos.h"

int main(int argc, char *argv[]) {
    // Verificando se o número de argumentos está correto
    if (argc < 5 || argc > 6) {
        printf("Uso: %s <método> <quantidade> <situação> <chave> [-P]\n", argv[0]);
        return 1;
    }
    // Variáveis para armazenar os parâmetros de entrada
    int metodo = atoi(argv[1]); 
    int quantReg = atoi(argv[2]);
    int situacao = atoi(argv[3]);
    Item itemPagina; // Estrutura para armazenar o item a ser pesquisado
    AnaliseExperimental analise;
    itemPagina.chave = atoi(argv[4]); // Armazena a chave do item a ser pesquisado
    bool P = false; // Indica se a opção -P foi passada

    // Tratando a opção -P para funcionar tanto com maiúscula quanto minúscula
    if(argc == 6 && ((strcmp(argv[5], "-P") == 0) || (strcmp(argv[5], "-p") == 0))) P = true;

    char nomeArq[100];
    sprintf(nomeArq, "dados_%d_%d.dat", quantReg, situacao);
    
    // Gera o arquivo com os dados
    if (gerarArquivo(quantReg, situacao, nomeArq)){
        printf("Arquivo gerado com sucesso!\n");
    } else {
        printf("Erro ao gerar o arquivo.\n");
        return 1;
    }

    // Mostra o arquivo gerado se a opção -P for passada
    if (P){
        mostraArquivo(nomeArq);
    }

    FILE *arquivo = fopen(nomeArq, "rb");
    if (arquivo == NULL) {
    printf("Erro ao abrir o arquivo %s\n", nomeArq);
    return 1;
}

    
    switch (metodo) {
        case 1:
            if (situacao != 1){
                printf("O acesso sequencial indexado só funciona para arquivos odernados.\n");
                return 1;
            }
            if(itemPagina.chave != -1){ //Se for passada uma chave de busca padrão, roda a pesquisa normalmente
                Indexado(&itemPagina, arquivo, &analise, quantReg);

                printf("Número de comparações: %d\n", analise.numComparacoes);
                printf("Número de transferências: %d\n", analise.numTransferencias);
                printf("Tempo de Execução: %.4f segundos\n", (double)analise.tempoExecucao / CLOCKS_PER_SEC);
            } else {
                RodaIndexadoExperimento(arquivo, quantReg); //Teste das 10 chaves
            }
            
        break;
        case 2:
            RodaArvoreBinaria(nomeArq, itemPagina.chave, quantReg, P);
        break;
        case 3:
            RodaArvoreB(nomeArq, itemPagina.chave, quantReg, P);
        break;
        case 4:
            RodaArvoreBEstrela(arquivo, itemPagina.chave, quantReg, P);
            break;
        default:
            printf("Metodo %d nao implementado.\n", metodo);
            break;
    }

    fclose(arquivo);
    return 0;
}