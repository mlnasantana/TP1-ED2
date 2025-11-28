#include "arvorebe.h"
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// --- Protótipos das Funções Internas (Estáticas) ---
// Só um lembrete para o compilador do que ele vai encontrar mais pra frente no arquivo.
static Apontador cria_no(bool eh_folha, AnaliseExperimental *analise);
static void insere_com_espaco(Apontador no, Item item, Apontador filho_dir);
static void trata_overflow(Apontador no, Apontador *raiz, AnaliseExperimental *analise);
static void split(Apontador no, Apontador *raiz, AnaliseExperimental *analise);
static bool redistribui(Apontador no, AnaliseExperimental *analise);
static void Imprime_recursivo(Apontador ap, int nivel);
static void Libera_recursivo(Apontador no);


// --- Implementação das Funções ---

// Começa a árvore do zero, sem nenhum nó.
void Inicializa(Apontador *arvore) {
    *arvore = NULL;
}

// Pede um pedacinho de memória pro sistema e prepara um nó novinho em folha.
static Apontador cria_no(bool eh_folha, AnaliseExperimental *analise) {
    analise->numTransferencias++; // Criar um nó conta como uma "escrita" no disco.
    Apontador no = (Apontador)malloc(sizeof(No));
    no->eh_folha = eh_folha;
    no->n = 0;
    no->pai = NULL;
    for(int i=0; i < MAX_CHAVES + 2; i++) no->filhos[i] = NULL;
    return no;
}

// A parte fácil: quando o nó tem espaço, só precisa achar o lugar certo
// e "empurrar" os outros pro lado pra encaixar o novo item.
static void insere_com_espaco(Apontador no, Item item, Apontador filho_dir) {
    int i = no->n - 1;
    // Enquanto o item novo for menor, vai empurrando...
    while (i >= 0 && item.chave < no->itens[i].chave) {
        no->itens[i + 1] = no->itens[i];
        no->filhos[i + 2] = no->filhos[i + 1];
        i--;
    }
    // Achou o lugar! Encaixa o item e o ponteiro da direita dele.
    no->itens[i + 1] = item;
    no->filhos[i + 2] = filho_dir;
    // Avisa o filho (se ele existir) quem é o novo pai dele.
    if (filho_dir) filho_dir->pai = no;
    no->n++;
}

// A função principal de inserção que o programa chama.
void Insere(Item item, Apontador *raiz, AnaliseExperimental *analise) {
    // Se a árvore nem existe ainda, cria a raiz, bota o primeiro item e pronto.
    if (*raiz == NULL) {
        *raiz = cria_no(true, analise);
        (*raiz)->itens[0] = item;
        (*raiz)->n = 1;
        return;
    }

    // Se a árvore já existe, a gente começa pela raiz e vai descendo...
    Apontador no = *raiz;
    analise->numTransferencias++; // Acessou a raiz.
    
    // ...de galho em galho (nós internos)...
    while (!no->eh_folha) {
        int i = no->n - 1;
        // Qual filho pegar? Procura o caminho certo.
        while (i >= 0) {
            analise->numComparacoes++;
            if (item.chave >= no->itens[i].chave) break;
            i--;
        }
        no = no->filhos[i + 1]; // Desce para o próximo nível.
        analise->numTransferencias++;
    }

    // ...até achar a folha certa. Agora insere o item lá.
    insere_com_espaco(no, item, NULL);
    
    // Epa, a folha ficou cheia demais? (Overflow!)
    // Se sim, chama o "resolvedor de problemas".
    if (no->n > MAX_CHAVES) {
        trata_overflow(no, raiz, analise);
    }
}

// O "resolvedor de problemas". A MÁGICA da B* acontece aqui.
static void trata_overflow(Apontador no, Apontador *raiz, AnaliseExperimental *analise) {
    // A primeira opção da B* é sempre ser "educada" e tentar redistribuir com os vizinhos.
    // Se a função 'redistribui' retornar 'true', ela conseguiu resolver e nosso trabalho aqui acabou.
    if (no->pai != NULL && redistribui(no, analise)) {
        return; // Sucesso, redistribuição resolveu o overflow!
    }

    // Se não tem pai (é a raiz) ou se a redistribuição falhou (vizinhos lotados),
    // não tem jeito... vamos para o plano B, que é o 'split' da Árvore B.
    split(no, raiz, analise);
}

// A FUNÇÃO "PLUS"! A essência da B*. Tenta evitar um split caro.
static bool redistribui(Apontador no, AnaliseExperimental *analise) {
    Apontador pai = no->pai;
    int i = 0;
    // Descobre qual a posição do nosso nó problemático na lista de filhos do pai.
    while(i <= pai->n && pai->filhos[i] != no) i++;

    // Tenta "emprestar" um espacinho do irmão da ESQUERDA.
    if (i > 0 && pai->filhos[i - 1]->n < MAX_CHAVES) {
        analise->numTransferencias += 1; // Acesso ao irmão
        Apontador irmao_esq = pai->filhos[i - 1];
        
        // Faz uma "rotação":
        // 1. Pega o primeiro item do nosso nó lotado.
        Item item_subindo = no->itens[0];
        Apontador primeiro_filho_do_no = no->filhos[0];

        // 2. A chave que separava os nós no pai desce para o final do irmão esquerdo.
        insere_com_espaco(irmao_esq, pai->itens[i-1], primeiro_filho_do_no);

        // 3. O item que pegamos do nosso nó sobe para o pai.
        pai->itens[i-1] = item_subindo;
        
        // 4. Limpa o item que subiu do nosso nó, deslocando todo mundo.
        no->n--;
        for (int j = 0; j < no->n; j++) no->itens[j] = no->itens[j + 1];
        for (int j = 0; j <= no->n; j++) no->filhos[j] = no->filhos[j + 1];
        
        return true; // Deu certo!
    }

    // Se não deu com a esquerda, tenta com o irmão da DIREITA.
    if (i < pai->n && pai->filhos[i + 1]->n < MAX_CHAVES) {
        analise->numTransferencias += 1; // Acesso ao irmão
        Apontador irmao_dir = pai->filhos[i + 1];
        
        // Pega o último item do nó lotado.
        Item item_subindo = no->itens[no->n - 1];
        Apontador ultimo_filho_do_no = no->filhos[no->n];
        no->n--; // Já podemos diminuir o tamanho dele.
        
        // A chave do pai desce para o começo do irmão direito.
        insere_com_espaco(irmao_dir, pai->itens[i], ultimo_filho_do_no);
        
        // O último item que pegamos do nó lotado sobe para o pai.
        pai->itens[i] = item_subindo;
        
        return true; // Deu certo!
    }

    return false; // Nenhum irmão pôde ajudar.
}

// O Plano B: se a redistribuição falhou, a gente racha o nó no meio.
static void split(Apontador no, Apontador *raiz, AnaliseExperimental *analise) {
    int meio = no->n / 2;
    // Cria um irmão novinho pra receber a metade dos itens.
    Apontador novo_irmao = cria_no(no->eh_folha, analise);
    
    // O item do meio da lista vai ser promovido pro pai.
    Item item_promovido = no->itens[meio];
    
    // Copia a segunda metade dos itens e filhos para o novo irmão.
    novo_irmao->n = no->n - meio - 1;
    for (int i = 0; i < novo_irmao->n; i++) {
        novo_irmao->itens[i] = no->itens[meio + 1 + i];
        novo_irmao->filhos[i] = no->filhos[meio + 1 + i];
        if(novo_irmao->filhos[i]) novo_irmao->filhos[i]->pai = novo_irmao;
    }
    novo_irmao->filhos[novo_irmao->n] = no->filhos[no->n];
    if(novo_irmao->filhos[novo_irmao->n]) novo_irmao->filhos[novo_irmao->n]->pai = novo_irmao;

    // O nó original agora só tem a primeira metade.
    no->n = meio;
    
    // Agora, vamos inserir o item promovido no pai.
    Apontador pai = no->pai;
    // Se não tinha pai (era a raiz), precisamos criar um pai novo. A árvore cresce!
    if (pai == NULL) {
        pai = cria_no(false, analise);
        *raiz = pai;
        pai->filhos[0] = no;
        no->pai = pai;
    }

    // Insere o item promovido e o ponteiro pro novo irmão no pai.
    novo_irmao->pai = pai;
    insere_com_espaco(pai, item_promovido, novo_irmao);

    // Epa, o pai também ficou cheio demais? Chamamos o 'resolvedor' pra ele também! (Recursão)
    if(pai->n > MAX_CHAVES) {
       trata_overflow(pai, raiz, analise);
    }
}

// Procurar é sempre mais fácil. Começa na raiz e vai descendo...
bool Pesquisa(Item *item, Apontador no, AnaliseExperimental *analise) {
    if (no == NULL) return false;
    analise->numTransferencias++;
    int i = 0;
    // Em cada nó, procura o caminho certo...
    while (i < no->n) {
        analise->numComparacoes++;
        if (item->chave < no->itens[i].chave) break; // Achei o caminho, é pra esquerda
        analise->numComparacoes++;
        if (item->chave == no->itens[i].chave) { // Epa, achei a própria chave!
            *item = no->itens[i];
            return true;
        }
        i++; // Se não, continuo procurando no mesmo nó.
    }
    // Se cheguei numa folha e não achei, o item não existe.
    if (no->eh_folha) return false;
    // Se não, desce pro filho que a gente encontrou.
    return Pesquisa(item, no->filhos[i], analise);
}

// Libera a memória da árvore, um nó de cada vez, de baixo pra cima.
static void Libera_recursivo(Apontador no) {
    if (no == NULL) return;
    if (!no->eh_folha) {
        for (int i = 0; i <= no->n; i++) Libera_recursivo(no->filhos[i]);
    }
    free(no);
}

void Libera(Apontador *arvore) {
    Libera_recursivo(*arvore);
    *arvore = NULL;
}

// Imprime a árvore nível por nível, pra gente conseguir visualizar.
static void Imprime_recursivo(Apontador ap, int nivel) {
    if (ap == NULL) return;
    for (int i = 0; i < nivel; i++) printf("  ");
    printf("Nivel %d [n=%d]: ", nivel, ap->n);
    for (int i = 0; i < ap->n; i++) printf("%d ", ap->itens[i].chave);
    printf("\n");
    if (!ap->eh_folha) {
        for (int i = 0; i <= ap->n; i++) Imprime_recursivo(ap->filhos[i], nivel + 1);
    }
}

void Imprime(Apontador arvore) {
    printf("--- ESTRUTURA DA ÁRVORE ---\n");
    Imprime_recursivo(arvore, 0);
    printf("---------------------------\n");
}


// A função principal que seu main.c chama. Ela orquestra tudo.
void RodaArvoreBEstrela(FILE *arq, int chave_procurada, int quantReg, bool P_flag) {
    Apontador arvore;
    Item registro_lido;
    AnaliseExperimental analise_criacao = {0, 0, 0.0};
    AnaliseExperimental analise_pesquisa = {0, 0, 0.0};
    clock_t inicio, fim;
    Inicializa(&arvore);

    // ETAPA 1: Construir a árvore lendo o arquivo.
    printf("\n[Método 4: Árvore B*] Iniciando criação do índice a partir do arquivo...\n");
    rewind(arq);
    inicio = clock();
    while (fread(&registro_lido, sizeof(Item), 1, arq) == 1) {
        Insere(registro_lido, &arvore, &analise_criacao);
    }
    fim = clock();
    analise_criacao.tempoExecucao = ((double)(fim - inicio)) / CLOCKS_PER_SEC;
    printf("[Árvore B*] Criação concluída.\n");

    // Se o usuário pediu pra ver a árvore com a flag -P
    if (P_flag) {
        Imprime(arvore);
    }

    // ETAPA 2: Fazer a pesquisa, seja a simples ou o experimento.
    if (chave_procurada != -1) { // MODO DE BUSCA SIMPLES
        Item item_pesquisado;
        item_pesquisado.chave = chave_procurada;
        printf("\nIniciando pesquisa pela chave %d...\n", chave_procurada);
        inicio = clock();
        bool encontrado = Pesquisa(&item_pesquisado, arvore, &analise_pesquisa);
        fim = clock();
        analise_pesquisa.tempoExecucao = ((double)(fim - inicio)) / CLOCKS_PER_SEC;
        if (encontrado) {
            printf("\n-> Item encontrado:\n   Chave: %d\n", item_pesquisado.chave);
        } else {
            printf("\n-> Item com chave %d nao encontrado.\n", chave_procurada);
        }
        printf("\n========== ANÁLISE DE DESEMPENHO (Busca Única) ==========\n");
        printf("[CRIAÇÃO DO ÍNDICE]\n  - Tempo: %f s\n  - Comparações: %d\n  - Transferências: %d\n", analise_criacao.tempoExecucao, analise_criacao.numComparacoes, analise_criacao.numTransferencias);
        printf("[PESQUISA]\n  - Tempo: %f s\n  - Comparações: %d\n  - Transferências: %d\n", analise_pesquisa.tempoExecucao, analise_pesquisa.numComparacoes, analise_pesquisa.numTransferencias);
        printf("========================================================\n");
    } else { // MODO EXPERIMENTO
        printf("\nIniciando modo de experimento: 10 buscas automáticas...\n");
        AnaliseExperimental analise_total = {0, 0, 0.0};
        int chaves_a_pesquisar[10];
        for(int i=0; i < 10; i++) chaves_a_pesquisar[i] = (i + 1) * (quantReg / 11);
        for(int i=0; i < 10; i++) {
            AnaliseExperimental analise_temp = {0, 0, 0.0};
            Item res_temp;
            res_temp.chave = chaves_a_pesquisar[i];
            clock_t start_p = clock();
            Pesquisa(&res_temp, arvore, &analise_temp);
            clock_t end_p = clock();
            analise_total.numComparacoes += analise_temp.numComparacoes;
            analise_total.numTransferencias += analise_temp.numTransferencias;
            analise_total.tempoExecucao += ((double)(end_p - start_p)) / CLOCKS_PER_SEC;
        }
        printf("\n========== ANÁLISE DE DESEMPENHO (Média de 10 Buscas) ==========\n");
        printf("[CRIAÇÃO DO ÍNDICE]\n  - Tempo: %f s\n  - Comparações: %d\n  - Transferências: %d\n", analise_criacao.tempoExecucao, analise_criacao.numComparacoes, analise_criacao.numTransferencias);
        printf("[PESQUISA - MÉDIAS]\n  - Tempo (médio): %f s\n  - Comparações (média): %.2f\n  - Transferências (média): %.2f\n", analise_total.tempoExecucao / 10.0, (double)analise_total.numComparacoes / 10.0, (double)analise_total.numTransferencias / 10.0);
        printf("===============================================================\n");
    }

    // ETAPA 3: Limpar a memória pra não deixar sujeira pra trás.
    Libera(&arvore);
}