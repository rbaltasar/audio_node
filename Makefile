audio_node:
		g++ audio_node.cpp -o audio_node -lboost_system -L/lib -lrt -lm -lpthread -lboost_program_options
