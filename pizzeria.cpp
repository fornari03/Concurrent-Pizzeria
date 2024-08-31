#include <bits/stdc++.h>
#include <pthread.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>

using namespace std;

typedef array<int,3> pizza;         // define o tipo pizza como um array de 3 inteiros, representando os ingredientes
typedef pair<int, pizza> pedido;    // define o tipo pedido como um pair de um id de cliente e uma pizza

// tipos de pizzas e cardápio contendo todas
pizza pizza_mucarela = {1, 0, 0};
pizza pizza_calabresa = {1, 1, 0};
pizza pizza_margheritta = {1, 0, 1};
pizza cardapio[3] = {pizza_mucarela, pizza_calabresa, pizza_margheritta};

// funções a serem implementadas
void * cozinheiro(void * meuid);
void * entregador(void * meuid);
void * cliente(void * meuid);

// mutex para garantir a exclusão mútua
pthread_mutex_t mutex_valores = PTHREAD_MUTEX_INITIALIZER;

// conds para as respectivas threads
pthread_cond_t cliente_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t cozinheiro_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t entregador_cond = PTHREAD_COND_INITIALIZER;

// ESTOQUE DA PIZZARIA
int quant_queijo = 15;
int quant_calabresa = 25;
int quant_tomate = 20;

// NUMERO DE FUNCIONARIOS E CLIENTES
const int quant_cozinheiros = 3;
const int quant_entregadores = 2;
const int quant_clientes = 5;

// FILA DE PEDIDOS A SEREM FEITOS E PEDIDOS PRONTOS
queue<pedido> fila_pedidos_a_fazer;
queue<pedido> fila_pedidos_prontos;

// STATUS CLIENTES
// um vetor que contém o status de cada cliente pelo seu id, sendo:
// 0: cliente não fez o pedido ainda
// 1: cliente fez o pedido e está esperando a pizza
// -1: cliente não vai receber a pizza
array<int, quant_clientes> status_clientes;     // inicializado com 0

/*
* Função que escolhe uma pizza aleatória do cardápio
* @return pizza: sabor de pizza escolhido
*/
pizza escolhe_pizza() {
    return cardapio[rand()%3];
}

/*
* Função que retorna o sabor da pizza em string
* @param pizza: sabor da pizza
* @return string: sabor da pizza em string
*/
string get_sabor_pizza(pizza pizza) {
    if (pizza[1]) return "calabresa";
    else if (pizza[2]) return "margheritta";
    return "mucarela";
}

/*
* Função que verifica se é possível fazer uma pizza
* @param pizza: sabor da pizza
* @return int: 1 se é possível fazer a pizza, 0 se não é possível, -1 se acabou o queijo e não é possível fazer mais nenhuma pizza
*/
int verifica_ingredientes(pizza pizza) {
    if (!quant_queijo)       // se acabou o queijo, não tem mais nenhuma pizza
        return -1;
    if (!quant_calabresa && pizza[1])   // se acabou outro ingrediente da pizza, não pode fazer ela
        return 0;
    if (!quant_tomate && pizza[2])      // se acabou outro ingrediente da pizza, não pode fazer ela
        return 0;
    return 1;                               // pizza pode ser feita
}

/*
* Função que prepara uma pizza, retirando os ingredientes do estoque
* @param pizza: sabor da pizza
*/
void prepara_pizza(pizza pizza) {
    quant_queijo -= pizza[0];
    quant_calabresa -= pizza[1];
    quant_tomate -= pizza[2];
}



// função principal
int main(void) {

    int erro;
    int i, n, m;
    int *id;

    // cria o vetor de threads de cozinheiros
    pthread_t tCozinheiros[quant_cozinheiros];

    for (i = 0; i < quant_cozinheiros; i++) {
        id = (int *) malloc(sizeof(int));
        *id = i;
        // cria a thread de um cozinheiro com seu id
        erro = pthread_create(&tCozinheiros[i], NULL, cozinheiro, (void *) (id));

        // verifica se houve erro na criação da thread
        if(erro) {
            printf("erro na criacao do thread %d\n", i);
            exit(1);
        }
    }

    // cria o vetor de threads de entregadores
    pthread_t tEntregadores[quant_entregadores];

    for (i = 0; i < quant_entregadores; i++) {
        id = (int *) malloc(sizeof(int));
        *id = i;
        // cria a thread de um entregador com seu id
        erro = pthread_create(&tEntregadores[i], NULL, entregador, (void *) (id));

        // verifica se houve erro na criação da thread
        if(erro) {
            printf("erro na criacao do thread %d\n", i);
            exit(1);
        }
    }

    // cria o vetor de threads de clientes
    pthread_t tClientes[quant_clientes];

    for (i = 0; i < quant_clientes; i++) {
        id = (int *) malloc(sizeof(int));
        *id = i;
        // cria a thread de um cliente com seu id
        erro = pthread_create(&tClientes[i], NULL, cliente, (void *) (id));

        // verifica se houve erro na criação da thread
        if(erro) {
            printf("erro na criacao do thread %d\n", i);
            exit(1);
        }
    }

    // espera todas as threads de clientes terminarem para o código terminar também
    for (int i = 0; i < quant_clientes; i++) {
        pthread_join(tClientes[i],NULL);
    }
    
    printf("PROGRAMA ENCERRADO");
    return 0;
}



/*
* Função que simula todo o processo de um cliente fazendo um pedido de pizza e a recebendo ou não
* @param pi: id do cliente
*/
void * cliente(void* pi) {
    while(1) {
        sleep(rand() % 30);                 // cliente liga em um intervalo de tempo aleatório

        pthread_mutex_lock(&mutex_valores);     // adquire o lock do mutex
            srand(time(0));
            // cliente escolhe uma pizza aleatória
            pizza pizza = escolhe_pizza();
            printf("Cliente: %d: quero uma pizza sabor %s\n", *(int *)(pi), get_sabor_pizza(pizza).c_str());
        
            // faz o pedido
            fila_pedidos_a_fazer.push({*(int *)(pi), pizza});
            status_clientes[*(int *)(pi)] = 1;

            // acorda o cozinheiro
            pthread_cond_broadcast(&cozinheiro_cond);

            // espera a pizza ficar pronta ou receber um aviso de que não vai recebê-la
            while (status_clientes[*(int *)(pi)] == 1) {
                pthread_cond_wait(&cliente_cond, &mutex_valores);
            }   

            // não vai receber a pizza
            if (status_clientes[*(int *)(pi)] == -1) {
                printf("Cliente %d: nunca mais peço nessa pizzaria!! \n", *(int *)(pi));
                break;
            }
            // recebe a pizza
            else 
            printf("Cliente %d: recebi minha pizza de sabor %s!! \n", *(int *)(pi), get_sabor_pizza(pizza).c_str());
        pthread_mutex_unlock(&mutex_valores);
    }
    pthread_mutex_unlock(&mutex_valores);       // garante que o mutex é liberado pro caso de um cliente não receber a pizza
    pthread_exit(0);
    return NULL;
}


/*
* Função que simula todo o processo de um cozinheiro fazendo uma pizza
* @param pi: id do cozinheiro
*/
void * cozinheiro(void* pi) {
    while(1)  {
        pthread_mutex_lock(&mutex_valores);         // adquire o lock do mutex
        // espera um pedido de cliente
        while (fila_pedidos_a_fazer.empty())
            pthread_cond_wait(&cozinheiro_cond, &mutex_valores);
        
            // pega o pedido
            pedido pedido = fila_pedidos_a_fazer.front();
            fila_pedidos_a_fazer.pop();
        
            // verifica se é possível fazer a pizza
            int status = verifica_ingredientes(pedido.second);

            // se for possível fazer a pizza
            if (status == 1) {
                printf("Cozinheiro %d: vou fazer uma pizza de sabor %s para o Cliente %d \n", *(int *)(pi), get_sabor_pizza(pedido.second).c_str(), pedido.first);
                // prepara a pizza e libera  lock do mutex
                prepara_pizza(pedido.second);
                pthread_mutex_unlock(&mutex_valores);

                sleep(10);          // simula tempo para fazer a pizza

                printf("Cozinheiro %d: terminei de fazer a pizza de %s para o Cliente %d \n", *(int *)(pi), get_sabor_pizza(pedido.second).c_str(), pedido.first);

                // adquire o lock, coloca a pizza pronta na fila de pedidos prontos e acorda o entregador
                pthread_mutex_lock(&mutex_valores);
                    fila_pedidos_prontos.push(pedido);
                    pthread_cond_broadcast(&entregador_cond);
            }
            // se não for possível fazer a pizza
            else {
                // se acabou o queijo, não aceita mais pedidos
                if (status == -1) {
                    printf("Cozinheiro %d: desculpe, não aceitamos mais pedidos pois acabou o queijo \n", *(int *)(pi));
                }
                // se acabou outro ingrediente, avisa o cliente que não vai receber a pizza
                else {
                    printf("Cozinheiro %d: não temos ingredientes para o sabor %s \n", *(int *)(pi), get_sabor_pizza(pedido.second).c_str());
                }
                // atualiza o status do cliente e acorda ele
                status_clientes[pedido.first] = -1;
                pthread_cond_broadcast(&cliente_cond);
            }
            // libera o lock do mutex
            pthread_mutex_unlock(&mutex_valores);
    }
    pthread_exit(0);

}

/*
* Função que simula todo o processo de um entregador entregando uma pizza
* @param pi: id do entregador
*/
void * entregador(void* pi) {
    while(1)  {
        // adquire o lock do mutex
        pthread_mutex_lock(&mutex_valores);
            // espera um pedido pronto da cozinha
            while (fila_pedidos_prontos.empty())
                pthread_cond_wait(&entregador_cond, &mutex_valores);
            
            // pega o pedido pronto
            pedido pedido = fila_pedidos_prontos.front();
            fila_pedidos_prontos.pop();
        // libera o lock do mutex    
        pthread_mutex_unlock(&mutex_valores);
        printf("Entregador %d: vou entregar a pizza do Cliente %d \n", *(int *)(pi), pedido.first);
        srand(time(0));
        int tempo = 5 + rand()%6;           // tempo de ida e de volta aleatório entre 5 e 10 segundos

        sleep(tempo);
        // atualiza o status do cliente e acorda ele
        status_clientes[pedido.first] = 0;
        pthread_cond_broadcast(&cliente_cond);
        sleep(tempo);
        
        printf("Entregador %d: voltei para a pizzaria \n", *(int *)(pi));
    }
    pthread_exit(0);

}
