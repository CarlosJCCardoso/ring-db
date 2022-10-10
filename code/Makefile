CC = gcc
CFLAGS  = -g -Wall

# To create the executable file ring we need the object files
# ringv10.o nodes.o commands.o udp_sc.o tcp_sc.o enderecos.o:

ring: ring.o comandos.o nodes.o enderecos.o udp_sc.o tcp_sc.o states.o messages.o
	gcc -Wall ring.o comandos.o nodes.o enderecos.o udp_sc.o tcp_sc.o states.o messages.o -o ring

ring.o: ring.c ring.h  comandos.o nodes.o enderecos.o udp_sc.o tcp_sc.o 
	gcc -Wall -c ring.c

comandos.o: comandos.c comandos.h nodes.o
	gcc -Wall -c comandos.c
	
messages.o: messages.c messages.h nodes.o
	gcc -Wall -c messages.c

nodes.o: nodes.c nodes.h enderecos.o
	gcc -Wall -c nodes.c
	
states.o: states.c states.h
	gcc -Wall -c states.c

enderecos.o: enderecos.c enderecos.h
	gcc -Wall -c enderecos.c

udp_sc.o: udp_sc.c udp_sc.h
	gcc -Wall -c udp_sc.c

tcp_sc.o: tcp_sc.c tcp_sc.h
	gcc -Wall -c tcp_sc.c

clean:
	rm *.o ring
