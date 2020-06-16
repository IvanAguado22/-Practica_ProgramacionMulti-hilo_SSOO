#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/stat.h>
#include <pthread.h>
#include "queue.h"
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define NUM_CONSUMERS 1 // Consumidor

int loadFile(const char *filePath, element ***elements) // Método para abrir archivo y almacenar sus valores
{

    FILE *file = fopen(filePath, "r"); // Abre archivo

    if (file == NULL) // Control de error al abrir el archivo
    {
        fprintf(stderr, "[ERROR]: No se pudo abrir el fichero. \n"); // Saca por pantalla el error
        exit(-1);                                                    // Finaliza ejecución
    }

    int numElements;
    int read = fscanf(file, "%d", &numElements); // Leemos la primera linea y la guardamos en numElements

    if (read == EOF || read == 0) // Control de errores al leer
    {
        fprintf(stderr, "[ERROR]: No se pudo leer el número de operaciones. \n");
        exit(-1);
    }

    if (numElements <= 0) // Control de errores, el número de operaciones debe ser al menos 1
    {
        fprintf(stderr, "[ERROR]: Debe insertar al menos una operación a realizar. \n");
        exit(-1);
    }

    *elements = (element **)malloc(sizeof(element *) * numElements); // Asignamos memoria

    int id;
    int i = 0;
    while (i < numElements) // Bucle para leer el resto de líneas y guardarlas.
    {
        element *elem = (element *)malloc(sizeof(element)); // Asignamos memoria

        read = fscanf(file, "%d", &id); // Leemos y guardamos el id
        if (read == EOF || read == 0)   // Control de errores al leer
        {
            fprintf(stderr, "[ERROR]: No se pudo leer el valor de id. \n");
            exit(-1);
        }

        read = fscanf(file, "%d", &elem->type); // Leemos y guardamos el tipo
        if (read == EOF || read == 0)           // Control de errores al leer
        {
            fprintf(stderr, "[ERROR]: No se pudo leer el valor de tipo. \n");
            exit(-1);
        }

        read = fscanf(file, "%d", &elem->time); // Leemos y guardamos el tiempo
        if (read == EOF || read == 0)           // Control de errores al leer
        {
            fprintf(stderr, "[ERROR]: No se pudo leer el valor de tiempo. \n");
            exit(-1);
        }

        *(*elements + i) = elem; // Lo añadimos al array
        i++;
    }

    fclose(file);       // Cerramos archivo
    return numElements; // Devolvemos el número de operaciones
}

typedef struct producerArgs // Argumentos de los productores
{
    element **elements;
    int start; // Índice de inicio de sus operaciones
    int end;   // Índice de final de sus operaciones
    queue *queue;
} producerArgs;

void produce(producerArgs *args) // Método de los hilos productores
{
    int i = args->start;
    while (i < args->end) // Añade sus operaciones a la cola
    {
        queue_put(args->queue, *(args->elements + i));
        i++;
    }
    pthread_exit(0); // Finalizamos hilo
}

typedef struct consumerArgs // Argumentos del consumidor
{
    queue *queue;
    int numElems;
} consumerArgs;

void consume(consumerArgs *args) // Método del hilo consumidor
{
    int total = 0;
    int i = 0;

    while (i < args->numElems) // Va desencolando las operaciones, calcula su coste y lo añade al total
    {
        element *elem = queue_get(args->queue);
        if (elem->type != 1 && elem->type != 2 && elem->type != 3)
        {
            fprintf(stderr, "[ERROR]: El valor de tipo es incorecto (1 para nodo común, 2 para nodo de cómputo y 3 para supercomputador). \n");
            exit(-1);
        }
        if (elem->type == 1)
        {
            total += 1 * elem->time;
        }
        if (elem->type == 2)
        {
            total += 3 * elem->time;
        }
        if (elem->type == 3)
        {
            total += 10 * elem->time;
        }

        free(elem); // Liberamos el bloque de memoria asignado
        i++;
    }

    printf("Total: %d €.\n", total); // Imprime por pantalla el resultado final
    pthread_exit(0);                 // Finaliza el hilo
}

int main(int argc, const char *argv[])
{
    if (argc != 4) // Control de errores para el número de argumentos
    {
        fprintf(stderr, "[ERROR]: El número de argumentos debe ser 4 (./calculator, archivo, número de productores, tamaño del buffer). \n");
        exit(-1);
    }

    const char *fileName = argv[1];   // Leemos y guardamos el nombre del archivo
    int numProducers = atoi(argv[2]); // Leemos, transformamos a entero y guardamos el número de productores
    if (numProducers < 1)             // Control de errores número de productores
    {
        fprintf(stderr, "[ERROR]: Número de productores no válido. \n");
        exit(-1);
    }

    int buffSize = atoi(argv[3]); // Leemos, transformamos a entero y guardamos el tamaño del buffer
    if (buffSize <= 0)            // Control de errores del tamaño del buffer
    {
        fprintf(stderr, "[ERROR]: Tamaño del buffer no válido. \n");
        exit(-1);
    }

    // Comenzamos operaciones

    element **elements; // Creamos un elemento

    int numElements = loadFile(fileName, &elements); //Leemos el archivo

    // Realizamos un reparto de las mismas entre los productores

    int *distributions = (int *)malloc(sizeof(int) * numProducers); // Asignamos memoria
    int aux = numElements / numProducers;                           // Cantidad de operaciones para cada productor, a falta de las restantes
    int mod = numElements % numProducers;                           // Operaciones restantes

    int j = 0;
    int i = 0;

    while (i < numProducers) // Asignamos operaciones a cada productor
    {
        *(distributions + i) = j;
        j += aux;
        if (i < mod) // Las operaciones restantes se van añadiendo a cada productor, una para cada uno
        {
            j++;
        }
        i++;
    }

    queue *queue = queue_init(buffSize); // Creamos la cola

    pthread_t id;
    producerArgs *pArgs;

    i = 0; // Restauramos contador para reutilizarlo

    while (i < numProducers - 1) // Asignamos a cada productor sus argumentos para posteriormente pasarlos a la hora de crear el hilo
    {
        pArgs = (producerArgs *)malloc(sizeof(producerArgs)); // Asignamos memoria
        pArgs->elements = elements;
        pArgs->start = *(distributions + i);
        pArgs->end = *(distributions + i + 1);
        pArgs->queue = queue;
        pthread_create(&id, NULL, (void *)produce, pArgs); // Creamos hilo productor
        i++;
    }

    // Último productor fuera del bucle para controlar el reparto
    pArgs = (producerArgs *)malloc(sizeof(producerArgs));
    pArgs->elements = elements;
    pArgs->start = *(distributions + numProducers - 1);
    pArgs->end = numElements;
    pArgs->queue = queue;
    pthread_create(&id, NULL, (void *)produce, pArgs);

    consumerArgs cArgs = {queue, numElements};          // Asignamos argumentos al consumidor
    pthread_create(&id, NULL, (void *)consume, &cArgs); // Creamos hilo consumidor

    pthread_join(id, NULL); // Espera al termino del hilo

    exit(0); // Finalizamos ejecución
}