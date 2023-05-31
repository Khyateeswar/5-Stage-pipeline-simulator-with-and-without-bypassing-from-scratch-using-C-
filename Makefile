


	

compile: 5stage.cpp 5stage_bypass.cpp
	g++ -std=c++11 5stage.cpp -o a
	g++ -std=c++11 5stage_bypass.cpp -o b

run_5stage :
	./a input.asm
run_5stage_bypass:
	./b input.asm

run_79stage:
	./a input.asm
run_79stage_bypass:
	./b input.asm

clean:
	rm a
	rm b
