#include "arvorebin.h"

// ... (Mantenha as funções criarNo e inserirNaArvoreBin iguais à resposta anterior) ...

NoArvoreBin criarNo(int chave, long int offsetDado) {
    NoArvoreBin no;
    no.chave = chave;
    no.offsetDado = offsetDado;
    no.esquerda = NULL_DISK;
    no.direita = NULL_DISK;
    return no;
}

long int inserirNaArvoreBin(FILE* arqArvore, long int posAtual, int chave, long int offsetDado, AnaliseExperimental *analise) {
    if (posAtual == NULL_DISK) {
        NoArvoreBin novoNo = criarNo(chave, offsetDado);
        fseek(arqArvore, 0, SEEK_END);
        long int novaPos = ftell(arqArvore);
        fwrite(&novoNo, sizeof(NoArvoreBin), 1, arqArvore);
        analise->numTransferencias++;
        return novaPos;
    }

    NoArvoreBin noAtual;
    fseek(arqArvore, posAtual, SEEK_SET);
    fread(&noAtual, sizeof(NoArvoreBin), 1, arqArvore);
    analise->numTransferencias++;
    analise->numComparacoes++;

    if (chave < noAtual.chave) {
        long int novoEsq = inserirNaArvoreBin(arqArvore, noAtual.esquerda, chave, offsetDado, analise);
        if (novoEsq != noAtual.esquerda) {
            noAtual.esquerda = novoEsq;
            fseek(arqArvore, posAtual, SEEK_SET);
            fwrite(&noAtual, sizeof(NoArvoreBin), 1, arqArvore);
            analise->numTransferencias++;
        }
    } else if (chave > noAtual.chave) {
        long int novoDir = inserirNaArvoreBin(arqArvore, noAtual.direita, chave, offsetDado, analise);
        if (novoDir != noAtual.direita) {
            noAtual.direita = novoDir;
            fseek(arqArvore, posAtual, SEEK_SET);
            fwrite(&noAtual, sizeof(NoArvoreBin), 1, arqArvore);
            analise->numTransferencias++;
        }
    }
    return posAtual;
}

void construirArvoreBin(const char* nomeArquivoDados, const char* nomeArquivoArvore, AnaliseExperimental *analiseCriacao) {
    FILE* arqDados = fopen(nomeArquivoDados, "rb");
    if (!arqDados) return;

    FILE* arqArvore = fopen(nomeArquivoArvore, "w+b");
    if (!arqArvore) { fclose(arqDados); return; }

    Item item;
    long int offsetDadoAtual = 0;
    long int raizOffset = NULL_DISK;
    
    clock_t inicio = clock();

    while (fread(&item, sizeof(Item), 1, arqDados) == 1) {
        analiseCriacao->numTransferencias++;
        raizOffset = inserirNaArvoreBin(arqArvore, raizOffset, item.chave, offsetDadoAtual, analiseCriacao);
        offsetDadoAtual = ftell(arqDados);
    }

    clock_t fim = clock();
    analiseCriacao->tempoExecucao = (double)(fim - inicio) / CLOCKS_PER_SEC;

    fclose(arqDados);
    fclose(arqArvore);
}

Item* pesquisarArvoreBin(FILE* arqArvore, long int posRaiz, int chave, FILE* arqDados, AnaliseExperimental *analise) {
    if (posRaiz == NULL_DISK) return NULL;

    NoArvoreBin noAtual;
    long int posAtual = posRaiz;

    while (posAtual != NULL_DISK) {
        fseek(arqArvore, posAtual, SEEK_SET);
        fread(&noAtual, sizeof(NoArvoreBin), 1, arqArvore);
        analise->numTransferencias++;
        analise->numComparacoes++;

        if (chave == noAtual.chave) {
            Item* itemEncontrado = (Item*)malloc(sizeof(Item));
            fseek(arqDados, noAtual.offsetDado, SEEK_SET);
            if (fread(itemEncontrado, sizeof(Item), 1, arqDados) == 1) {
                analise->numTransferencias++;
                return itemEncontrado;
            } else {
                free(itemEncontrado);
                return NULL;
            }
        } 
        else if (chave < noAtual.chave) posAtual = noAtual.esquerda;
        else posAtual = noAtual.direita;
    }
    return NULL;
}

// === IMPLEMENTAÇÃO DO WRAPPER ===
bool pesquisaArvoreBinariaWrapper(FILE* arqDados, int quantReg, Item* item, AnaliseExperimental* analise) {
    // 1. Abre o arquivo de índice (criado previamente no main)
    FILE* arqArvore = fopen("arvore_index.bin", "rb");
    if (arqArvore == NULL) {
        perror("Erro ao abrir arquivo de índice na pesquisa");
        return false;
    }

    // 2. Verifica se a árvore tem conteúdo (raiz começa no offset 0 se arquivo > 0)
    long int raizOffset = 0;
    fseek(arqArvore, 0, SEEK_END);
    if (ftell(arqArvore) == 0) raizOffset = NULL_DISK;

    // 3. Realiza a pesquisa
    Item* resultado = pesquisarArvoreBin(arqArvore, raizOffset, item->chave, arqDados, analise);
    
    // 4. Fecha índice
    fclose(arqArvore);

    // 5. Processa resultado
    if (resultado != NULL) {
        *item = *resultado; // Copia dados para o ponteiro do main
        free(resultado);
        return true;
    }

    return false;
}