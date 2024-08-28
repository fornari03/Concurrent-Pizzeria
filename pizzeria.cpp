#include <bits/stdc++.h>
#include <pthread.h>
#include <unistd.h>

using namespace std;

#define pizza_mucarela {1, 0, 0}
#define pizza_calabresa {1, 1, 0}
#define pizza_margheritta {1, 0, 1}
array<int, 3> cardapio[3] = {pizza_mucarela, pizza_calabresa, pizza_margheritta};

void * cozinheiro(void * meuid);
void * entregador(void * meuid);
void * cliente(void * meuid);

pthread_mutex_t mutex_valores = PTHREAD_MUTEX_INITIALIZER;
// FAZER UM MUTEX_valores SÓ PARA OS PEDIDOS????
pthread_cond_t cozinheiro_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t entregador_cond = PTHREAD_COND_INITIALIZER;

// ESTOQUE DA PIZZARIA
int quant_queijo = 50;
int quant_calabresa = 25;
int quant_tomate = 20;
int* ingredientes[3] = {&quant_queijo,&quant_calabresa,&quant_tomate};

// NUMERO DE FUNCIONARIOS
int quant_cozinheiros = 5;
int quant_entregadores = 3;
int quant_clientes = 10;

typedef array<int,3> pizza;
typedef pair<int, pizza> pedido;
// FILA DE PEDIDOS
queue<pedido> fila_pedidos;
queue<pedido> pedidos_prontos;


pizza escolhe_pizza() {
    return cardapio[rand()%3];
}

string get_sabor_pizza(pizza pizza) {
    if (pizza[1]) return "calabresa";
    else if (pizza[2]) return "margheritta";
    return "mucarela";
}

int verifica_ingredientes(pizza pizza) {
    if (!ingredientes[0] && pizza[0])       // se acabou o queijo, não tem mais nenhuma pizza
        return -1;
    for (int i = 1; i < 3; i++) {
        if (!ingredientes[i] && pizza[i])   // se acabou outro ingrediente da pizza, não pode fazer ela
            return 0;
    }
    return 1;                               // pizza pode ser feita
}

void prepara_pizza(pizza pizza) {
    for (int i = 0; i < 3; i++)
        *ingredientes[i] -= pizza[i];
}


int main(void) {

    int erro;
    int i, n, m;
    int *id;

    pthread_t tCozinheiros[quant_cozinheiros];

    for (i = 0; i < quant_cozinheiros; i++) {
        id = (int *) malloc(sizeof(int));
        *id = i;
        erro = pthread_create(&tCozinheiros[i], NULL, cozinheiro, (void *) (id));

        if(erro) {
            printf("erro na criacao do thread %d\n", i);
            exit(1);
        }
    }

    pthread_t tEntregadores[quant_entregadores];

    for (i = 0; i < quant_entregadores; i++) {
        id = (int *) malloc(sizeof(int));
        *id = i;
        erro = pthread_create(&tEntregadores[i], NULL, entregador, (void *) (id));

        if(erro) {
            printf("erro na criacao do thread %d\n", i);
            exit(1);
        }
    }

    pthread_t tClientes[quant_clientes];

    for (i = 0; i < quant_clientes; i++) {
        id = (int *) malloc(sizeof(int));
        *id = i;
        erro = pthread_create(&tClientes[i], NULL, cliente, (void *) (id));

        if(erro) {
            printf("erro na criacao do thread %d\n", i);
            exit(1);
        }
    }

    for (int i = 0; i < quant_clientes; i++) {
        pthread_join(tClientes[i],NULL);
    }
    
    printf("PROGRAMA ENCERRADO");
    return 0;
}

void * cliente(void* pi) {
    while(1) {
        sleep(rand() % 30);
        pizza pizza = escolhe_pizza();
        printf("Cliente: %d: quero uma pizza sabor %s\n", *(int *)(pi), get_sabor_pizza(pizza).c_str());
        
        pthread_mutex_lock(&mutex_valores);
            if (quant_queijo)
                fila_pedidos.push({*(int *)(pi), pizza});
            else {
                printf("Gerente: desculpe, não aceitamos mais pedidos pois acabou o queijo \n");
                break;
            }
        pthread_mutex_unlock(&mutex_valores);
        pthread_cond_broadcast(&cozinheiro_cond);
    }
    pthread_mutex_unlock(&mutex_valores);
    pthread_exit(0);
    return NULL;
}


void * cozinheiro(void* pi) {
    while(1)  {
        pthread_mutex_lock(&mutex_valores);
        while (fila_pedidos.empty())
            pthread_cond_wait(&cozinheiro_cond, &mutex_valores);
        
            pedido pedido = fila_pedidos.front();
            fila_pedidos.pop();
        printf("Cozinheiro %d: vou fazer uma pizza de sabor %s para o Cliente %d \n", *(int *)(pi), get_sabor_pizza(pedido.second).c_str(), pedido.first);
        
            int status = verifica_ingredientes(pedido.second);
            if (status == 1) {
                prepara_pizza(pedido.second);
                pthread_mutex_unlock(&mutex_valores);
                sleep(10);
                printf("Cozinheiro %d: terminei de fazer a pizza de %s para o Cliente %d \n", *(int *)(pi), get_sabor_pizza(pedido.second).c_str(), pedido.first);
                pedidos_prontos.push(pedido);
                pthread_cond_broadcast(&entregador_cond);
            }
            else {
                pthread_mutex_unlock(&mutex_valores);
                printf("Cozinheiro %d: não temos ingredientes para o sabor %s \n", *(int *)(pi), get_sabor_pizza(pedido.second).c_str());
                printf("Cliente %d: nunca mais peço nessa pizzaria!!", pedido.first);
                // if (status == -1) {
                //     printf("Gerente: ACABOU O QUEIJO... NÃO ATENDEMOS MAIS PEDIDOS \n");
                // }
            }
    }
    pthread_exit(0);

}

void * entregador(void* pi) {
    while(1)  {
        pthread_mutex_lock(&mutex_valores);
        while (pedidos_prontos.empty())
            pthread_cond_wait(&entregador_cond, &mutex_valores);
            pedido pedido = pedidos_prontos.front();
            pedidos_prontos.pop();
        pthread_mutex_unlock(&mutex_valores);
        printf("Entregador %d: vou entregar a pizza do cliente %d \n", *(int *)(pi), pedido.first);
        int tempo = 5 + rand()%6;
        sleep(tempo);
        printf("Cliente %d: recebi minha pizza de sabor %s do Entregador %d!! \n", pedido.first, get_sabor_pizza(pedido.second).c_str(), *(int *)(pi));
        sleep(tempo);
        printf("Entregador %d: voltei para a pizzaria \n", *(int *)(pi));
    }
    pthread_exit(0);

}
