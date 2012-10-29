#ifndef CLIENT_H_
#define CLIENT_H_

#include "duckchat.h"

struct channel_list {
	struct channel_list *next;
	char channel[CHANNEL_MAX];
} packed;

int addChannel(struct channel_list **list, const char *channel) {
	struct channel_list *curr= *list;
	
	if (curr != NULL) {
		while (curr->next != NULL) {
			if (strcmp(curr->channel, channel) == 0) {
				return 0;
			}
			curr= curr->next;
		}
	}
	
	curr= (struct channel_list*) malloc(sizeof(struct channel_list));
	if (*list == NULL) {
		*list= curr;
	}
	strncpy(curr->channel, channel, CHANNEL_MAX);
	return 1;
}

int removeChannel(struct channel_list **list, const char *channel) {
	struct channel_list *curr= *list;
	struct channel_list *prev= NULL;

	if (curr != NULL) {
		while(curr->next != NULL) {
			if (strcmp(curr->channel, channel) == 0) {
				if (prev == NULL) {
					*list= curr->next;
				} else {
					prev->next= curr->next;
				}
				free(curr);
				return 1;
			}
		}
		prev= curr;
		curr= curr->next;
	}
	return 0;
}

void clear(struct channel_list **list) {
	struct channel_list *curr= *list;

	while (list != NULL) {
		*list= curr->next;
		free(curr);
		curr= *list;
	}
}

#endif
