interp: interp.c cpu.c hardware.c hardware.h sync.h cpu.h lua-src-5.2.3/liblua.a
	gcc -Ilua-src-5.2.3 interp.c cpu.c hardware.c -o interp lua-src-5.2.3/liblua.a -lm -lpthread
lua-src-5.2.3/liblua.a:
	cd lua-src-5.2.3 && make
clean:
	rm interp
clean-lua:
	cd lua-src-5.2.3 && make clean
