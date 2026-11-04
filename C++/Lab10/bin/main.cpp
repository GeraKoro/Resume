#include "../lib/include/database.h"
#include "parser.h"

int main(int argc, char* argv[]) {
    Database db;
    ApplyCommandLine(db, argc, argv);
    ReadLines(db);
    return 0;
}
