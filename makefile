INCLUDE=-I./speech
LIB=-ljsoncpp -lcurl -lcrypto -lpthread

Friday:Friday.cc
	g++ -o $@ $^ $(INCLUDE) -std=c++11 $(LIB)
.PHONY:clean
clean:
	rm -r Friday

