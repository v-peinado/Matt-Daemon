# Compiler and flags
NAME        = Matt_daemon
CXX         = c++
CXXFLAGS    = -Wall -Wextra -Werror
INCLUDES    = -Iincludes

# Directories
SRCDIR      = src
OBJDIR      = objs

# Source files
SRCS        = $(SRCDIR)/TintinReporter.cpp \
              $(SRCDIR)/Daemonize.cpp \
			  $(SRCDIR)/Server.cpp \
			  $(SRCDIR)/MattDaemon.cpp \
			  $(SRCDIR)/main.cpp \

# Object files
OBJS        = $(SRCS:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

# Colors
GREEN       = \033[0;32m
RED         = \033[0;31m
RESET       = \033[0m

# Rules
all: $(NAME)

$(NAME): $(OBJS)
	@echo "$(GREEN)Linking $(NAME)...$(RESET)"
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@echo "$(GREEN)✓ $(NAME) created successfully$(RESET)"

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(OBJDIR):
	@mkdir -p $(OBJDIR)

clean:
	@echo "$(RED)Cleaning objects...$(RESET)"
	@rm -rf $(OBJDIR)

fclean: clean
	@echo "$(RED)Cleaning $(NAME)...$(RESET)"
	@rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re