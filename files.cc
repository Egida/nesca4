/*
 * NESCA4
 * by oldteam & lomaster
 * license GPL-3.0
 * - Сделано от души 2023.
*/

#include "include/files.h"

int 
get_count_lines(const char* path){
    std::ifstream file(path);
    int count = 0;
    std::string line;
    while (std::getline(file, line)){++count;}
    return count;
}

bool 
check_file(const char* path){
    std::ifstream file(path);
    return file.good();
}

std::vector<std::string>
write_file(const std::string& filename){
    std::string line;
    std::ifstream file(filename);
    std::vector<std::string> lines;
    while (std::getline(file, line))
    {lines.push_back(line);}
    return lines;
}

int 
write_line(std::string path, std::string line){
    std::ofstream outfile;
    outfile.open(path, std::ios_base::app);
    if (!outfile.is_open()){return -1;}
    outfile << line;
    if (outfile.fail()){return -2;}
    outfile.close();
    if (outfile.fail()){return -3;}
    return 0;
}
