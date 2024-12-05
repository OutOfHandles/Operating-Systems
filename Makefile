CC  = gcc
CFLAGS = -Wall -pthread -fsanitize=thread

manager: Manager/manager.c Manager/user.c Manager/topic.c utils.c 
	$(CC) -o manager Manager/manager.c Manager/user.c Manager/topic.c utils.c $(CFLAGS)

feed: feed.c utils.c
	$(CC) -o feed feed.c utils.c $(CFLAGS)

clean:
	rm Pipes/*