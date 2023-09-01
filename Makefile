LIB := $(shell $(CXX) -print-file-name=libstdc++.so)
STDLIB := $(shell realpath -e "$(LIB)")
STDDIR := $(shell dirname "$(STDLIB)")

CXXOPT := -std=c++17 -O3 -Wall -Wextra -pedantic-errors

clocxx.hpp: mkclocxx mkclocxx.cpp base.hpp seen_clockid_t
	@./mkclocxx && echo "clocxx.hpp created ok"

mkclocxx: mkclocxx.cpp clock_test_driver.cpp Makefile
	$(CXX) $(CXXOPT) -o $@ $< -Wl,-rpath $(STDDIR)

clock_test_driver: clock_test_driver.cpp
	$(CXX) $(CXXOPT) -DCLOCK_TO_TEST=$(CLOCK_TO_TEST) -o $@ $<

clean:
	rm -f clocxx.hpp mkclocxx clock_test_driver
