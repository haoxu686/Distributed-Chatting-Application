all: dchat

clean: 
	rm *.o dchat

dchat: BullyTimer.o ChatTimer.o ConnectTimer.o ElectionTimer.o HelloTimer.o LeaderTimer.o LoginTimer.o Main.o Member.o Message.o Model.o MulticastTimer.o Runnable.o State.o Timer.o
	g++ -L"./lib" ./lib/randombytes.o BullyTimer.o ChatTimer.o ConnectTimer.o ElectionTimer.o HelloTimer.o LeaderTimer.o LoginTimer.o Main.o Member.o Message.o Model.o MulticastTimer.o Runnable.o State.o Timer.o -static -lnacl -dynamic -lpthread -o dchat

BullyTimer.o: BullyTimer.cc
	g++ -O3 -c -I "./include" BullyTimer.cc -o BullyTimer.o

ChatTimer.o: ChatTimer.cc
	g++ -O3 -c -I "./include" ChatTimer.cc -o ChatTimer.o

ConnectTimer.o: ConnectTimer.cc
	g++ -O3 -c -I "./include" ConnectTimer.cc -o ConnectTimer.o

ElectionTimer.o: ElectionTimer.cc
	g++ -O3 -c -I "./include" ElectionTimer.cc -o ElectionTimer.o

HelloTimer.o: HelloTimer.cc
	g++ -O3 -c -I "./include" HelloTimer.cc -o HelloTimer.o

LeaderTimer.o: LeaderTimer.cc
	g++ -O3 -c -I "./include" LeaderTimer.cc -o LeaderTimer.o

LoginTimer.o: LoginTimer.cc
	g++ -O3 -c -I "./include" LoginTimer.cc -o LoginTimer.o

Main.o: Main.cc
	g++ -O3 -c -I"./include" Main.cc -o Main.o

Member.o: Member.cc
	g++ -O3 -c -I "./include" Member.cc -o Member.o

Message.o: Message.cc
	g++ -O3 -c -I "./include" Message.cc -o Message.o

Model.o: Model.cc
	g++ -O3 -c -I"./include" Model.cc -o Model.o

MulticastTimer.o: MulticastTimer.cc
	g++ -O3 -c -I "./include" MulticastTimer.cc -o MulticastTimer.o

Runnable.o: Runnable.cc
	g++ -O3 -c -I "./include" Runnable.cc -o Runnable.o

State.o: State.cc
	g++ -O3 -c -I "./include" State.cc -o State.o

Timer.o: Timer.cc
	g++ -O3 -c -I "./include" Timer.cc -o Timer.o
