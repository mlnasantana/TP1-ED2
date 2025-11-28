#include "arvoreb.h"
#include "item.h"
B_No* B_criaNo(bool folha) {
    B_No* no = (B_No*)malloc(sizeof(B_No));
    no->qtdChaves = 0;
    no->folha = folha;
    for (int i = 0; i <= MAX; i++) {
        no->filhos[i] = NULL;
    }
    return no;
}

bool B_arvoreBusca(B_No* raiz, int chave, Item* resultado, AnaliseExperimental* analise) {
    int i = 0;

    if (raiz == NULL) {
        return false;
    }

    // Inicializa o resultado para evitar comportamento indefinido
    if (resultado != NULL) {
        resultado->chave = -1;
    }

    while (i < raiz->qtdChaves) {
        analise->numComparacoes++;
        if (chave == raiz->chaves[i]) {
            return true;
        } else if (chave > raiz->chaves[i]) {
            i++;
        } else {
            break;
        }
    }

    if (raiz->folha) {
        return false;
    }

    return B_arvoreBusca(raiz->filhos[i], chave, resultado, analise);
}

void B_splitFilho(B_No* pai, int i, B_No* filho) {
    B_No* novo = B_criaNo(filho->folha);
    novo->qtdChaves = M - 1;  // Correção: novo nó recebe M-1 chaves

    // Copia as últimas M-1 chaves do filho para o novo nó
    for (int j = 0; j < M - 1; j++) {
        novo->chaves[j] = filho->chaves[j + M + 1];
    }

    // Se não for folha, copia os últimos M filhos
    if (!filho->folha) {
        for (int j = 0; j < M; j++) {
            novo->filhos[j] = filho->filhos[j + M + 1];
        }
    }

    filho->qtdChaves = M;  // Correção: nó original agora tem M chaves

    // Desloca filhos do pai para abrir espaço para o novo filho
    for (int j = pai->qtdChaves; j >= i + 1; j--) {
        pai->filhos[j + 1] = pai->filhos[j];
    }
    pai->filhos[i + 1] = novo;

    // Desloca chaves do pai para abrir espaço para a nova chave
    for (int j = pai->qtdChaves - 1; j >= i; j--) {
        pai->chaves[j + 1] = pai->chaves[j];
    }
    pai->chaves[i] = filho->chaves[M];
    pai->qtdChaves++;
}

void B_insereNaoCheio(B_No* no, Item item, AnaliseExperimental* analise) {
    int i = no->qtdChaves - 1;
    int chave = item.chave;

    if (no->folha) {
        while (i >= 0 && chave < no->chaves[i]) {
            analise->numComparacoes++;
            no->chaves[i + 1] = no->chaves[i];
            i--;
        }
        if (i >= 0) {
            analise->numComparacoes++;  // Conta a última comparação
        }
        no->chaves[i + 1] = chave;
        no->qtdChaves++;
    } else {
        while (i >= 0 && chave < no->chaves[i]) {
            analise->numComparacoes++;
            i--;
        }
        i++;
        // Removida comparação redundante
        if (no->filhos[i] != NULL && no->filhos[i]->qtdChaves == MAX) {
            B_splitFilho(no, i, no->filhos[i]);
            if (chave > no->chaves[i]) {
                i++;
            }
        }
        B_insereNaoCheio(no->filhos[i], item, analise);
    }
}

void B_arvoreInsere(ArvoreB* arvore, Item item, AnaliseExperimental* analise) {
    if (*arvore == NULL) {
        *arvore = B_criaNo(true);
    }

    B_No* raiz = *arvore;

    if (raiz->qtdChaves == MAX) {
        B_No* novaRaiz = B_criaNo(false);
        novaRaiz->filhos[0] = raiz;
        B_splitFilho(novaRaiz, 0, raiz);
        B_insereNaoCheio(novaRaiz, item, analise);
        *arvore = novaRaiz;
    } else {
        B_insereNaoCheio(raiz, item, analise);
    }
}

void B_liberaArvoreB(ArvoreB arvore) {
    if (arvore == NULL) return;
    for (int i = 0; i <= arvore->qtdChaves; i++) {
        B_liberaArvoreB(arvore->filhos[i]);
    }
    free(arvore);
}

void RodaArvoreB(const char* nomeArq, int chave_procurada, int quantReg, bool P_flag) {
    ArvoreB arvoreB = NULL;
    Item item_lido;
    AnaliseExperimental analiseCriacao = {0, 0, 0};

    FILE* arquivoCriacao = fopen(nomeArq, "rb");
    if (arquivoCriacao == NULL) {
        perror("Erro ao abrir arquivo para criação da Arvore B");
        return;
    }

    clock_t start_criacao = clock();
    while (fread(&item_lido, sizeof(Item), 1, arquivoCriacao) == 1) {
        analiseCriacao.numTransferencias++;
        B_arvoreInsere(&arvoreB, item_lido, &analiseCriacao);
    }
    clock_t end_criacao = clock();
    analiseCriacao.tempoExecucao = (long)(((double)(end_criacao - start_criacao)) / CLOCKS_PER_SEC * 1000000.0);

    fclose(arquivoCriacao);

    AnaliseExperimental analisePesquisa = {0, 0, 0};
    bool encontrado = false;
    Item resultado;

    if (chave_procurada != -1) {
        FILE* arquivoParaPesquisa = fopen(nomeArq, "rb");
        if (arquivoParaPesquisa == NULL) {
            perror("Erro ao abrir arquivo para pesquisa na Arvore B");
            B_liberaArvoreB(arvoreB);
            return;
        }

        clock_t start_pesquisa = clock();
        encontrado = B_arvoreBusca(arvoreB, chave_procurada, &resultado, &analisePesquisa);
        clock_t end_pesquisa = clock();
        analisePesquisa.tempoExecucao = (long)(((double)(end_pesquisa - start_pesquisa)) / CLOCKS_PER_SEC * 1000000.0);

        if (encontrado) {
            rewind(arquivoParaPesquisa);
            Item temp_item;
            while(fread(&temp_item, sizeof(Item), 1, arquivoParaPesquisa) == 1) {
                if (temp_item.chave == chave_procurada) {
                    resultado = temp_item;
                    analisePesquisa.numTransferencias++;
                    break;
                }
            }
        }
        fclose(arquivoParaPesquisa);

        printf("\nItem com chave %d %s encontrado.\n", chave_procurada, encontrado ? "foi" : "NAO foi");
        if (encontrado) {
            printf("Chave: %d\n", resultado.chave);
            printf("Dado 1: %ld\n", resultado.dado1);
            printf("Dado 2: %.100s...\n", resultado.dado2);
            printf("Dado 3: %.100s...\n", resultado.dado3);
        }
    } else {
        // Rodar experimento com 10 buscas médias se chave_procurada == -1
        int chaves_a_pesquisar[10];
        for (int i = 0; i < 10; i++) {
            chaves_a_pesquisar[i] = (i + 1) * (quantReg / 10);
            if (chaves_a_pesquisar[0] == 0 && quantReg > 0) chaves_a_pesquisar[0] = 1;
            if (chaves_a_pesquisar[i] == 0 && i > 0 && quantReg > 0) chaves_a_pesquisar[i] = chaves_a_pesquisar[i - 1] + 1;
            if (chaves_a_pesquisar[i] > quantReg && quantReg > 0) chaves_a_pesquisar[i] = quantReg;
        }

        AnaliseExperimental analisePesquisaTotal = {0, 0, 0};

        for (int i = 0; i < 10; i++) {
            AnaliseExperimental analiseTemp = {0, 0, 0};
            FILE* arquivoParaPesquisa = fopen(nomeArq, "rb");
            if (arquivoParaPesquisa == NULL) {
                perror("Erro ao abrir arquivo para pesquisa na Arvore B (modo -E)");
                B_liberaArvoreB(arvoreB);
                return;
            }

            clock_t start_p = clock();
            bool found = B_arvoreBusca(arvoreB, chaves_a_pesquisar[i], &resultado, &analiseTemp);
            clock_t end_p = clock();
            analiseTemp.tempoExecucao = (long)(((double)(end_p - start_p)) / CLOCKS_PER_SEC * 1000000.0);

            if (found) {
                rewind(arquivoParaPesquisa);
                Item temp_item;
                while (fread(&temp_item, sizeof(Item), 1, arquivoParaPesquisa) == 1) {
                    if (temp_item.chave == chaves_a_pesquisar[i]) {
                        analiseTemp.numTransferencias++;
                        break;
                    }
                }
            }
            fclose(arquivoParaPesquisa);

            analisePesquisaTotal.numComparacoes += analiseTemp.numComparacoes;
            analisePesquisaTotal.numTransferencias += analiseTemp.numTransferencias;
            analisePesquisaTotal.tempoExecucao += analiseTemp.tempoExecucao;
        }

        printf("\nAnálise dos dados da Arvore B (média de 10 buscas):\n");
        printf("Tempo Médio Pesquisa: %.6f segundos\n", (double)analisePesquisaTotal.tempoExecucao / 10.0 / 1000000.0);
        printf("Transferências Médias: %.2f\n", (double)analisePesquisaTotal.numTransferencias / 10.0);
        printf("Comparações Médias: %.2f\n", (double)analisePesquisaTotal.numComparacoes / 10.0);
    }

    printf("\n--- FASE DE CRIAÇÃO DO ÍNDICE ---\n");
    printf("Tempo Inserção: %.6f segundos | Transferências Inserção: %d | Comparações Inserção: %d\n",
           (double)analiseCriacao.tempoExecucao / 1000000.0,
           analiseCriacao.numTransferencias,
           analiseCriacao.numComparacoes);

    B_liberaArvoreB(arvoreB);
}