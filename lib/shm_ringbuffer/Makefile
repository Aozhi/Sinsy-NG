all: test_ringbuffer_read.cpp test_ringbuffer_write.cpp ringbuffer.cpp ringbuffer.h
	g++ test_ringbuffer_read.cpp ringbuffer.cpp -o rb_read
	g++ test_ringbuffer_write.cpp ringbuffer.cpp -o rb_write
	
test: all
	./rb_write
	./rb_read
