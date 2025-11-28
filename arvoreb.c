// ESTA LINHA DEVE SER A PRIMEIRA DO ARQUIVO
#define _FILE_OFFSET_BITS 64

#include "arvoreb.h"
#include "item.h"
#include <string.h> // Essencial para memcpy e memmove
#include <time.h>

// Criação de Nó otimizada com calloc (zera memória automaticamente)
B_No* B_criaNo(bool folha) {
    B_No* no = (B_No*)calloc(1, sizeof(B_No));
    if (no == NULL) {
        perror("Erro fatal: Memoria insuficiente");
        exit(1);
    }
    no->folha = folha;
    // qtdChaves inicia em 0 e filhos em NULL automaticamente pelo calloc
    return no;
}

// Busca mais limpa e direta
FilePos B_arvoreBusca(B_No* raiz, int chave, AnaliseExperimental* analise) {
    if (raiz == NULL) return -1;

    int i = 0;
    // Procura a primeira chave maior ou igual a buscada
    while (i < raiz->qtdChaves && chave > raiz->chaves[i]) {
        analise->numComparacoes++;
        i++;
    }

    // Se achou a chave neste nó
    if (i < raiz->qtdChaves && chave == raiz->chaves[i]) {
        analise->numComparacoes++; // Confirmação da igualdade
        return raiz->offsets[i];
    }

    // Se é folha e não achou, não existe
    if (raiz->folha) return -1;

    // Desce para o filho apropriado
    return B_arvoreBusca(raiz->filhos[i], chave, analise);
}

// Split simplificado com memmove (substitui loops manuais)
void B_splitFilho(B_No* pai, int i, B_No* filho) {
    B_No* novo = B_criaNo(filho->folha);
    novo->qtdChaves = M - 1;

    // 1. Copia metade direita (chaves/offsets) do filho para o novo nó
    memcpy(novo->chaves, &filho->chaves[M + 1], (M - 1) * sizeof(int));
    memcpy(novo->offsets, &filho->offsets[M + 1], (M - 1) * sizeof(FilePos));

    // 2. Se não for folha, copia os filhos correspondentes
    if (!filho->folha) {
        memcpy(novo->filhos, &filho->filhos[M + 1], M * sizeof(B_No*));
    }

    filho->qtdChaves = M; // Ajusta tamanho do filho original

    // 3. Abre espaço no PAI para subir a chave mediana
    // Empurra filhos do pai para a direita
    memmove(&pai->filhos[i + 2], &pai->filhos[i + 1], (pai->qtdChaves - i) * sizeof(B_No*));
    // Empurra chaves e offsets do pai para a direita
    memmove(&pai->chaves[i + 1], &pai->chaves[i], (pai->qtdChaves - i) * sizeof(int));
    memmove(&pai->offsets[i + 1], &pai->offsets[i], (pai->qtdChaves - i) * sizeof(FilePos));

    // 4. Conecta o novo nó e sobe a mediana
    pai->filhos[i + 1] = novo;
    pai->chaves[i] = filho->chaves[M];
    pai->offsets[i] = filho->offsets[M];
    pai->qtdChaves++;
}

// Inserção em nó não cheio (refatorada)
void B_insereNaoCheio(B_No* no, int chave, FilePos offset, AnaliseExperimental* analise) {
    int i = no->qtdChaves - 1;

    if (no->folha) {
        // Encontra a posição correta de trás para frente
        while (i >= 0 && chave < no->chaves[i]) {
            analise->numComparacoes++;
            i--;
        }
        if (i >= 0) analise->numComparacoes++; // Última comparação que falhou o while

        i++; // Posição de inserção

        // Abre espaço usando memmove (mais eficiente que loop)
        memmove(&no->chaves[i + 1], &no->chaves[i], (no->qtdChaves - i) * sizeof(int));
        memmove(&no->offsets[i + 1], &no->offsets[i], (no->qtdChaves - i) * sizeof(FilePos));

        no->chaves[i] = chave;
        no->offsets[i] = offset;
        no->qtdChaves++;
    } else {
        // Busca o filho para descer
        while (i >= 0 && chave < no->chaves[i]) {
            analise->numComparacoes++;
            i--;
        }
        i++;
        
        // Verifica se o filho está cheio antes de descer (Top-Down)
        if (no->filhos[i]->qtdChaves == MAX) {
            B_splitFilho(no, i, no->filhos[i]);
            if (chave > no->chaves[i]) {
                i++;
            }
        }
        B_insereNaoCheio(no->filhos[i], chave, offset, analise);
    }
}

// Função principal de inserção
void B_arvoreInsere(ArvoreB* arvore, int chave, FilePos offset, AnaliseExperimental* analise) {
    if (*arvore == NULL) {
        *arvore = B_criaNo(true);
        (*arvore)->chaves[0] = chave;
        (*arvore)->offsets[0] = offset;
        (*arvore)->qtdChaves = 1;
        return;
    }

    B_No* raiz = *arvore;

    if (raiz->qtdChaves == MAX) {
        B_No* novaRaiz = B_criaNo(false);
        novaRaiz->filhos[0] = raiz;
        B_splitFilho(novaRaiz, 0, raiz);
        B_insereNaoCheio(novaRaiz, chave, offset, analise);
        *arvore = novaRaiz;
    } else {
        B_insereNaoCheio(raiz, chave, offset, analise);
    }
}

// Liberação de memória recursiva
void B_liberaArvoreB(ArvoreB arvore) {
    if (arvore == NULL) return;
    if (!arvore->folha) {
        for (int i = 0; i <= arvore->qtdChaves; i++) {
            B_liberaArvoreB(arvore->filhos[i]);
        }
    }
    free(arvore);
}

// Função Driver (mantida idêntica à original, apenas formatação)
void RodaArvoreB(const char* nomeArq, int chave_procurada, int quantReg, bool P_flag) {
    ArvoreB arvoreB = NULL;
    Item item_lido;
    AnaliseExperimental analiseCriacao = {0, 0, 0};

    // --- FASE 1: CRIACAO ---
    FILE* arquivoCriacao = fopen(nomeArq, "rb");
    if (arquivoCriacao == NULL) { perror("Erro ao abrir arquivo"); return; }

    clock_t start_criacao = clock();
    while (true) {
        FilePos posicao_atual = ftello(arquivoCriacao);
        if (fread(&item_lido, sizeof(Item), 1, arquivoCriacao) != 1) break;
        
        // Opcional: limitar quantidade de registros se quantReg > 0
        // if (quantReg > 0 && analiseCriacao.numTransferencias >= quantReg) break;

        analiseCriacao.numTransferencias++;
        B_arvoreInsere(&arvoreB, item_lido.chave, posicao_atual, &analiseCriacao);
    }
    clock_t end_criacao = clock();
    analiseCriacao.tempoExecucao = (double)(end_criacao - start_criacao) / CLOCKS_PER_SEC;
    fclose(arquivoCriacao);

    printf("\n========================================================\n");
    printf("METODO: 3 - ARVORE B\n");
    printf("========================================================\n");

    printf("--- FASE 1: CRIACAO DO INDICE ---\n");
    printf("Tempo Execucao: %.6f s\n", analiseCriacao.tempoExecucao);
    printf("Transferencias: %d\n", analiseCriacao.numTransferencias);
    printf("Comparacoes:    %d\n", analiseCriacao.numComparacoes);

    // --- FASE 2: PESQUISA ---
    printf("\n--- FASE 2: PESQUISA ---\n");

    FILE* arquivoPesquisa = fopen(nomeArq, "rb");
    if (!arquivoPesquisa) { B_liberaArvoreB(arvoreB); return; }

    AnaliseExperimental analisePesquisa = {0, 0, 0};

    if (chave_procurada != -1) {
        clock_t start_p = clock();
        FilePos offset = B_arvoreBusca(arvoreB, chave_procurada, &analisePesquisa);
        
        bool encontrado = false;
        if (offset != -1) {
            fseeko(arquivoPesquisa, offset, SEEK_SET);
            fread(&item_lido, sizeof(Item), 1, arquivoPesquisa);
            analisePesquisa.numTransferencias++;
            encontrado = true;
        }
        clock_t end_p = clock();
        analisePesquisa.tempoExecucao = (double)(end_p - start_p) / CLOCKS_PER_SEC;

        if (encontrado) {
            printf("[STATUS]: ITEM ENCONTRADO\n");
            printf("> Chave:  %d\n", item_lido.chave);
            printf("> Dado1:  %ld\n", item_lido.dado1);
            if(P_flag) printf("> Dado2:  %.50s...\n", item_lido.dado2);
        } else {
            printf("[STATUS]: ITEM NAO ENCONTRADO (Chave %d)\n", chave_procurada);
        }

        printf("\n--- METRICAS DE PESQUISA ---\n");
        printf("Tempo Execucao: %.6f s\n", analisePesquisa.tempoExecucao);
        printf("Transferencias: %d\n", analisePesquisa.numTransferencias);
        printf("Comparacoes:    %d\n", analisePesquisa.numComparacoes);

    } else {
        printf("Modo experimental (media) nao ativado.\n");
    }
    printf("========================================================\n");

    fclose(arquivoPesquisa);
    B_liberaArvoreB(arvoreB);
}