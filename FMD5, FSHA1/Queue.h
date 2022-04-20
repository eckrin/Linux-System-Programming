#ifndef QUEUE_H
#define QUEUE_H

#include<stdio.h>
#include<stdlib.h>
#include"FileInfo.h"


typedef struct _node {
	Fileinfo data;
	struct _node* next;
} QNode;

typedef struct _queue {
	QNode* front;
	QNode* rear;
} Queue;

void queueInit(Queue* q);
int isEmptyQueue(Queue* q);

void enQueue(Queue* q, Fileinfo data);
Fileinfo deQueue(Queue* q);
Fileinfo peek(Queue *q);

#endif
