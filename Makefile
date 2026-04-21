all: clean main

1.o: main.cpp
	g++ -c main.cpp

2.o: bloc.cpp
	g++ -c bloc.cpp

3.o: menu.cpp
	g++ -c menu.cpp

main: 1.o 2.o 3.o
	g++ -g -o TetrisLinux main.o bloc.o menu.o -lsfml-graphics -lsfml-window -lsfml-system -lGL -lpthread -lfreetype -ljpeg
	rm -f *.o

clean:
	rm -f *.o *.exe TetrisLinux


# CXX = /home/Lucas/Téléchargements/winlib/mingw64/bin/x86_64-w64-mingw32-g++.exe
# CXXFLAGS = -I/home/Lucas/SFMLTT/include -std=c++17 -Wall -DSFML_STATIC
# LDFLAGS = -L/home/Lucas/SFMLTT/lib -static -lsfml-graphics-s -lsfml-window-s -lsfml-system-s -lopengl32 -lgdi32 \
# -lwinmm -lkernel32 -lfreetype -ljpeg -static-libstdc++ -static-libgcc -lssp -lmingw32 -lmoldname -mwindows

# SOURCES = main.cpp bloc.cpp menu.cpp
# OBJECTS = $(SOURCES:.cpp=.o)
# EXECUTABLE = TetrisWindows.exe

# # Règles
# window: $(EXECUTABLE)

# $(EXECUTABLE): $(OBJECTS)
# 	$(CXX) -o $@ $^ $(LDFLAGS)

# %.o: %.cpp	
# 	$(CXX) $(CXXFLAGS) -c $< -o $@
