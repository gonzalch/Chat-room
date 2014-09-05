all: chatroom 

chatroom: server.o reader_process.o writer_process.o command_process.o

server.o: server.c
	gcc server.c -o server.o -w -lpthread
	
reader_process.o: reader_process.c
	gcc reader_process.c -o reader_process.o -w
	
command_process.o: command_process.c
	gcc command_process.c -o command_process.o -w
	
writer_process.o: writer_process.c
	gcc writer_process.c -o writer_process.o -w

clean:
	rm -rf *.o chatroom
