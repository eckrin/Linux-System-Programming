#include "Queue.h"

void queueInit(Queue* q) {
	q->front = NULL;
	q->rear = NULL;
}

int isEmptyQueue(Queue* q) {
	if(q->front == NULL)
		return 1;
	else
		return 0;
}

void enQueue(Queue* q, Fileinfo data) {
	QNode* newNode = (QNode*)malloc(sizeof(QNode));
	newNode->next = NULL;
	newNode->data = data;

	if(isEmptyQueue(q)) {
		q->front = newNode;
		q->rear = newNode;
	}
	else {
		q->rear->next = newNode;
		q->rear = newNode;
	}
}

Fileinfo deQueue(Queue* q) {
	QNode* delNode;
	Fileinfo retData;

	if(isEmptyQueue(q)) {
		printf("Queue Memory Error!");
		exit(-1);
	}

	delNode = q->front;
	retData = delNode->data;
	q->front = q->front->next;

	free(delNode);
	return retData;
}

Fileinfo peek(Queue* q) {
	if(isEmptyQueue(q)) {
		printf("Queue Memory Error!");
		exit(-1);
	}

	return q->front->data;
}
