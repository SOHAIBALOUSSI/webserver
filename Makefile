CXX = c++
CXXFLAGS = -Wall -Wextra -Werror #-std=c++98

NAME = server

SRCS = main.cpp HttpRequest.cpp Server.cpp Common.cpp Socket.cpp Config.cpp Route.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJS)

fclean: clean
	rm -rf $(NAME)

re: fclean all

.PHONY: all clean debug
