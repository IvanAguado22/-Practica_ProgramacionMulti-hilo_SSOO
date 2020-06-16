#include <pthread.h>
#ifndef HEADER_FILE
#define HEADER_FILE

// Estructura de las máquinas con sus atributos
typedef struct element
{
  int type; // Tipo de máquina
  int time; // Tiempo utilizado
} element;

// Estructura de los nodos de la cola
typedef struct node
{
  struct element *elem;
  struct node *prev;
  struct node *next;
} node;

// Estructura de la cola
typedef struct queue
{
  struct node *head;
  struct node *tail;
  int capacity;
  int size;
  pthread_mutex_t *mutex;
  pthread_cond_t *isFull;
  pthread_cond_t *isEmpty;
} queue;

// Métodos de la cola
queue *queue_init(int size);
int queue_put(queue *q, element *elem);
element *queue_get(queue *q);
int queue_empty(queue *q);
int queue_full(queue *q);
int queue_destroy(queue *q);

#endif
