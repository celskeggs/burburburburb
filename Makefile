interp: interp.c
	gcc interp.c -o interp -llua
clean:
	rm interp
