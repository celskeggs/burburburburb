interp: interp.c lua-src-5.2.3/liblua.a
	gcc -Ilua-src-5.2.3 interp.c -o interp lua-src-5.2.3/liblua.a -lm
lua-src-5.2.3/liblua.a:
	cd lua-src-5.2.3 && make
clean:
	rm interp
clean-lua:
	cd lua-src-5.2.3 && make clean
