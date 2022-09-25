#Compile Game
RogueThread: game.c
	gcc -o game game.c -lgraph -lpthread -lX11