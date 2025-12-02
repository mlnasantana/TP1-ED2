#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// Includes de todos os métodos
#include "item.h"
#include "sequencialIndex.h"
#include "arvorebin.h"  // Método 2
#include "arvoreb.h"    // Método 3
#include "arvorebe.h"   // Método 4
#include "geradorDeArquivos.h"
#include "analiseExperimental.h"

int main(int argc, char *argv[]){

    // Validação dos argumentos
    if (argc < 5 || argc > 6) {
        printf("Uso: %s <método> <quantidade> <situação> <chave> [-P]\n", argv[0]);
        printf("Métodos:\n");
        printf("1 - Acesso Sequencial Indexado\n");
        printf("2 - Árvore Binária Externa\n");
        printf("3 - Árvore B\n");
        printf("4 - Árvore B*\n");
        return 1;
    }

    // Leitura dos parâmetros
    int metodo = atoi(argv[1]); 
    int quantReg = atoi(argv[2]);
    int situacao = atoi(argv[3]);
    Item itemP;
    itemP.chave = atoi(argv[4]); // Chave a ser pesquisada (-1 para experimento)
    bool P = false; 

    // Inicialização da análise
    AnaliseExperimental analise;
    analise.numComparacoes = 0;
    analise.numTransferencias = 0;
    analise.tempoExecucao = 0.0;

    // Verificação da flag -P (Print)
    if(argc == 6 && ((strcmp(argv[5], "-P") == 0) || (strcmp(argv[5], "-p") == 0))) P = true;

    // Geração do nome do arquivo
    char nomeArq[100];
    sprintf(nomeArq, "dados_%d_%d.dat", quantReg, situacao);

    // Gera o arquivo se necessário
    if (gerarArquivo(quantReg, situacao, nomeArq)){
        printf("Arquivo de dados verificado/gerado: %s\n", nomeArq);
    } else {
        printf("Erro ao gerar o arquivo.\n");
        return 1;
    }

    if (P){
        mostraArquivo(nomeArq);
    }

    // Abre o arquivo de dados principal
    FILE *arquivo = fopen(nomeArq, "rb");
    if (arquivo == NULL){
        printf("Erro ao abrir o arquivo %s\n", nomeArq);
        return 1;
    }

    // Seletor de Métodos
    switch (metodo){
        // ---------------------------------------------------------
        // MÉTODO 1: ACESSO SEQUENCIAL INDEXADO
        // ---------------------------------------------------------
        case 1: 
            printf("\n>>> Método: Acesso Sequencial Indexado <<<\n");
            if (situacao != 1){ // Verifica se está ordenado
                printf("Erro: O acesso sequencial indexado só funciona para arquivos ordenados ascedentemente.\n");
                fclose(arquivo);
                return 1;
            }
            
            if (itemP.chave != -1){
                // Pesquisa Única
                if(indexado(arquivo, quantReg, &itemP, &analise)){
                    imprimeAnalise(&analise, &itemP);
                } else {
                    printf("Item com chave %d não encontrado.\n", itemP.chave);
                }
            } else{
                // Experimento (20 execuções)
                executarExperimento(arquivo, quantReg, &analise, indexado);
                imprimeMediaAnalise(&analise);
            }
        break;

        // ---------------------------------------------------------
        // MÉTODO 2: ÁRVORE BINÁRIA EXTERNA
        // ---------------------------------------------------------
        case 2:
            printf("\n>>> Método: Árvore Binária Externa <<<\n");
            
            // FASE 1: Construção do Índice
            printf("--- Construindo Índice (arvore_index.bin) ---\n");
            AnaliseExperimental analiseCriacao = {0,0,0};
            construirArvoreBin(nomeArq, "arvore_index.bin", &analiseCriacao);
            printf("Tempo Criação: %.6f s | Transferências: %d\n\n", 
                   analiseCriacao.tempoExecucao, analiseCriacao.numTransferencias);

            // FASE 2: Pesquisa
            if (itemP.chave != -1){
                if(pesquisaArvoreBinariaWrapper(arquivo, quantReg, &itemP, &analise)){
                    imprimeAnalise(&analise, &itemP);
                } else {
                    printf("Item com chave %d não encontrado.\n", itemP.chave);
                }
            } else {
                executarExperimento(arquivo, quantReg, &analise, pesquisaArvoreBinariaWrapper);
                imprimeMediaAnalise(&analise);
            }
            
            // remove("arvore_index.bin"); // Opcional: limpar arquivo temporário
        break;

        // ---------------------------------------------------------
        // MÉTODO 3: ÁRVORE B
        // ---------------------------------------------------------
        case 3: 
            printf("\n>>> Método: Árvore B <<<\n");
            
            if (itemP.chave != -1){
                // Pesquisa Única (Constrói árvore em RAM -> Pesquisa -> Libera)
                if(pesquisaArvoreB(arquivo, quantReg, &itemP, &analise)){
                    imprimeAnalise(&analise, &itemP);
                } else {
                    printf("Item com chave %d não encontrado.\n", itemP.chave);
                }
            } else {
                // Experimento
                executarExperimento(arquivo, quantReg, &analise, pesquisaArvoreB);
                imprimeMediaAnalise(&analise);
            }
        break;

        // ---------------------------------------------------------
        // MÉTODO 4: ÁRVORE B* (ESTRELA)
        // ---------------------------------------------------------
        case 4: 
            printf("\n>>> Método: Árvore B* (Estrela) <<<\n");

            if (itemP.chave != -1){
                // Pesquisa Única
                if(pesquisaArvoreBEstrela(arquivo, quantReg, &itemP, &analise)){
                    imprimeAnalise(&analise, &itemP);
                } else {
                    printf("Item com chave %d não encontrado.\n", itemP.chave);
                }
            } else {
                // Experimento
                executarExperimento(arquivo, quantReg, &analise, pesquisaArvoreBEstrela);
                imprimeMediaAnalise(&analise);
            }
        break;

        default:
            printf("Método inválido. Escolha entre 1 e 4.\n");
            break;
    }

    if (arquivo != NULL) fclose(arquivo);
    return 0;
}