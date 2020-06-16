#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "queue.h"

// Método para crear la cola
queue *queue_init(int size)
{
    // Asignamos memoria  para la cola y sus nodos inicial y final.
    queue *q = (queue *)malloc(sizeof(queue));

    q->head = (node *)malloc(sizeof(node));
    q->tail = (node *)malloc(sizeof(node));

    // Inicializamos los valores de la cola y los asignamos un valor null
    q->head->elem = NULL;
    q->head->prev = NULL;
    q->head->next = q->tail;
    q->tail->elem = NULL;
    q->tail->prev = q->head;
    q->tail->next = NULL;

    q->capacity = size; // Capacidad total de la cola
    q->size = 0;        // Cola inicialmente vacía

    // Asignación de memoria para variables concición y mutex
    q->mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    q->isFull = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    q->isEmpty = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));

    // Control de la concurrencia mediante variables condición y mutex
    pthread_mutex_init(q->mutex, NULL);
    pthread_cond_init(q->isFull, NULL);
    pthread_cond_init(q->isEmpty, NULL);

    return q;
}

// Creación de nodos estándar para la cola
node *createNode(node *prev, element *x)
{
    node *newNode = (node *)malloc(sizeof(node));
    newNode->elem = x;
    newNode->prev = prev;
    newNode->next = prev->next;
}

// Insercíon de nodos estándar para la cola
int insertNode(node *prev, element *x)
{
    node *newNode = createNode(prev, x);
    prev->next->prev = newNode;
    prev->next = newNode;
}

// Método para encolar
int queue_put(queue *q, element *x)
{
    pthread_mutex_lock(q->mutex);

    while (q->size == q->capacity) // Espera a poder insertar en la cola
    {
        pthread_cond_wait(q->isFull, q->mutex);
    }

    insertNode(q->tail->prev, x); // Insertamos un nodo en la cola
    q->size++;                    // Aumentamos tamaño de la cola

    // Control de la concurrencia
    pthread_cond_signal(q->isEmpty);
    pthread_mutex_unlock(q->mutex);

    return 0;
}

// Borrado de nodos estándar para la cola
node *removeNode(node *node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
    return node;
}

// Método para desencolar
element *queue_get(queue *q)
{
    pthread_mutex_lock(q->mutex);

    while (q->size == 0) // Espera para poder desencolar
    {
        pthread_cond_wait(q->isEmpty, q->mutex);
    }

    node *first = removeNode(q->head->next); // Elimina un nodo de la cola
    q->size--;                               // Reduce el tamaño de la cola

    // Control de concurrencia
    pthread_cond_signal(q->isFull);
    pthread_mutex_unlock(q->mutex);

    return first->elem;
}

// Para saber si la cola está vacía
int queue_empty(queue *q)
{
    pthread_mutex_lock(q->mutex);
    int empty = q->size == 0;
    pthread_mutex_unlock(q->mutex);

    return empty;
}

// Para saber si la cola está llena
int queue_full(queue *q)
{
    pthread_mutex_lock(q->mutex);
    int full = q->size == q->capacity;
    pthread_mutex_unlock(q->mutex);

    return full;
}

// Método para liberar memoria asignada y eliminar la cola
int queue_destroy(queue *q)
{

    for (node *node = q->head->next; node != q->tail; node = node->next) // Liberamos memoria asignada para cada nodo
    {
        free(node->prev);
    }

    free(q->tail); // Liberamos memoria del último nodo

    // Gestión de concurrencia
    pthread_mutex_destroy(q->mutex);
    pthread_cond_destroy(q->isFull);
    pthread_cond_destroy(q->isEmpty);

    // Liberamos memoria asignada para la cola
    free(q);

    return 0;
}