
#pragma once

#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#define DPRT() printf("### %s %d\n", __FUNCTION__, __LINE__);

const char* Format(const char* fmt, ...) {
    static char buffer[4096];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(buffer, fmt, ap);
    va_end(ap);
    buffer[sizeof(buffer) - 1] = '\0';
    return buffer;
}

template <typename T> class Num1D;
template <typename T> class Num2D;

class MemoryManager {
    public:
    std::vector<void*> Pointer;
    std::vector<size_t> Size;
    std::vector<int> InUse;
    int ReleaseCount;
    int ReUseCount;
    MemoryManager() {
        this->ReleaseCount = 0;
        this->ReUseCount = 0;
    }
    ~MemoryManager() {
        for(auto iter = this->Pointer.begin(); iter != this->Pointer.end(); iter++) {
            free(*iter);
        }
        //this->Report();
    }
    
    void Report() {
        std::cout << "### Memory report"  << std::endl;
        for(unsigned int i = 0; i < this->Pointer.size(); i++) {
            std::cout << i << ": " << this->Size[i] << std::endl;
        }
        std::cout << "release count: " << this->ReleaseCount << std::endl;
        std::cout << "reuse count: " << this->ReUseCount << std::endl;
        std::cout << "alloc count: " << this->Pointer.size() << std::endl;
    }
    
    int FindMemory(size_t size) {
        for(auto iter = this->Size.begin(); iter != this->Size.end(); iter++) {
            if(*iter >= size) {
                int index = std::distance(this->Size.begin(), iter);;
                if(this->InUse[index] == 0) {
                    return index;
                }
            }
        }
        return -1;
    }
    
    void Append(void* p, size_t size) {
        this->Pointer.push_back(p);
        this->Size.push_back(size);
        this->InUse.push_back(1);
    }
    
    void* Alloc(size_t size) {
        auto index = this->FindMemory(size);
        if(index >= 0) {
            this->ReUseCount += 1;
            this->InUse[index] = 1;
            return this->Pointer[index];
        } else {
            void* p = malloc(size);
            if(p == NULL) {
                throw Format("error in %s: %d, malloc failed, size=%ld", __FUNCTION__, __LINE__, size);
            }
            this->Append(p, size);
            return p;
        }
    }
    
    /*
    template <typename T>
    Array1DAccessor<T> Alloc1D(int count) {
        T* value = (T*)this->Alloc(sizeof(T) * count);
        return Array1DAccessor<T>(count, value);
    }
    template <typename T>
    Array2DAccessor<T> Alloc2D(int row, int col) {
        T* value = (T*)this->Alloc(sizeof(T) * row * col);
        return Array2DAccessor<T>(row, col, value);
    }
    */
    
    long int FindPointer(void* p) {
        auto iter = std::find(this->Pointer.begin(), this->Pointer.end(), p);
        if(iter == this->Pointer.end()) {
            throw Format("error in %s: %d, specified address cannot be find, address=%p" , __FUNCTION__, __LINE__, p);
        }
        long int index = std::distance(this->Pointer.begin(), iter);
        return index;
    }
    
    void Release(void* p) {
        auto index = this->FindPointer(p);
        this->ReleaseCount += 1;
        this->InUse[index] = 0;
    }
    

    template <typename T>
    void Release(Num1D<T>& a) {
        this->Release(a.Value);
    }
    
    template <typename T>
    void Release(Num2D<T>& a) {
        this->Release(a.Value);
    }
    
    void Free(void* p) {
        auto index = this->FindPointer(p);
        free(this->Pointer[index]);
        this->Pointer.erase(this->Pointer.begin() + index);
        this->Size.erase(this->Size.begin() + index);
        this->InUse.erase(this->InUse.begin() + index);
    }
};

template <typename T>
class ValueWithIndex {
    public:
    int Index;
    T Value;
};

template <typename T>
class SortValueWithIndex {
    public:
    int Count;
    ValueWithIndex<T> *X;
    SortValueWithIndex(int count, T* value) {
        this->Count = count;
        this->X = (ValueWithIndex<T>*)malloc(sizeof(ValueWithIndex<T>) * this->Count);
        for(int i = 0; i < this->Count; i++) {
            this->X[i].Index = i;
            this->X[i].Value = value[i];
        }
    }
    static int sortCallBack(const void* pa, const void* pb) {
        ValueWithIndex<T> a = *((ValueWithIndex<T>*)(pa));
        ValueWithIndex<T> b = *((ValueWithIndex<T>*)(pb));
        T su = a.Value - b.Value;
        if(su == 0) {
            return 0;
        } else if(su < 0) {
            return -1;
        } else {
            return 1;
        }
    }
    void Sort() {
        qsort(this->X, this->Count, sizeof(ValueWithIndex<T>), SortValueWithIndex<T>::sortCallBack);
    }
    ~SortValueWithIndex() {
        free(this->X);
    }
};

template <typename T>
class Num1D {
    public:
    int Count;
    T* Value;
    MemoryManager& mm;
    Num1D(MemoryManager& memoryManager): Count(0), Value(NULL), mm(memoryManager) {}
    Num1D(MemoryManager& memoryManager, int count, T* value): Count(count), Value(value), mm(memoryManager) {}
    
    void ThrowDifferentCount(Num1D a, Num1D b) {
        if(a.Count != b.Count) {
            throw Format("error in %s: %d, different Num1D count %d != %d" , __FUNCTION__, __LINE__, a.Count, b.Count);
        }
    }
    
    Num1D operator=(Num1D r) {
        auto tmp = this->Clone(r);
        this->Count = tmp.Count;
        this->Value = tmp.Value;
        return *this;
    }
    
    T& operator[](int index) {
        return this->Value[index];
    }
    
    Num1D operator+(Num1D b) {
        this->ThrowDifferentCount(*this, b);
        auto answer = this->Clone(*this);
        for(int i = 0; i < this->Count; i += 1) {
            answer[i] += b[i];
        }
        return answer;
    }
    
    Num1D operator-(Num1D b) {
        this->ThrowDifferentCount(*this, b);
        auto answer = this->Clone(*this);
        for(int i = 0; i < this->Count; i += 1) {
            answer[i] -= b[i];
        }
        return answer;
    }
    
    Num1D operator/(Num1D b) {
        this->ThrowDifferentCount(*this, b);
        auto answer = this->Clone(*this);
        for(int i = 0; i < this->Count; i += 1) {
            answer[i] /= b[i];
        }
        return answer;
    }
    
    Num1D operator/(T r) {
        auto answer = this->Clone(*this);
        for(int i = 0; i < this->Count; i += 1) {
            answer[i] /= r;
        }
        return answer;
    }
    
    Num1D Create(int count) {
        Num1D dst(this->mm, count, (T*)this->mm.Alloc(sizeof(T) * count));
        return dst;
    }
    void Release() {
        this->mm.Release(*this);
    }
    
    Num1D Clone(MemoryManager&mm, Num1D src) {
        Num1D<T> n1d(mm);
        return n1d.Clone(src);
    }
    Num1D Clone(Num1D src) {
        auto dst = this->Create(src.Count);
        memcpy(dst.Value, src.Value, sizeof(T) * dst.Count);
        return dst;
    }
    Num1D Clone() {
        return this->Clone(*this);
    }
    
    Num1D Copy(Num1D src) {
        ThrowDifferentCount(*this, src);
        memcpy(this->Value, src.Value, sizeof(T) * this->Count);
        return *this;
    }
    Num1D Empty() {
        Num1D dst(this->mm);
        return dst;
    }
    Num1D Full(int count, T value) {
        auto dst = this->Create(count);
        for(int i = 0; i < dst.Count; i += 1) {
            dst[i] = value;
        }
        return dst;
    }
    Num1D Zeros(int count) {
        return this->Full(count, 0);
    }
    Num1D Arange(int start, int end, int step = 1) {
        int count = end - start;
        auto dst = this->Create(count);
        for(int i = 0; i < count; i += 1) {
            dst[i] = start + i * step;
        }
        return dst;
    }
    
    Num1D Random(int count, std::int32_t min, std::int32_t max) {
        auto dst = this->Create(count);
        std::mt19937 mt;
        //mt.seed(0);
        for(int i = 0; i < dst.Count; i += 1) {
            dst[i] = (((double)mt() / (double)std::mt19937::max()) * ((double)max - (double)min) ) + (double)min;
        }
        return dst;
    }
    
    Num1D<int> ArgSort(Num1D src) {
        SortValueWithIndex<T> sorter(src.Count, src.Value);
        sorter.Sort();
        Num1D<int> n1d(this->mm);
        auto dst = n1d.Create(src.Count);
        for(int i = 0; i < src.Count; i += 1) {
            dst[i] = sorter.X[i].Index;
        }
        return dst;
    }
    
    Num1D Sort(Num1D src) {
        auto indexes = this->ArgSort(src);
        auto dst = this->Create(src.Count);
        for(int i = 0; i < src.Count; i += 1) {
            dst[i] = src[indexes[i]];
        }
        indexes.Release();
        return dst;
    }
    
    Num1D Shuffle(Num1D src) {
        auto rnd = this->Random(src.Count, 0, std::numeric_limits<int32_t>::max());
        auto indexes = this->ArgSort(rnd);
        auto dst = this->Create(src.Count);
        for(int i = 0; i < src.Count; i += 1) {
            dst[i] = src[indexes[i]];
        }
        rnd.Release();
        indexes.Release();
        return dst;
    }
    
    
    
    ////////////////////////////////////////
    // Operation
    ////////////////////////////////////////
    Num1D Slice(Num1D src, int start, int end) {
        int count = end - start;
        auto dst = this->Create(count);
        for(int i = 0; i < count; i++) {
            dst[i] = src[start + i];
        }
        return dst;
    }
    
    ////////////////////////////////////////
    // 
    ////////////////////////////////////////
    int ArgMin(Num1D x) {
        auto indexes = this->ArgSort(x);
        int idx = indexes[0];
        indexes.Release();
        return idx;
    }
    Num1D<int> WhereEq(Num1D x, T n) {
        int count = 0;
        for(int i = 0; i < x.Count; i += 1) {
            if(x[i] == n) {
                count += 1;
            }
        }
        Num1D<int> n1d(this->mm);
        auto idxs = n1d.Create(count);
        int idx = 0;
        for(int i = 0; i < x.Count; i += 1) {
            if(x[i] == n) {
                idxs[idx] = i;
                idx += 1;
            }
        }
        return idxs;
    }
    
    ////////////////////////////////////////
    // Calculation
    ////////////////////////////////////////
    Num1D Subtract(Num1D a, Num1D b) {
        return a - b;
    }
    
    Num1D Power(double b) {
        auto dst = this->Clone(*this);
        for(int i = 0; i < dst.Count; i += 1) {
            dst[i] = (T)powf((double)dst[i], b);
        }
        return dst;
    }
    
    Num1D Sqrt() {
        auto dst = this->Clone(*this);
        for(int i = 0; i < dst.Count; i += 1) {
            dst[i] = (T)sqrt((double)dst[i]);
        }
        return dst;
    }
    
    T Total(Num1D x) {
        T total = 0;
        for(int i = 0; i < x.Count; i += 1) {
            total += x[i];
        }
        return total;
    }
    
    T Mean() {
        T total = 0;
        for(int i = 0; i < this->Count; i += 1) {
            total += (*this)[i];
        }
        return total / this->Count;
    }
    
    T CalcDistance(Num1D a, Num1D b) {
        auto subtract = a - b;
        auto power = subtract.Power(2);
        T total = this->Total(power);
        subtract.Release();
        power.Release();
        return sqrt(total);
    }
};

template <typename T>
void Dump1D(Num1D<T> n1d) {
    for(int i = 0; i < n1d.Count; i += 1) {
        std::cout << n1d[i];
        if(i < (n1d.Count - 1)) {
            std::cout << ", ";
        }
    }
    std::cout << std::endl;
}

template <typename T>
class SpotNum1D: public Num1D<T> {
    public:
    MemoryManager mm;
    SpotNum1D(): Num1D<T>(mm) {}
    SpotNum1D(int count, T* value): Num1D<T>(mm, count, value) {}
};


template <typename T>
class Num2D {
    public:
    int Row;
    int Col;
    T* Value;
    MemoryManager& mm;
    Num2D(MemoryManager& memoryManager): Row(0), Col(0), Value(NULL), mm(memoryManager) {}
    Num2D(MemoryManager& memoryManager, int row, int col, T* value): Row(row), Col(col), Value(value), mm(memoryManager) {}
    //Num2D(Num2D x): Row(x.Row), Col(x.Col), Value(x.Value), mm(x.mm) {}
    
    void ThrowDifferentRowCol(Num2D a, Num2D b) {
        if(a.Row != b.Row || a.Col != b.Col) {
            throw Format("error in %s: %d, different Num2D (%d, %d) != (%d, %d)" , __FUNCTION__, __LINE__, a.Row, a.Col, b.Row, b.Col);
        }
    }
    
    void ThrowDifferentRow(Num2D a, Num1D<T> b) {
        if(a.Row != b.Count) {
            throw Format("error in %s: %d, different Num2D::Row %d != %d", __FUNCTION__, __LINE__, a.Row, b.Count);
        }
    }
    
    void ThrowDifferentCol(Num2D a, Num1D<T> b) {
        if(a.Col != b.Count) {
            throw Format("error in %s: %d, different Num2D::Col %d != %d", __FUNCTION__, __LINE__, a.Col, b.Count);
        }
    }
    
    Num2D operator=(Num2D r) {
        auto tmp = this->Clone(r);
        this->Row = tmp.Row;
        this->Col = tmp.Col;
        this->Value = tmp.Value;
        return *this;
    }
    
    T* operator[](int index) {
        return &this->Value[index * this->Col];
    }
    
    Num2D operator+(Num2D r) {
        auto answer = this->Clone(*this);
        for(int m = 0; m < this->Row; m += 1) {
            for(int n = 0; n < this->Col; n += 1) {
                answer[m][n] += r[m][n];
            }
        }
        return answer;
    }
    
    Num2D operator-(Num2D r) {
        auto answer = this->Clone(*this);
        for(int m = 0; m < this->Row; m += 1) {
            for(int n = 0; n < this->Col; n += 1) {
                answer[m][n] -= r[m][n];
            }
        }
        return answer;
    }
    
    Num2D operator/(Num2D r) {
        auto answer = this->Clone(*this);
        for(int m = 0; m < this->Row; m += 1) {
            for(int n = 0; n < this->Col; n += 1) {
                answer[m][n] /= r[m][n];
            }
        }
        return answer;
    }
    
    Num2D operator/(T r) {
        auto answer = this->Clone(*this);
        for(int m = 0; m < this->Row; m += 1) {
            for(int n = 0; n < this->Col; n += 1) {
                answer[m][n] /= r;
            }
        }
        return answer;
    }
    
    Num2D Create(int row, int col) {
        Num2D dst(this->mm, row, col, (T*)this->mm.Alloc(sizeof(T) * row * col));
        return dst;
    }
    void Release() {
        this->mm.Release(this->Value);
    }

    Num2D Clone(MemoryManager& mm, Num2D src) {
        Num2D<T> n2d(mm);
        return n2d.Clone(src);
    }
    Num2D Clone(Num2D src) {
        auto dst = this->Create(src.Row, src.Col);
        memcpy(dst.Value, src.Value, sizeof(T) * dst.Row * dst.Col);
        return dst;
    }
    Num2D Clone() {
        return this->Clone(*this);
    }
    
    Num2D Copy(Num2D src) {
        ThrowDifferentRowCol(*this, src);
        memcpy(this->Value, src.Value, sizeof(T) * this->Row * this->Col);
        return *this;
    }
    Num2D Empty() {
        Num2D dst(this->mm);
        return dst;
    }
    Num2D Transpose() {
        auto dst = Create(this->Col, this->Row);
        for(int m = 0; m < dst.Row; m += 1) {
            for(int n = 0; n < dst.Col; n += 1) {
                dst[m][n] += (*this)[n][m];
            }
        }
        return dst;
    }
    Num2D Transpose(Num2D src) {
        auto dst = Create(src.Col, src.Row);
        for(int m = 0; m < dst.Row; m += 1) {
            for(int n = 0; n < dst.Col; n += 1) {
                dst[m][n] += src[n][m];
            }
        }
        return dst;
    }
    
    
    ////////////////////////////////////////
    // reference
    ////////////////////////////////////////
    Num1D<T> Ref(int index) {
        Num1D<T> ref(this->mm, this->Col, (*this)[index]);
        return ref;
    }
    Num1D<T> Ref(Num2D src, int index) {
        Num1D<T> ref(this->mm, src.Col, src[index]);
        return ref;
    }
    Num1D<T> Val(Num2D src, int index) {
        Num1D<T> n1d(this->mm);
        auto dst = n1d.Create(src.Col);
        memcpy(dst.Value, src[index], sizeof(T) * src.Col);
        return dst;
    }
    Num1D<T> ValT(Num2D src, int index) {
        Num1D<T> n1d(this->mm);
        auto dst = n1d.Create(src.Row);
        for(int i = 0; i < dst.Count; i++) {
            dst[i] = src[i][index];
        }
        return dst;
    }

    ////////////////////////////////////////
    // index operation
    ////////////////////////////////////////
    Num2D Indexing(Num2D src, Num1D<int> indexes) {
        auto dst = Create(indexes.Count, src.Col);
        for(int i = 0; i < indexes.Count; i+= 1) {
            memcpy(dst[i], src[indexes[i]], sizeof(T) * src.Col);
        }
        return dst;
    }
    Num2D IndexingT(Num2D src, Num1D<int> indexes) {
        auto srcT = Transpose(src);
        auto indexed = Indexing(srcT, indexes);
        auto dst = Transpose(indexed);
        srcT.Release();
        indexed.Release();
        return dst;
    }
    
    
    ////////////////////////////////////////
    // Calculation
    ////////////////////////////////////////
    
    
    
    // Subtract
    Num2D<T> Subtract(Num1D<T> r) {
        ThrowDifferentCol(*this, r);
        auto answer = this->Create(this->Row, this->Col);
        for(int m = 0; m < this->Row; m += 1) {
            for(int n = 0; n < this->Col; n += 1) {
                answer[m][n] = (*this)[m][n] - r[n];
            }
        }
        return answer;
    }
    
    Num1D<T> SubtractT(Num1D<T> r) {
        ThrowDifferentRow(*this, r);
        auto answer = this->Create(this->Row, this->Col);
        for(int m = 0; m < this->Row; m += 1) {
            for(int n = 0; n < this->Col; n += 1) {
                answer[m][n] = (*this)[m][n] - r[m];
            }
        }
        return answer;
    }
    
    Num1D<T> SubtractX(T n) {
        auto answer = this->Create(this->Row, this->Col);
        for(int m = 0; m < this->Row; m += 1) {
            for(int n = 0; n < this->Col; n += 1) {
                answer[m][n] = (*this)[m][n] - n;
            }
        }
        return answer;
    }
    
    
    
    // Division
     Num2D<T> Division(Num1D<T> r, double safeValue = 0) {
        ThrowDifferentCol(*this, r);
        auto answer = this->Create(this->Row, this->Col);
        for(int m = 0; m < this->Row; m += 1) {
            for(int n = 0; n < this->Col; n += 1) {
                answer[m][n] = (*this)[m][n] / (r[n] + safeValue);
            }
        }
        return answer;
    }
    
    
    
    
    Num2D Power(double b) {
        auto dst = this->Clone();
        for(int m = 0; m < this->Row; m += 1) {
            for(int n = 0; n < this->Col; n += 1) {
                dst[m][n] += (T)powf((double)dst[m][n], b);
            }
        }
        return dst;
    }
    
    
    
    // Total
    Num1D<T> Total() {
        Num1D<T> n1d(this->mm);
        auto answer = n1d.Zeros(this->Col);
        for(int m = 0; m < this->Row; m += 1) {
            for(int n = 0; n < this->Col; n += 1) {
                answer[n] += (*this)[m][n];
            }
        }
        return answer;
    }
    Num1D<T> TotalT() {
        Num1D<T> n1d(this->mm);
        auto answer = n1d.Zeros(this->Row);
        for(int m = 0; m < this->Row; m += 1) {
            for(int n = 0; n < this->Col; n += 1) {
                answer[m] += (*this)[m][n];
            }
        }
        return answer;
    }
    T TotalX() {
        T answer = 0;
        for(int m = 0; m < this->Row; m += 1) {
            for(int n = 0; n < this->Col; n += 1) {
                answer += (*this)[m][n];
            }
        }
        return answer;
    }
    
    
    
    // Mean
    Num1D<T> Mean() {
        Num1D<T> n1d(this->mm);
        auto total = this->Total();
        auto answer = total / this->Row;
        total.Release();
        return answer;
    }
    Num1D<T> MeanT() {
        Num1D<T> n1d(this->mm);
        auto total = this->TotalT();
        auto answer = total / this->Col;
        
        total.Release();
        
        return answer;
    }
    
    // Variance
    Num1D<T> Variance(double ddof = 1) {
        Num1D<T> n1d(this->mm);
        auto mean = this->Mean();
        auto subtract = this->Subtract(mean);
        auto power = subtract.Power(2);
        auto total = power.Total();
        auto answer = total / (this->Row - ddof);
        
        mean.Release();
        subtract.Release();
        power.Release();
        total.Release();
        
        return answer;
    }
    Num1D<T> VarianceT(double ddof = 1) {
        Num1D<T> n1d(this->mm);
        auto mean = this->MeanT();
        auto subtract = this->SubtractT(mean);
        auto power = subtract.Power(2);
        auto total = power.Total();
        auto answer = total / (this->Col - ddof);
        
        mean.Release();
        subtract.Release();
        power.Release();
        total.Release();
        
        return answer;
    }
    
    
    
    // Standard Deviation
    Num1D<T> StdDev(double ddof = 1) {
        auto variance = this->Variance(ddof);
        auto answer = variance.Sqrt();
        
        variance.Release();
        
        return answer;
    }
    Num1D<T> StdDevT(double ddof = 1) {
        auto variance = this->VarianceT(ddof);
        auto answer = variance.Sqrt();
        
        variance.Release();
        
        return answer;
    }
};

template <typename T>
void Dump2D(Num2D<T> x) {
    for(int m = 0; m < x.Row; m += 1) {
        for(int n = 0; n < x.Col; n += 1) {
            std::cout << x[m][n];
            if(n < (x.Col - 1)) {
                std::cout << ", ";
            }
        }
        std::cout << std::endl;
    }
}

template <typename T>
class SpotNum2D: public Num2D<T> {
    public:
    MemoryManager mm;
    SpotNum2D(): Num2D<T>(mm) {}
    SpotNum2D(int row, int col, T* value): Num2D<T>(mm, row, col, value) {}
};

std::vector<std::string> string_token(std::string x, std::vector<char> tokens) {
    char* buffer = (char*)malloc(sizeof(char) * strlen(x.c_str()) + 1);
    char* p = buffer;
    strcpy(p, x.c_str());
    std::vector<char*> positions;
    while(*p) {
        positions.push_back(p);
        while(1) {
            bool isBreak = false;
            for(auto tok = tokens.begin(); tok != tokens.end(); tok++) {
                if(*p != *tok && *p != '\0') {
                } else {
                    isBreak = true;
                    break;
                }
                p++;
            }
            if(isBreak) {
                break;
            }
        }
        
        while(1) {
            bool isBreak = false;
            for(auto tok = tokens.begin(); tok != tokens.end(); tok++) {
                if(*p == *tok) {
                    *p = '\0';
                } else {
                    isBreak = true;
                    break;
                }
                p++;
            }
            if(isBreak) {
                break;
            }
        }
    }
    
    std::vector<std::string> y;
    for(auto pos = positions.begin(); pos != positions.end(); pos++) {
        y.push_back(std::string(*pos));
    }
    
    free(buffer);
    
    return y;
}

