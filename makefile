all: test_server.cpp server.cpp server.h threadPool.hpp taskQueue.hpp test_client.cpp client.cpp client.h head.h
	g++ -o test_server test_server.cpp server.cpp -lpthread -lmysqlclient -lhiredis
	g++ -o test_client test_client.cpp client.cpp -lpthread 
test_server: test_server.cpp server.cpp server.h threadPool.hpp taskQueue.hpp head.h
	g++ -o test_server test_server.cpp server.cpp -lpthread -lmysqlclient -lhiredis
test_client: test_client.cpp client.cpp client.h head.h
	g++ -o test_client test_client.cpp client.cpp -lpthread 
clean:
	rm test_server
	rm test_client
