CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11 -pedantic
APPNAME = init format import export ls copy move rm mkdir
OBJECTS = $(builddir)/File.o $(builddir)/Directory.o $(builddir)/FileSystem.o $(builddir)/Path.o $(builddir)/utilities.o
srcdir = src
builddir = obj

all: $(builddir) $(APPNAME)

$(builddir):
	mkdir -p $(builddir)

####################

$(builddir)/Directory.o: src/Directory.cpp src/Directory.hpp src/File.hpp src/Path.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(builddir)/File.o: src/File.cpp src/File.hpp src/utilities.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(builddir)/FileSystem.o: src/FileSystem.cpp src/FileSystem.hpp src/utilities.hpp src/Directory.hpp src/File.hpp src/Path.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(builddir)/Path.o: src/Path.cpp src/Path.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(builddir)/utilities.o: src/utilities.cpp src/utilities.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

####################

$(builddir)/%.o: $(srcdir)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%: $(builddir)/%.o $(OBJECTS)
	$(CXX) $(CXXFLAGS) $< $(OBJECTS) -o $@

clean:
	rm -rf $(builddir)
	rm -rf $(APPNAME)
	
run:
	@./tests/setup.sh
	@./tests/run.sh import
	@./tests/run.sh export
	#@./tests/run.sh mkdir
	#@./tests/run.sh move
	@./tests/run.sh copy
	#@./tests/run.sh rm

.PHONY: clean