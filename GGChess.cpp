// UnicodeChess.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include<stdio.h>
#include <fcntl.h>
#include <io.h>

#include "BasicTypes.h"
#include "Board.h"
#include "IO.h"
#include "Fen.h"
#include "MoveGenerator.h"
#include "ThreadPool.h"
#include "TransposTable.h"

#include "InputHandler.h"

using namespace GGChess;

int main()
{
    GGChess::UCIMain();
}
