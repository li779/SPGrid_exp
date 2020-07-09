CXX=icc
FLAGS=-pthread
NAME=spgrid_demo
all:
	$(CXX) $(FLAGS) -o $(NAME) -I. \
		Blocked_Copy_Helper.cpp \
		Laplace_Helper.cpp \
		PTHREAD_QUEUE.cpp \
		SPGrid/Core/SPGrid_Utilities.cpp \
		main.cpp
			
			
