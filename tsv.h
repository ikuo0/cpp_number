
#include "numxd.h"

namespace TSV {
    using Buffer = std::vector<std::vector<std::string>>;
    
    Buffer Read(const char* fileName, char delimiter='\t') {
        Buffer rows;
        
        std::vector<char> cellTokens;
        cellTokens.push_back('\t');
        
        std::ifstream ifs(fileName);
        if(ifs.fail()) {
            throw Format("error in %s: %d, %s open failed.", __FUNCTION__, __LINE__, fileName);
        }
        std::string line;
        std::vector<std::string> row;
        while(std::getline(ifs, line)) {
            std::istringstream stream(line);
            std::string col;
            while(std::getline(stream, col, delimiter)) {
                row.push_back(col);
            }
            
            if(row.size() == 0) {
                continue;
            }
            
            rows.push_back(row);
            row.clear();
        }
        
        // check
        long unsigned int columns = 0;
        for(auto m = rows.begin(); m != rows.end(); m++) {
            if(columns == 0) {
                columns = m->size();
            } else if(columns != m->size()) {
                throw Format("error in %s: %d, different column size %d != %d", __FUNCTION__, __LINE__, columns, m->size());
            }
        }
        
        return rows;
    }
    
    template <typename T>
    int Write(const char* fileName, Num2D<T> x) {
        std::ofstream ofs(fileName, std::ios::binary);
        if(ofs.fail()) {
            DPRT();
            return -1;
        }
        
        for(int m = 0; m < x.Row; m += 1) {
            for(int n= 0; n < x.Col; n += 1) {
                //ofs << x[m][n];
                ofs << std::scientific << std::setprecision(15) << x[m][n];
                if(n < (x.Col - 1)) {
                    ofs << "\t";
                }
            }
            ofs << "\n";
        }
        return 0;
    }
    
    template <typename T>
    int Write(const char* fileName, Num1D<T> x) {
        std::ofstream ofs(fileName, std::ios::binary);
        if(ofs.fail()) {
            return -1;
        }
        
        for(int i = 0; i < x.Count; i += 1) {
            ofs << std::scientific << std::setprecision(15) << x[i];
            if(i < (x.Count - 1)) {
                ofs << "\n";
            }
        }
        ofs << "\n";
        return 0;
    }
    
    Num2D<double> ToDouble(MemoryManager& mm, Buffer buffer) {
        Num2D<double> n2d(mm);
        const int Row = buffer.size();
        const int Col = buffer[0].size();
        auto dst = n2d.Create(Row, Col);
        for(auto row = buffer.begin(); row != buffer.end(); row++) {
            auto m = std::distance(buffer.begin(), row);
            for(auto col = row->begin(); col != row->end(); col++) {
                auto n = std::distance(row->begin(), col);
                dst[m][n] = std::stod(*col);
            }
        }
        return dst;
    }
};
