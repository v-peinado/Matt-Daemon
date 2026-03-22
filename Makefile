# Compiler and flags
DAEMON_NAME = Matt_daemon
CLIENT_NAME = Ben_AFK
CXX         = c++
CXXFLAGS    = -Wall -Wextra -Werror -std=c++20

# Directories
SERVER_SRCDIR    = server/src
SERVER_INCDIR    = server/includes
SERVER_OBJDIR    = server/objs

CLIENT_SRCDIR    = client/src
CLIENT_INCDIR    = client/includes
CLIENT_OBJDIR    = client/objs

# Server sources
SERVER_SRCS = $(SERVER_SRCDIR)/TintinReporter.cpp \
              $(SERVER_SRCDIR)/Daemonize.cpp \
              $(SERVER_SRCDIR)/Server.cpp \
              $(SERVER_SRCDIR)/MattDaemon.cpp \
              $(SERVER_SRCDIR)/main.cpp

# Client sources
CLIENT_SRCS = $(CLIENT_SRCDIR)/Connection.cpp \
              $(CLIENT_SRCDIR)/BenAfk.cpp \
              $(CLIENT_SRCDIR)/ArgParser.cpp \
              $(CLIENT_SRCDIR)/main.cpp

# Object files
SERVER_OBJS = $(SERVER_SRCS:$(SERVER_SRCDIR)/%.cpp=$(SERVER_OBJDIR)/%.o)
CLIENT_OBJS = $(CLIENT_SRCS:$(CLIENT_SRCDIR)/%.cpp=$(CLIENT_OBJDIR)/%.o)

# Colors
GREEN       = \033[0;32m
BLUE        = \033[0;34m
RED         = \033[0;31m
RESET       = \033[0m

# Rules
all: $(DAEMON_NAME)

# Server
$(DAEMON_NAME): $(SERVER_OBJS)
	@echo "$(GREEN)Linking $(DAEMON_NAME)...$(RESET)"
	@$(CXX) $(CXXFLAGS) $(SERVER_OBJS) -o $(DAEMON_NAME)
	@echo "$(GREEN)✓ Server ready$(RESET)"

$(SERVER_OBJDIR)/%.o: $(SERVER_SRCDIR)/%.cpp | $(SERVER_OBJDIR)
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) -I$(SERVER_INCDIR) -c $< -o $@

$(SERVER_OBJDIR):
	@mkdir -p $(SERVER_OBJDIR)

# Client
client: $(CLIENT_NAME)

$(CLIENT_NAME): $(CLIENT_OBJS)
	@echo "$(BLUE)Linking $(CLIENT_NAME)...$(RESET)"
	@$(CXX) $(CXXFLAGS) $(CLIENT_OBJS) -o $(CLIENT_NAME)
	@echo "$(GREEN)✓ Client ready$(RESET)"

$(CLIENT_OBJDIR)/%.o: $(CLIENT_SRCDIR)/%.cpp | $(CLIENT_OBJDIR)
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) -I$(CLIENT_INCDIR) -c $< -o $@

$(CLIENT_OBJDIR):
	@mkdir -p $(CLIENT_OBJDIR)

# Build both
bonus: all client

# Clean
clean:
	@echo "$(RED)Cleaning objects...$(RESET)"
	@rm -rf $(SERVER_OBJDIR) $(CLIENT_OBJDIR)

fclean: clean
	@echo "$(RED)Cleaning executables...$(RESET)"
	@rm -f $(DAEMON_NAME) $(CLIENT_NAME)

re: fclean all

.PHONY: all client bonus clean fclean re