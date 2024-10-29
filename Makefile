EXE = main.exe
SRC = $(wildcard *.cpp)
OBJS = $(SRC:.cpp=.o)

CFLAGS = -std=c++17 -g -I./include -c

run: $(EXE)
	$(EXE)

$(EXE): $(OBJS)
	g++ $(OBJS) -L./lib -lopengl32 -lglfw3dll -lglew32 -lgdi32 -o $(EXE)

%.o: %.cpp
	g++ $(CFLAGS) $< -o $@

clean:
	rm $(OBJS)