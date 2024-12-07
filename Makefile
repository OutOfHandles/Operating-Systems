CC  = gcc
CFLAGS = -Wall -pthread -fsanitize=thread

all: broker feed
	export MSG_FICH="messages.txt" 
broker: Manager/manager.c Manager/user.c Manager/topic.c utils.c 
	$(CC) -o manager Manager/manager.c Manager/user.c Manager/topic.c utils.c $(CFLAGS)

feed: Feed/feed.c utils.c
	$(CC) -o feed Feed/feed.c utils.c $(CFLAGS)

clean:
	rm feed manager
	rm Pipes/*