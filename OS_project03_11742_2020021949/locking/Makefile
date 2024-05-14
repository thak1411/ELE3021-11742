

all:
	@echo "Compiling pthread lock linux file..."
	@gcc -std=gnu99 -o run pthread_lock_linux.c -lpthread
	@echo "Compilation done. Run using ./run"
	@./run
.PHONY: all

clean:
	@echo "Cleaning pthread lock linux file..."
	@rm -rf run
	@echo "Done cleaning pthread lock linux file."
.PHONY: clean
