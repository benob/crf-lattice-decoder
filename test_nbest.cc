#include <iostream>
#include "nbest.h"

int main(int argc, char** argv) {
    StdNBest nbest(3);
    nbest.insertNmax(1.3, 0);
    nbest.insertNmax(0.3, 1);
    nbest.insertNmax(5.2, 2);
    nbest.insertNmax(2.4, 3);
    nbest.insertNmax(3.7, 4);
    nbest.sortNmax();
    for(int i = 0; i < nbest.size(); i++) {
        std::cout << nbest.get(i) << " " << nbest.getValue(i) << std::endl;
    }
}
