.PHONY: all lib src
all: lib src

lib:
	@echo "[LIB]"
	@make -C lib all
	
src:
	@echo "[SRC]"
	@make -C src all

.PHONY: clean
clean:
	@make -C lib clean
	@make -C src clean
