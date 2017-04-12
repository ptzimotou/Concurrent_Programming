trainMake:
	gcc ../preprocessor.c -o preprocessor;
	./preprocessor train_synch.c train.c
	gcc train.c -lpthread -o train
	./train < input.txt
	rm train.c
	rm train
	rm preprocessor
