test : spp_porting.o spp_test.o spp_multiTimer.o
	gcc -o test spp_porting.o spp_test.o spp_multiTimer.o  -lpthread
spp_test.o : spp_test.c spp_def.h spp_global.h spp_porting.h spp_multiTimer.h 
	gcc -c -std=gnu99 spp_test.c -lpthread
spp_porting.o : spp_porting.c spp_def.h spp_global.h spp_porting.h spp_multiTimer.h
	gcc -c -std=gnu99 spp_porting.c -lpthread
spp_multiTimer.o : spp_multiTimer.c spp_def.h spp_global.h  spp_porting.h spp_multiTimer.h
	gcc -c -std=gnu99 spp_multiTimer.c -lpthread
