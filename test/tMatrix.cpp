#include "G3D/G3DAll.h"
#include "testassert.h"
using G3D::uint8;
using G3D::uint32;
using G3D::uint64;


void testPseudoInverse() {
#ifdef G3D_WINDOWS
    // Windows seems to preserve precision better when compiling the SVD code.
    float normThreshold = 0.0009f;
#else
    float normThreshold = 0.04f;
#endif

    for(int n = 4; n <= 30; ++n) {
        const Matrix& A = Matrix::random(1,n);
        const Matrix& B = Matrix::random(n,1);
        const Matrix& C = Matrix::random(2,n);
        const Matrix& D = Matrix::random(n,2);
        const Matrix& E = Matrix::random(3,n);
        const Matrix& F = Matrix::random(n,3);
        const Matrix& G = Matrix::random(4,n);
        const Matrix& H = Matrix::random(n,4);
        const Matrix& A1 = A.pseudoInverse();
        const Matrix& A2 = A.svdPseudoInverse();
        const Matrix& B1 = B.pseudoInverse();
        const Matrix& B2 = B.svdPseudoInverse();
        const Matrix& C1 = C.pseudoInverse();
        const Matrix& C2 = C.svdPseudoInverse();
        const Matrix& D1 = D.pseudoInverse();
        const Matrix& D2 = D.svdPseudoInverse();
        const Matrix& E1 = E.pseudoInverse();
        const Matrix& E2 = E.svdPseudoInverse();
        const Matrix& F1 = F.pseudoInverse();
        const Matrix& F2 = F.svdPseudoInverse();
        const Matrix& G1 = G.pseudoInverse();
        const Matrix& G2 = G.svdPseudoInverse();
        const Matrix& H1 = H.pseudoInverse();
        const Matrix& H2 = H.svdPseudoInverse();

        testAssertM((A1-A2).norm() < normThreshold, format("%dx1 case failed",n));
        testAssertM((B1-B2).norm() < normThreshold, format("1x%d case failed",n));
        testAssertM((C1-C2).norm() < normThreshold, format("%dx2 case failed",n));
        testAssertM((D1-D2).norm() < normThreshold, format("2x%d case failed",n));
        testAssertM((E1-E2).norm() < normThreshold, format("%dx3 case failed",n));
        testAssertM((F1-F2).norm() < normThreshold, format("3x%d case failed",n));
        testAssertM((G1-G2).norm() < normThreshold, format("%dx4 case failed, error = %f", n,
                                                            (G1-G2).norm()));

        /*
        float x = (H1-H2).norm();
        printf("x = %f, cutoff = %f\n", x, normThreshold);
        */
        /*
        printf("H1 = %s\n", H1.toString().c_str());
        printf("H2 = %s\n", H2.toString().c_str());
        */
        testAssertM((H1-H2).norm() < normThreshold, format("4x%d case failed, error=%f",n,(H1-H2).norm()));
    }
}

void testMatrix() {
    printf("Matrix ");
    // Zeros
    {
        Matrix M(3, 4);
        testAssert(M.rows() == 3);
        testAssert(M.cols() == 4);
        testAssert(M.get(0, 0) == 0);
        testAssert(M.get(1, 1) == 0);
    }

    // Identity
    {
        Matrix M = Matrix::identity(4);
        testAssert(M.rows() == 4);
        testAssert(M.cols() == 4);
        testAssert(M.get(0, 0) == 1);
        testAssert(M.get(0, 1) == 0);
    }

    // Add
    {
        Matrix A = Matrix::random(2, 3);
        Matrix B = Matrix::random(2, 3);
        Matrix C = A + B;
    
        for (int r = 0; r < 2; ++r) {
            for (int c = 0; c < 3; ++c) {
                testAssert(fuzzyEq(C.get(r, c), A.get(r, c) + B.get(r, c)));
            }
        }
    }

    // Matrix multiply
    {
        Matrix A(2, 2);
        Matrix B(2, 2);

        A.set(0, 0, 1); A.set(0, 1, 3);
        A.set(1, 0, 4); A.set(1, 1, 2);

        B.set(0, 0, -6); B.set(0, 1, 9);
        B.set(1, 0, 1); B.set(1, 1, 7);

        Matrix C = A * B;

        testAssert(fuzzyEq(C.get(0, 0), -3));
        testAssert(fuzzyEq(C.get(0, 1), 30));
        testAssert(fuzzyEq(C.get(1, 0), -22));
        testAssert(fuzzyEq(C.get(1, 1), 50));
    }

    // Transpose
    {
        Matrix A(2, 2);

        A.set(0, 0, 1); A.set(0, 1, 3);
        A.set(1, 0, 4); A.set(1, 1, 2);

        Matrix C = A.transpose();

        testAssert(fuzzyEq(C.get(0, 0), 1));
        testAssert(fuzzyEq(C.get(0, 1), 4));
        testAssert(fuzzyEq(C.get(1, 0), 3));
        testAssert(fuzzyEq(C.get(1, 1), 2));

        A = Matrix::random(3, 4);
        A = A.transpose();

        testAssert(A.rows() == 4);        
        testAssert(A.cols() == 3);
    }

    // Copy-on-mutate
    {

        Matrix::debugNumCopyOps = Matrix::debugNumAllocOps = 0;

        Matrix A = Matrix::identity(2);

        testAssert(Matrix::debugNumAllocOps == 1);
        testAssert(Matrix::debugNumCopyOps == 0);

        Matrix B = A;
        testAssert(Matrix::debugNumAllocOps == 1);
        testAssert(Matrix::debugNumCopyOps == 0);

        B.set(0,0,4);
        testAssert(B.get(0,0) == 4);
        testAssert(A.get(0,0) == 1);
        testAssert(Matrix::debugNumAllocOps == 2);
        testAssert(Matrix::debugNumCopyOps == 1);
    }

    // Inverse
    {
        Matrix A(2, 2);

        A.set(0, 0, 1); A.set(0, 1, 3);
        A.set(1, 0, 4); A.set(1, 1, 2);

        Matrix C = A.inverse();

        testAssert(fuzzyEq(C.get(0, 0), -0.2f));
        testAssert(fuzzyEq(C.get(0, 1), 0.3f));
        testAssert(fuzzyEq(C.get(1, 0), 0.4f));
        testAssert(fuzzyEq(C.get(1, 1), -0.1f));
    }

    {
        Matrix A = Matrix::random(10, 10);
        Matrix B = A.inverse();

        B = B * A;

        for (int r = 0; r < B.rows(); ++r) {
            for (int c = 0; c < B.cols(); ++c) {

                float v = B.get(r, c);
                // The precision isn't great on our inverse, so be tolerant
                if (r == c) {
                    testAssert(abs(v - 1.0f) < 1e-4);
                } else {
                    testAssert(abs(v) < 1e-4);
                }
                (void)v;
            }
        }
    }

    // Negate
    {
        Matrix A = Matrix::random(2, 2);
        Matrix B = -A;

        for (int r = 0; r < A.rows(); ++r) {
            for (int c = 0; c < A.cols(); ++c) {
                testAssert(B.get(r, c) == -A.get(r, c));
            }
        }
    }

    // Transpose
    {
        Matrix A = Matrix::random(3,2);
        Matrix B = A.transpose();
        testAssert(B.rows() == A.cols());
        testAssert(B.cols() == A.rows());

        for (int r = 0; r < A.rows(); ++r) {
            for (int c = 0; c < A.cols(); ++c) {
                testAssert(B.get(c, r) == A.get(r, c));
            }
        }
    }

    // SVD
    {
        Matrix A = Matrix(3, 3);
        A.set(0, 0,  1.0);  A.set(0, 1,  2.0);  A.set(0, 2,  1.0);
        A.set(1, 0, -3.0);  A.set(1, 1,  7.0);  A.set(1, 2, -6.0);
        A.set(2, 0,  4.0);  A.set(2, 1, -4.0);  A.set(2, 2, 10.0);
        A = Matrix::random(27, 15);

        Array<float> D;
        Matrix U, V;

        A.svd(U, D, V);

        // Verify that we can reconstruct
        Matrix B = U * Matrix::fromDiagonal(D) * V.transpose();

        Matrix test = abs(A - B) < 0.1f;

//        A.debugPrint("A");
//        U.debugPrint("U");
//        D.debugPrint("D");
//        V.debugPrint("V");
//        (U * D * V.transpose()).debugPrint("UDV'");

        testAssert(test.allNonZero());

        float m = float((A - B).norm() / A.norm());
        testAssert(m < 0.01f);
        (void)m;
    }

    testPseudoInverse();

    /*
    Matrix a(3, 5);
    a.set(0,0, 1);  a.set(0,1, 2); a.set(0,2,  3); a.set(0,3, 4);  a.set(0,4,  5);
    a.set(1,0, 3);  a.set(1,1, 5); a.set(1,2,  3); a.set(1,3, 1);  a.set(1,4,  2);
    a.set(2,0, 1);  a.set(2,1, 1); a.set(2,2,  1); a.set(2,3, 1);  a.set(2,4,  1);

    Matrix b = a;
    b.set(0,0, 1.8124); b.set(0,1,    0.5341); b.set(0,2,    2.8930); b.set(0,3,    5.2519); b.set(0,4,    4.8829);
    b.set(1,0, 2.5930); b.set(1,1,   2.6022); b.set(1,2,    4.2760); b.set(1,3,    5.9497); b.set(1,4,    6.3751);

    a.debugPrint("a");
    a.debugPrint("b");

    Matrix H = b * a.pseudoInverse();
    H.debugPrint("H");
    */


    printf("passed\n");
}


void testMatrix4() {
    printf("Matrix4 ");
    {
        const double 
            sleft = -0.069638041824473751, 
            sright = 0.062395225117240799,
            sbottom = 0.073294763927117534, 
            stop = -0.07,//-0.073294763927117534, 
            snearval = -0.1f, 
            sfarval = -100.0f;      
        const Matrix4 M = Matrix4::perspectiveProjection(sleft, sright, sbottom, stop, snearval, sfarval);

        double dleft, dright, dbottom, dtop, dnearval, dfarval;
        M.getPerspectiveProjectionParameters(dleft, dright, dbottom, dtop, dnearval, dfarval);

        testAssert(fuzzyEq(sleft, dleft));
        testAssert(fuzzyEq(sright   , dright));
        testAssert(fuzzyEq(stop     , dtop));
        testAssert(fuzzyEq(sbottom  , dbottom));
        testAssert(fuzzyEq(snearval , dnearval));
        testAssert(fabs(sfarval - dfarval) < 0.0001f);
    }

    {
        const double L = -1.0f;
        const double R =  4.0f;
        const double B = -2.0f;
        const double T =  3.0f;
        const double N =  1.5f;
        const double F = 100.2f;
        const Matrix4 P = Matrix4::perspectiveProjection(L, R, B, T, N, F);

        double L2 = 0.0;
        double R2 = 0.0;
        double B2 = 0.0;
        double T2 = 0.0;
        double N2 = 0.0;
        double F2 = 0.0;
        P.getPerspectiveProjectionParameters(L2, R2, B2, T2, N2, F2);
    
        testAssert(fuzzyEq(L, L2));
        testAssert(fuzzyEq(R, R2));
        testAssert(fuzzyEq(B, B2));
        testAssert(fuzzyEq(T, T2));
        testAssert(fuzzyEq(N, N2));
        testAssert(fuzzyEq(F, F2));
    }
    printf("passed\n");
}
