bf:
	@echo " Compile bf_main ...";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./programs/bf_main.c ./modules/record.c -lbf -o ./build/bf_main -O2;
hp:
	@echo " Compile hp_main ...";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./programs/hp_main.c ./modules/record.c ./modules/hp_file.c -lbf -o ./build/hp_main -O2
ht:
	@echo " Compile hp_main ...";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./programs/ht_main.c ./modules/record.c ./modules/ht_table.c -lbf -o ./build/ht_main -O2
sht:
	@echo " Compile sht_main ...";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./programs/sht_main.c ./modules/record.c ./modules/sht_table.c ./modules/ht_table.c -lbf -o ./build/sht_main -O2
stat:
	@echo " Compile HashStatistics_main ...";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./programs/HashStatistics_main.c ./modules/record.c ./modules/HashStatistics.c ./modules/ht_table.c ./modules/sht_table.c -lbf -o ./build/stat_main -O2