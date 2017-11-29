all:
	gcc MirrorInit.c -o mirrorinit
	gcc  MirrorServer.c -lpthread -o mirrorserver 
	gcc ContentServer.c -o contentserver
	./mirrorinit -n 127.0.0.1 -s 127.0.0.1:9013:/home/teomandi/Desktop/syspro/E1/cdr.h:5 -p 9038