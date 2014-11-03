interp: interp.c
	gcc -Ilua-src-5.2.3 interp.c -o interp lua-src-5.2.3/liblua.a -lm
clean:
	rm interp
