carMake:
	gcc ../preprocessor.c -o preprocessor
	./preprocessor car_synch.c car.c
	gcc car.c -lpthread -o car
	./car < input.txt
	rm car.c
	rm car
	rm preprocessor
