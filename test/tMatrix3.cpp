#include "G3D/G3DAll.h"
#include "testassert.h"
using G3D::uint8;
using G3D::uint32;
using G3D::uint64;

static void mul(float (&A)[3][3], float (&B)[3][3], float (&C)[3][3]) {
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            float sum = 0;
            for (int i = 0; i < 3; ++i) {
                sum += A[r][i] * B[i][c];
            }
            C[r][c] = sum;
        }
    }
}


static void testEuler() {
    float x = 1;
    float y = 2;
    float z = -3;

    float x2, y2, z2;

    Matrix3 rX = Matrix3::fromAxisAngle(Vector3::unitX(), x);
    Matrix3 rY = Matrix3::fromAxisAngle(Vector3::unitY(), y);
    Matrix3 rZ = Matrix3::fromAxisAngle(Vector3::unitZ(), z);
    Matrix3 rot = rZ * rX * rY;
    rot.toEulerAnglesZXY(z2, x2, y2);
    testAssert(fuzzyEq(x, x2));
    testAssert(fuzzyEq(y, y2));
    testAssert(fuzzyEq(z, z2));
}


static double frobeniusNormDiff(const Matrix3 &a, const Matrix3& b) {
  double d = 0;

  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      double d0 = a[i][j] - b[i][j];
      d += d0 * d0;
    }
  }

  return sqrt(d);
}

void testPolarDecomposition() {
  printf("G3D::Matrix3::polarDecomposition  (");
  
  // TEST PURE ROTATION
  //--------------------------------
  printf("pure rotation, ");

  Matrix3 A0 = Matrix3::fromAxisAngle(Vector3(1,-2,3).unit(),1.23f);
  Matrix3 R1,S1;
  A0.polarDecomposition(R1,S1);
  Matrix3 A1 = R1 * S1;
  double fn;
  double eps = 0.001;

  // check that it decomposes A0
  fn = frobeniusNormDiff(A0,A1); 
  assert(fn < eps);
  fn = R1.determinant();

  // R is special orthogonal if initial det > 0
  assert(fabs(1 - fn) < eps); 

  // check R is orthogonal
  fn = frobeniusNormDiff(Matrix3::identity(),R1*R1.transpose()); 
  assert(fn < eps);

  // check S is  symmetric
  fn = frobeniusNormDiff(S1,S1.transpose()); 
  assert(fn < eps);

  // check S is identity in this case
  fn = frobeniusNormDiff(S1, Matrix3::identity());
  
  assert(fn < eps);

  // TEST GENERAL MATRIX det>0
  printf("det > 0, ");

  A0 = Matrix3::fromAxisAngle(Vector3(.1f,-1,.3f).unit(), 2.3f);
  A0 *= Matrix3( .1f ,-.2f ,.3f,
                 .3f ,.2f ,.1f,
                 -.1f ,.2f ,.4f);

  assert(A0.determinant() > 0);

  A0.polarDecomposition(R1,S1);
  A1 = R1 * S1;

  // check that it decomposes A0
  fn = frobeniusNormDiff(A0, A1); 
  assert(fn < eps);
  
  fn = R1.determinant();
  // R is special orthagonal if initial det > 0
  assert(fabs(1 - fn) < eps); 

  // check R is orthogonal
  fn = frobeniusNormDiff(Matrix3::identity(),R1*R1.transpose()); 
  assert(fn < eps);

  // check S is  symmetric
  fn = frobeniusNormDiff(S1,S1.transpose()); 
  assert(fn < eps);

  // TEST GENERAL MATRIX det<0
  printf("det < 0, ");

  A0 = Matrix3::fromAxisAngle(Vector3(.1f,-1,.3f).unit(), 2.3f);
  A0 *= Matrix3( -.1f ,-.2f ,.3f,
                 -.3f ,.2f ,.1f,
                 +.1f ,.2f ,.4f);
  
  assert(A0.determinant() < 0);
  
  A0.polarDecomposition(R1,S1);
  fn = R1.determinant();
  
  // Neg det on A0 yields R  that reflects
  assert(fabs(-1 - fn) < eps); 
  A1 = R1 * S1;

  // check that it decomposes A0
  fn = frobeniusNormDiff(A0,A1); 
  assert(fn < eps);

  // check Ris orthogonal
  fn = frobeniusNormDiff(Matrix3::identity(),R1*R1.transpose()); 
  assert(fn < eps);

  // check S issymmetric
  fn = frobeniusNormDiff(S1,S1.transpose()); 
  assert(fn < eps);
 
  printf("done) passed.\n");
}


void testMatrix3() {
    printf("G3D::Matrix3  ");

    Vector3 axis  = Vector3(1.0f, 1.0f, 1.0f);
    float   angle = 1.0f;
    Matrix3 test( Matrix3::fromAxisAngle(axis, angle) );
    testAssert(fuzzyEq(test.determinant(), 1.0f));

    testEuler();

    {
        Matrix3 M = Matrix3::identity();
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                M[i][j] = uniformRandom(0, 1);
            }
        }

        Vector3 v = Vector3::random();

        Vector3 x1 = v * M;
        Vector3 x2 = M.transpose() * v;

        testAssert(x1 == x2);
    }

    printf("passed\n");

    testPolarDecomposition();
}


void perfMatrix3() {
    printf("Matrix3:\n");
    uint64 raw, opt, overhead, naive;

    // 0.5 million operations
    int n = 1024 * 1024 / 2;

    // Use two copies to avoid nice cache behavior
    Matrix3 A = Matrix3::fromAxisAngle(Vector3(1, 2, 1), 1.2f);
    Matrix3 B = Matrix3::fromAxisAngle(Vector3(0, 1, -1), .2f);
    Matrix3 C = Matrix3::zero();

    Matrix3 D = Matrix3::fromAxisAngle(Vector3(1, 2, 1), 1.2f);
    Matrix3 E = Matrix3::fromAxisAngle(Vector3(0, 1, -1), .2f);
    Matrix3 F = Matrix3::zero();

    int i;
    System::beginCycleCount(overhead);
    for (i = n - 1; i >= 0; --i) {
    }
    System::endCycleCount(overhead);

    System::beginCycleCount(raw);
    for (i = n - 1; i >= 0; --i) {
        C = A.transpose();
        F = D.transpose();
        C = B.transpose();
    }
    System::endCycleCount(raw);

    System::beginCycleCount(opt);
    for (i = n - 1; i >= 0; --i) {
        Matrix3::transpose(A, C);
        Matrix3::transpose(D, F);
        Matrix3::transpose(B, C);
    }
    System::endCycleCount(opt);

    raw -= overhead;
    opt -= overhead;

    printf(" Transpose Performance                       outcome\n");
    printf("     transpose(A, C): %g cycles/mul       %s\n\n", 
        (double)opt / (3*n), (opt/(3*n) < 400) ? " ok " : "FAIL");
    printf("   C = A.transpose(): %g cycles/mul       %s\n", 
        (double)raw / (3*n), (raw/(3*n) < 150) ? " ok " : "FAIL");
    printf("\n");
    /////////////////////////////////


    printf(" Matrix-Matrix Multiplication\n");
    System::beginCycleCount(raw);
    for (i = n - 1; i >= 0; --i) {
        C = A * B;
        F = D * E;
        C = A * D;
    }
    System::endCycleCount(raw);

    System::beginCycleCount(opt);
    for (i = n - 1; i >= 0; --i) {
        Matrix3::mul(A, B, C);
        Matrix3::mul(D, E, F);
        Matrix3::mul(A, D, C);
    }
    System::endCycleCount(opt);

    
    {
        float A[3][3], B[3][3], C[3][3], D[3][3], E[3][3], F[3][3];

        System::beginCycleCount(naive);
        for (i = n - 1; i >= 0; --i) {
            mul(A, B, C);
            mul(D, E, F);
            mul(A, D, C);
        }
        System::endCycleCount(naive);
    }

    raw -= overhead;
    opt -= overhead;
    
    printf("  mul(A, B, C)          %g cycles/mul     %s\n", (double)opt / (3*n), (opt/(3*n) < 250) ? " ok " : "FAIL");
    printf("     C = A * B          %g cycles/mul     %s\n", (double)raw / (3*n), (raw/(3*n) < 500) ? " ok " : "FAIL");
    printf("  naive for-loops       %g cycles/mul\n", (double)naive / (3*n));

    printf("\n\n");
}
