CC = gcc

SCORE = ssu_score
BLANK = blank
MAIN = main

$(SCORE) : $(SCORE).o $(BLANK).o $(MAIN).o
	$(CC) -o $(SCORE) $(SCORE).o $(BLANK).o $(MAIN).o 

$(SCORE).o : $(SCORE).h $(BLANK).h
	$(CC) -c -o $@ $(SCORE).c

$(BLANK).o : $(BLANK).h
	$(CC) -c -o $@ $(BLANK).c

$(MAIN).o : $(SCORE).h
	$(CC) -c -o $@ $(MAIN).c

clean :
	rm -rf $(SCORE)
	rm -rf *.o
