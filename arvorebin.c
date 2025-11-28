#include "arvorebin.h"

NoArvoreBin* criarNoArvoreBin(int chave, long int offset) {
    NoArvoreBin* novoNo = (NoArvoreBin*)malloc(sizeof(NoArvoreBin));
    if (novoNo == NULL) {
        perror("Erro ao alocar memória para o nó da árvore binária");
        exit(EXIT_FAILURE);
    }
    novoNo->chave = chave;
    novoNo->offset = offset;
    novoNo->esquerda = NULL;
    novoNo->direita = NULL;
    return novoNo;
}

NoArvoreBin* inserirNaArvoreBin(NoArvoreBin* raiz, int chave, long int offset, AnaliseExperimental *analise) {
    if (raiz == NULL) {
        return criarNoArvoreBin(chave, offset);
    }

    analise->numComparacoes++;
    if (chave < raiz->chave) {
        raiz->esquerda = inserirNaArvoreBin(raiz->esquerda, chave, offset, analise);
    } else if (chave > raiz->chave) {
        raiz->direita = inserirNaArvoreBin(raiz->direita, chave, offset, analise);
    }
    return raiz;
}

NoArvoreBin* construirArvoreBin(const char* nomeArquivo, AnaliseExperimental *analiseCriacao) {
    FILE* arquivo = fopen(nomeArquivo, "rb");
    if (arquivo == NULL) {
        perror("Erro ao abrir o arquivo para construir a árvore binária");
        return NULL;
    }

    NoArvoreBin* raiz = NULL;
    Item item;
    long int offsetAtual = 0;

    clock_t inicio = clock();

    while (fread(&item, sizeof(Item), 1, arquivo) == 1) {
        analiseCriacao->numTransferencias++;
        raiz = inserirNaArvoreBin(raiz, item.chave, offsetAtual, analiseCriacao);
        offsetAtual = ftell(arquivo);
    }

    clock_t fim = clock();
    analiseCriacao->tempoExecucao = (long int)(((double)(fim - inicio)) / CLOCKS_PER_SEC * 1000000.0);

    fclose(arquivo);
    return raiz;
}

Item* pesquisarArvoreBin(NoArvoreBin* raiz, int chave, FILE* arquivo, AnaliseExperimental *analise) {
    if (arquivo == NULL) {
        fprintf(stderr, "Erro: Arquivo não pode ser NULL para pesquisa.\n");
        return NULL;
    }

    Item* itemEncontrado = NULL;
    analise->numTransferencias = 0;
    analise->numComparacoes = 0;
    clock_t inicio = clock();

    NoArvoreBin* atual = raiz;
    while (atual != NULL) {
        analise->numComparacoes++;
        if (chave == atual->chave) {
            if (fseek(arquivo, atual->offset, SEEK_SET) != 0) {
                perror("Erro ao posicionar no arquivo para ler o Item");
                break;
            }
            itemEncontrado = (Item*)malloc(sizeof(Item));
            if (itemEncontrado == NULL) {
                perror("Erro ao alocar memória para o Item encontrado");
                break;
            }
            if (fread(itemEncontrado, sizeof(Item), 1, arquivo) == 1) {
                analise->numTransferencias++;
            } else {
                perror("Erro ao ler o Item do arquivo");
                free(itemEncontrado);
                itemEncontrado = NULL;
            }
            break;
        } else if (chave < atual->chave) {
            atual = atual->esquerda;
        } else {
            atual = atual->direita;
        }
    }

    clock_t fim = clock();
    analise->tempoExecucao = (long int)(((double)(fim - inicio)) / CLOCKS_PER_SEC * 1000000.0);

    return itemEncontrado;
}

void imprimirChavesArvoreBin(NoArvoreBin* raiz) {
    if (raiz != NULL) {
        imprimirChavesArvoreBin(raiz->esquerda);
        printf("Chave: %d, Offset: %ld\n", raiz->chave, raiz->offset);
        imprimirChavesArvoreBin(raiz->direita);
    }
}

void liberarArvoreBin(NoArvoreBin* raiz) {
    if (raiz != NULL) {
        liberarArvoreBin(raiz->esquerda);
        liberarArvoreBin(raiz->direita);
        free(raiz);
    }
}

void RodaArvoreBinaria(const char* nomeArq, int chave_procurada, int quantReg, bool P_flag) {
    NoArvoreBin* raizArvoreBin = NULL;

    AnaliseExperimental analiseCriacao;
    analiseCriacao.numComparacoes = 0;
    analiseCriacao.numTransferencias = 0;
    analiseCriacao.tempoExecucao = 0;

    raizArvoreBin = construirArvoreBin(nomeArq, &analiseCriacao);
    if (raizArvoreBin == NULL) {
        fprintf(stderr, "Erro ao construir a arvore binaria.\n");
        return;
    }

    AnaliseExperimental analisePesquisa;
    AnaliseExperimental analisePesquisaTotal;
    analisePesquisaTotal.numComparacoes = 0;
    analisePesquisaTotal.numTransferencias = 0;
    analisePesquisaTotal.tempoExecucao = 0;

    bool encontrado = false;
    Item resultado;

    if (chave_procurada != -1) {
        FILE* arquivoParaPesquisa = fopen(nomeArq, "rb");
        if (arquivoParaPesquisa == NULL) {
            perror("Erro ao abrir arquivo para pesquisa na Arvore Binaria");
            if (raizArvoreBin) liberarArvoreBin(raizArvoreBin);
            return;
        }

        Item* temp_resultado = pesquisarArvoreBin(raizArvoreBin, chave_procurada, arquivoParaPesquisa, &analisePesquisa);
        if (temp_resultado != NULL) {
            resultado = *temp_resultado;
            free(temp_resultado);
            encontrado = true;
        } else {
            encontrado = false;
        }
        fclose(arquivoParaPesquisa);
    } else {
        int chaves_a_pesquisar[10];

        for(int i=0; i<10; i++) {
             chaves_a_pesquisar[i] = (i + 1) * (quantReg / 10);
             if (chaves_a_pesquisar[0] == 0 && quantReg > 0) chaves_a_pesquisar[0] = 1;
             if (chaves_a_pesquisar[i] == 0 && i > 0 && quantReg > 0) chaves_a_pesquisar[i] = chaves_a_pesquisar[i-1] + 1;
             if (chaves_a_pesquisar[i] > quantReg && quantReg > 0) chaves_a_pesquisar[i] = quantReg;
        }

        for(int i=0; i<10; i++){
            AnaliseExperimental analiseTemp;
            analiseTemp.numComparacoes = 0;
            analiseTemp.numTransferencias = 0;
            analiseTemp.tempoExecucao = 0;

            FILE* arquivoParaPesquisa = fopen(nomeArq, "rb");
            if (arquivoParaPesquisa == NULL) {
                perror("Erro ao abrir arquivo para pesquisa na Arvore Binaria (modo -E)");
                if (raizArvoreBin) liberarArvoreBin(raizArvoreBin);
                return;
            }

            Item* res_temp = pesquisarArvoreBin(raizArvoreBin, chaves_a_pesquisar[i], arquivoParaPesquisa, &analiseTemp);
            if (res_temp != NULL) {
                free(res_temp);
            }
            fclose(arquivoParaPesquisa);

            analisePesquisaTotal.numComparacoes += analiseTemp.numComparacoes;
            analisePesquisaTotal.numTransferencias += analiseTemp.numTransferencias;
            analisePesquisaTotal.tempoExecucao += analiseTemp.tempoExecucao;
        }
    }

    if (chave_procurada != -1) {
        if (encontrado) {
            printf("\nItem com chave %d encontrado!\n", chave_procurada);
            printf("Chave: %d\n", resultado.chave);
            printf("Dado 1: %ld\n", resultado.dado1);
            printf("Dado 2: %.100s...\n", resultado.dado2);
            printf("Dado 3: %.100s...\n", resultado.dado3);
        } else {
            printf("\nItem com chave %d NAO encontrado.\n", chave_procurada);
        }
    }

    printf("\nAnálise dos dados da Arvore Binaria:\n");
    printf("--- FASE DE CRIAÇÃO DO ÍNDICE ---\n");
    printf("Tempo Inserção: %.6f segundos | Transferências Inserção: %d | Comparações Inserção: %d\n",
           (double)analiseCriacao.tempoExecucao / 1000000.0,
           analiseCriacao.numTransferencias,
           analiseCriacao.numComparacoes);

    if (chave_procurada != -1) {
        printf("\n--- FASE DE PESQUISA SIMPLES ---\n");
        printf("Tempo Pesquisa: %.6f segundos | Transferências Pesquisa: %d | Comparações Pesquisa: %d\n",
               (double)analisePesquisa.tempoExecucao / 1000000.0,
               analisePesquisa.numTransferencias,
               analisePesquisa.numComparacoes);
    } else {
        printf("\n--- FASE DE PESQUISA (MEDIA DE 10 BUSCAS) ---\n");
        printf("Tempo Pesquisa Medio: %.6f segundos | Transferências Pesquisa Media: %.2f | Comparações Pesquisa Media: %.2f\n",
               (double)analisePesquisaTotal.tempoExecucao / 10.0 / 1000000.0,
               (double)analisePesquisaTotal.numTransferencias / 10.0,
               (double)analisePesquisaTotal.numComparacoes / 10.0);
    }

    if (raizArvoreBin) {
        liberarArvoreBin(raizArvoreBin);
    }
}