// matrix_mul.cpp
// Compile: g++ -O2 matrix_mul.cpp -o matrix_mul
#include <bits/stdc++.h>
using namespace std;
using Matrix = vector<vector<double>>;

Matrix naive_mul(const Matrix &A, const Matrix &B) {
    int n = A.size();
    Matrix C(n, vector<double>(n, 0.0));
    for (int i=0;i<n;++i) {
        for (int k=0;k<n;++k) {
            double aik = A[i][k];
            for (int j=0;j<n;++j) C[i][j] += aik * B[k][j];
        }
    }
    return C;
}

Matrix add(const Matrix &A, const Matrix &B) {
    int n = A.size();
    Matrix C(n, vector<double>(n));
    for (int i=0;i<n;++i) for (int j=0;j<n;++j) C[i][j]=A[i][j]+B[i][j];
    return C;
}
Matrix subm(const Matrix &A, const Matrix &B) {
    int n = A.size();
    Matrix C(n, vector<double>(n));
    for (int i=0;i<n;++i) for (int j=0;j<n;++j) C[i][j]=A[i][j]-B[i][j];
    return C;
}

void split(const Matrix &A, Matrix &A11, Matrix &A12, Matrix &A21, Matrix &A22) {
    int n = A.size(), m = n/2;
    A11.assign(m, vector<double>(m)); A12.assign(m, vector<double>(m));
    A21.assign(m, vector<double>(m)); A22.assign(m, vector<double>(m));
    for (int i=0;i<m;++i) for (int j=0;j<m;++j) {
        A11[i][j]=A[i][j];
        A12[i][j]=A[i][j+m];
        A21[i][j]=A[i+m][j];
        A22[i][j]=A[i+m][j+m];
    }
}

Matrix join(const Matrix &C11, const Matrix &C12, const Matrix &C21, const Matrix &C22) {
    int m = C11.size(), n = m*2;
    Matrix C(n, vector<double>(n));
    for (int i=0;i<m;++i) {
        for (int j=0;j<m;++j) {
            C[i][j]=C11[i][j];
            C[i][j+m]=C12[i][j];
            C[i+m][j]=C21[i][j];
            C[i+m][j+m]=C22[i][j];
        }
    }
    return C;
}

Matrix pad_to_pow2(const Matrix &A) {
    int n = A.size();
    int p = 1; while (p<n) p<<=1;
    if (p==n) return A;
    Matrix B(p, vector<double>(p,0.0));
    for (int i=0;i<n;++i) for (int j=0;j<n;++j) B[i][j]=A[i][j];
    return B;
}

Matrix strassen_mul(const Matrix &A, const Matrix &B, int cutoff=64) {
    int n = A.size();
    if (n <= cutoff) return naive_mul(A,B);
    // pad
    int p = 1; while (p<n) p<<=1;
    if (p!=n) {
        Matrix Ap = pad_to_pow2(A);
        Matrix Bp = pad_to_pow2(B);
        Matrix Cp = strassen_mul(Ap,Bp,cutoff);
        Matrix C(n, vector<double>(n));
        for (int i=0;i<n;++i) for (int j=0;j<n;++j) C[i][j]=Cp[i][j];
        return C;
    }
    Matrix A11,A12,A21,A22,B11,B12,B21,B22;
    split(A,A11,A12,A21,A22);
    split(B,B11,B12,B21,B22);
    Matrix M1 = strassen_mul(add(A11,A22), add(B11,B22), cutoff);
    Matrix M2 = strassen_mul(add(A21,A22), B11, cutoff);
    Matrix M3 = strassen_mul(A11, subm(B12,B22), cutoff);
    Matrix M4 = strassen_mul(A22, subm(B21,B11), cutoff);
    Matrix M5 = strassen_mul(add(A11,A12), B22, cutoff);
    Matrix M6 = strassen_mul(subm(A21,A11), add(B11,B12), cutoff);
    Matrix M7 = strassen_mul(subm(A12,A22), add(B21,B22), cutoff);

    Matrix C11 = add( subm( add(M1,M4), M5 ), M7 );
    Matrix C12 = add(M3,M5);
    Matrix C21 = add(M2,M4);
    Matrix C22 = add( subm( add(M1,M3), M2 ), M6 );
    return join(C11,C12,C21,C22);
}

int main(){
    int n = 128; // example
    Matrix A(n, vector<double>(n)), B(n, vector<double>(n));
    std::mt19937_64 rng(123);
    std::uniform_real_distribution<double> dist(0.0,1.0);
    for (int i=0;i<n;++i) for (int j=0;j<n;++j) { A[i][j]=dist(rng); B[i][j]=dist(rng); }
    Matrix C1 = naive_mul(A,B);
    Matrix C2 = strassen_mul(A,B, 32);
    // verify
    double maxdiff = 0.0;
    for (int i=0;i<n;++i) for (int j=0;j<n;++j) maxdiff = max(maxdiff, fabs(C1[i][j]-C2[i][j]));
    cout << "max diff: " << maxdiff << endl;
    return 0;
}

