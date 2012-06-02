#include "LKlist.h"

position LKAdvanceList(listptr alist)
	/* Move current to the succeeding element. */
{
	if (alist->current) {
		if (alist->icurrent == alist->n)
			alist->icurrent = 1;
		else
			++alist->icurrent;
	}
	alist->current = alist->current->next;
	return((int)alist->current);
}


position LKReverseList(listptr alist)	/* Move current to the preceeding element. */
{
	if (alist->current) {
		if (alist->icurrent == 1)
			alist->icurrent = alist->n;
		else
			--alist->icurrent;
	}
	alist->current = alist->current->prior;
	return((int)alist->current);
}


position LKGotoInList(listptr alist, position i)
	/* Set current to the ith list element. */
{
	position k;

	alist->current = alist->head;
					/* Closer to the tail or head? */
	if ((i - 1) < (alist->n - i)) {
/* Head : traverse forward. */
		for (k = 1; k <= i - 1; k++)
			alist->current = alist->current->next;
	} else {
/* Tail : traverse backward. */
		for (k = alist->n; k >= i; k--)
			alist->current = alist->current->prior;
	}

	alist->icurrent = i;
	return((int)alist->current);
}


void LKGetFromList(listptr alist, position i, void *data)
/* Retrieve the current list element from the list into data. */
{
	if (i <=  alist->n) {
		LKGotoInList(alist, i);
#if 0
	memmove(data, alist->current->data, alist->elSize);
#else
		data = alist->current->data;
#endif

	}
	return;
}


void LKPutInList(listptr alist, position i, void *data)
/* Update the current list element with data */
{
	if (i <= alist->n) {
		LKGotoInList(alist, i);
		memmove(alist->current->data, data, alist->elSize);
	}
	return;
}


position LKSearchList(listptr alist, void *data)
{
	int i;
	int a, b;

	a = data;

	LKGotoInList(alist, 1);
	for (i = 0; i < (alist->n); i++) {
		b = alist->current->data;

		if (b == a)
			return alist->icurrent;

		LKAdvanceList(alist);
	}

	if (alist->icurrent == 1)
		return 0;
}

int LKInsertAfterInList(listptr alist, position i, void *data)
/* Inserts a new list element following the current element. */
{
	nodeptr p;

	if ((alist->head == NULL) || (i <= alist->n)) {
		if ((p = malloc(sizeof(listnode))) == NULL) {
			fprintf(stderr, "list.c: LKInsertAfterInList: malloc() [1] failed\n");
			return(-1);
		} else {
#if 0
			if ((p->data = (void *) malloc(alist->elSize)) == NULL) {
				fprintf(stderr, "list.c: LKInsertAfterInList: malloc() [2] failed\n");
				return(-1);
			} else
#endif
			{
#if 0
				memmove(p->data, data, alist->elSize);
#else
				p->data = data;
#endif
				if (alist->head == NULL) {
/* Empty list. */
					alist->head = p;
					alist->icurrent = 1;
					p->next = p;
					p->prior = p;
				} else {						/* Adjust the linkage */
					LKGotoInList(alist, i);
					alist->current->next->prior = p;
					p->next = alist->current->next;
					p->prior = alist->current;
					alist->current->next = p;
					++alist->icurrent;
				}
				alist->current = p;
				++alist->n;
			}
		}
	}
	return(1);
}

int LKInsertBeforeInList(listptr alist, position i, void *data)
/* Inserts a new list element preceeding the current element. */
{
	int result = 0;

	if ((alist->head == NULL) || (i <= alist->n)) {
/* Is list not empty? */
		if (alist->head != NULL) {
			LKGotoInList(alist, i);
			alist->current = alist->current->prior;
			--alist->icurrent;
			if (alist->icurrent == 0) {				/* check for head */
				alist->icurrent = alist->n;
			}
		}
		result = LKInsertAfterInList(alist, alist->icurrent, data);
	/* used to be i not alist->icurrent */

		if (alist->current->next == alist->head) {
		/* Is insertion at list head? */
			alist->head = alist->current;
			alist->icurrent = 1;
		}
	}
	return(result);
}


void LKDeleteFromList(listptr alist, position i)
/* Delete the current element. */
{
	nodeptr precurrent;

	if ((alist->head != NULL) & (i <= alist->n)) {
		LKGotoInList(alist, i);
		precurrent = alist->current;


/* Relink list to exclude the node to be deleted. */
		alist->current->prior->next = alist->current->next;
		alist->current->next->prior = alist->current->prior;

		/* Is tail node to be deleted? */

		if (precurrent->next == alist->head)
			alist->icurrent = 1;
		alist->current = alist->current->next;
		if (alist->n == 1) {
			alist->head = NULL;
		} else if (alist->head == precurrent) {
			alist->head = alist->current;
		}
		--alist->n;
#if 0
		free(precurrent->data);
#endif
		free(precurrent);
		precurrent = NULL;
	}
	return;
}


position LKCurrent(listptr alist)
	/* Return the position of current. */
{
  return(alist->icurrent);
}


position LKLastInList(listptr alist)
	/* Return the current list size. */
{
	return(alist->n);
}


int LKCreateList(linklist, space)
	/* Initialize package before first use. */
	listptr		*linklist;
	position	   space;
{
	listptr alist;
	if ((alist = (listptr) malloc(sizeof(list))) == NULL) {
		fprintf(stderr, "list.c: LKCreateList: malloc() failed\n");
		return(-1);
	} else {
		alist->head = NULL;
		alist->current = NULL;
		alist->n = 0;
		alist->icurrent = 0;
		alist->elSize = space;
		*linklist = alist;
	}
	return(1);
}

void LKClearList(listptr alist)
	/* Reset the list to empty. */
{
	nodeptr prehead, q;

	prehead = alist->head;
/* Is the list not empty? */
	if (alist->head != NULL) {
		do {
			q = alist->head;
			alist->head = alist->head->next;
			free(q->data);
			free(q);
		} while (alist->head != prehead);
	}

	/* Reinitialize the list. */
	alist->head = NULL;
	alist->current = NULL;
	alist->n = 0;
	alist->icurrent = 0;
	return;
}


int LKEmptyList(listptr alist)
		/* Is the list Empty? */
{
	if (alist->head == NULL) {
		return((int) NULL);
	}
	return 1;
}



void LKZapList(listptr *alist)
	/* Reset the list to empty. */
{

	LKClearList(*alist);
	free(*alist);
	*alist = NULL;
	return;
}
