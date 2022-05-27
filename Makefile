all: Tron.exe

%.exe:%.c
	@gcc $< -lpthread -o $@

clean:
	@rm *.exe
